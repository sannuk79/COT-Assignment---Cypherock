import { readFileSync } from 'fs';
import net from 'net';
import path from 'path';

import * as protobuf from 'protobufjs';

import { CryptoOps } from './crypto_ops';
import { AliceOT, MTAProtocol } from './ot_protocol';

function getBit(bytes: Uint8Array, bitIndex: number): number {
  const byte = bytes[bytes.length - 1 - Math.floor(bitIndex / 8)];
  return (byte >> (bitIndex % 8)) & 1;
}

type CliOptions = {
  debugReveal: boolean;
  host?: string;
  local: boolean;
  port: number;
  x?: Uint8Array;
  y?: Uint8Array;
};

type ProtoTypes = {
  DebugRevealRequest: protobuf.Type;
  DebugRevealResponse: protobuf.Type;
  MTAShare: protobuf.Type;
  OTEncrypted: protobuf.Type;
  OTInit: protobuf.Type;
  OTResponse: protobuf.Type;
  SessionConfig: protobuf.Type;
};

class FramedSocket {
  private readonly chunks: Buffer[] = [];
  private bufferedLength = 0;
  private ended = false;

  constructor(private readonly socket: net.Socket) {
    socket.on('data', (chunk: Buffer) => {
      this.chunks.push(chunk);
      this.bufferedLength += chunk.length;
    });
    socket.on('end', () => {
      this.ended = true;
    });
  }

  async write(body: Uint8Array): Promise<void> {
    const header = Buffer.alloc(4);
    header.writeUInt32BE(body.length, 0);
    const payload = Buffer.concat([header, Buffer.from(body)]);

    await new Promise<void>((resolve, reject) => {
      this.socket.write(payload, (error?: Error | null) => {
        if (error) {
          reject(error);
          return;
        }
        resolve();
      });
    });
  }

  async read(): Promise<Buffer> {
    await this.waitForBytes(4);
    const header = this.consume(4);
    const size = header.readUInt32BE(0);
    await this.waitForBytes(size);
    return this.consume(size);
  }

  close(): void {
    this.socket.end();
  }

  private async waitForBytes(required: number): Promise<void> {
    if (this.bufferedLength >= required) {
      return;
    }

    await new Promise<void>((resolve, reject) => {
      const onData = () => {
        if (this.bufferedLength >= required) {
          cleanup();
          resolve();
        }
      };

      const onEnd = () => {
        cleanup();
        reject(new Error('socket closed before full frame arrived'));
      };

      const onError = (error: Error) => {
        cleanup();
        reject(error);
      };

      const cleanup = () => {
        this.socket.off('data', onData);
        this.socket.off('end', onEnd);
        this.socket.off('error', onError);
      };

      this.socket.on('data', onData);
      this.socket.on('end', onEnd);
      this.socket.on('error', onError);
    });

    if (this.ended && this.bufferedLength < required) {
      throw new Error('socket closed before enough data was received');
    }
  }

  private consume(size: number): Buffer {
    let remaining = size;
    const out = Buffer.alloc(size);
    let outOffset = 0;

    while (remaining > 0) {
      const chunk = this.chunks[0];
      const take = Math.min(remaining, chunk.length);
      chunk.copy(out, outOffset, 0, take);

      if (take === chunk.length) {
        this.chunks.shift();
      } else {
        this.chunks[0] = chunk.subarray(take);
      }

      outOffset += take;
      remaining -= take;
      this.bufferedLength -= take;
    }

    return out;
  }
}

function loadProtoTypes(): ProtoTypes {
  const protoPath = path.resolve(__dirname, '../../proto/cot.proto');
  const root = protobuf.parse(readFileSync(protoPath, 'utf8')).root;

  return {
    DebugRevealRequest: root.lookupType('DebugRevealRequest') as protobuf.Type,
    DebugRevealResponse: root.lookupType('DebugRevealResponse') as protobuf.Type,
    OTInit: root.lookupType('OTInit') as protobuf.Type,
    OTResponse: root.lookupType('OTResponse') as protobuf.Type,
    OTEncrypted: root.lookupType('OTEncrypted') as protobuf.Type,
    MTAShare: root.lookupType('MTAShare') as protobuf.Type,
    SessionConfig: root.lookupType('SessionConfig') as protobuf.Type,
  };
}

function parseArgs(argv: string[]): CliOptions {
  const options: CliOptions = {
    debugReveal: false,
    local: true,
    port: 8888,
  };

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];

    if (arg === '--x') {
      const value = argv[i + 1];
      if (!value) {
        throw new Error('missing value after --x');
      }
      options.x = CryptoOps.scalarFromHex(value);
      i += 1;
      continue;
    }

    if (arg === '--y') {
      const value = argv[i + 1];
      if (!value) {
        throw new Error('missing value after --y');
      }
      options.y = CryptoOps.scalarFromHex(value);
      i += 1;
      continue;
    }

    if (arg === '--host') {
      const value = argv[i + 1];
      if (!value) {
        throw new Error('missing value after --host');
      }
      options.host = value;
      options.local = false;
      i += 1;
      continue;
    }

    if (arg === '--port') {
      const value = argv[i + 1];
      if (!value) {
        throw new Error('missing value after --port');
      }
      options.port = Number.parseInt(value, 10);
      if (!Number.isInteger(options.port) || options.port <= 0 || options.port > 65535) {
        throw new Error('port must be an integer in range 1..65535');
      }
      i += 1;
      continue;
    }

    if (arg === '--local') {
      options.local = true;
      continue;
    }

    if (arg === '--debug-reveal') {
      options.debugReveal = true;
      continue;
    }

    if (arg === '--help' || arg === '-h') {
      console.log('Usage: npm start -- [--local] [--host <hostname>] [--port <port>] [--x <hex>] [--y <hex>] [--debug-reveal]');
      console.log('Default mode is local demo. Pass --host to connect to the C++ server.');
      process.exit(0);
    }

    throw new Error(`unknown argument: ${arg}`);
  }

  return options;
}

async function runLocalDemo(options: CliOptions): Promise<void> {
  const x = options.x ?? CryptoOps.generateRandomScalar();
  const y = options.y ?? CryptoOps.generateRandomScalar();

  const uiValues: Uint8Array[] = [];
  const mcValues: Uint8Array[] = [];

  for (let i = 0; i < 256; i += 1) {
    const m0 = CryptoOps.generateRandomScalar();
    const m1 = CryptoOps.modAdd(m0, x);
    const alice = new AliceOT(m0, m1, m0);

    const b = CryptoOps.generateRandomScalar();
    const A = alice.generateInit(i).pointA;
    const choiceBit = getBit(y, i);
    const bG = CryptoOps.scalarMultiplyG(b);
    const B = choiceBit === 0 ? bG : CryptoOps.pointAdd(bG, A);

    const encrypted = alice.processResponse(B);
    const shared = CryptoOps.scalarMultiplyPoint(b, A);
    const key = CryptoOps.deriveKey(CryptoOps.getXCoordinate(shared));
    const selected = choiceBit === 0 ? encrypted.encryptedM0 : encrypted.encryptedM1;
    const mc = CryptoOps.xorEncrypt(selected, key);

    uiValues.push(alice.ui);
    mcValues.push(mc);
  }

  const u = MTAProtocol.computeAliceShare(uiValues);
  const v = MTAProtocol.computeBobShare(mcValues);

  console.log(`[Client] multiplicative share x = 0x${CryptoOps.toHex(x)}`);
  console.log(`[Client] multiplicative share y = 0x${CryptoOps.toHex(y)}`);
  console.log(`[Client] additive share U      = 0x${CryptoOps.toHex(u)}`);
  console.log(`[Client] additive share V      = 0x${CryptoOps.toHex(v)}`);
  console.log(`[Client] verification         = ${MTAProtocol.verify(x, y, u, v) ? 'ok' : 'failed'}`);
}

async function connectSocket(host: string, port: number): Promise<net.Socket> {
  return new Promise<net.Socket>((resolve, reject) => {
    const socket = net.createConnection({ host, port }, () => {
      socket.off('error', rejectOnce);
      resolve(socket);
    });
    const rejectOnce = (error: Error) => {
      socket.off('error', rejectOnce);
      reject(error);
    };
    socket.once('error', rejectOnce);
  });
}

async function runNetworkDemo(options: CliOptions): Promise<void> {
  if (options.y) {
    throw new Error('network mode does not accept --y; Bob generates y on the server side');
  }

  const host = options.host ?? '127.0.0.1';
  const socket = await connectSocket(host, options.port);
  const framed = new FramedSocket(socket);
  const proto = loadProtoTypes();

  try {
    const x = options.x ?? CryptoOps.generateRandomScalar();
    await framed.write(proto.SessionConfig.encode({ debugReveal: options.debugReveal }).finish());

    const uiValues: Uint8Array[] = [];

    for (let i = 0; i < 256; i += 1) {
      const m0 = CryptoOps.generateRandomScalar();
      const m1 = CryptoOps.modAdd(m0, x);
      const alice = new AliceOT(m0, m1, m0);

      const initPayload = proto.OTInit.encode({
        pointA: alice.generateInit(i).pointA,
        bitIndex: i,
      }).finish();
      await framed.write(initPayload);

      const responseFrame = await framed.read();
      const responseMessage = proto.OTResponse.decode(responseFrame);
      const pointB = new Uint8Array(proto.OTResponse.toObject(responseMessage, { bytes: Uint8Array }).pointB);

      const encrypted = alice.processResponse(pointB);
      const encryptedPayload = proto.OTEncrypted.encode({
        encryptedM0: encrypted.encryptedM0,
        encryptedM1: encrypted.encryptedM1,
        nonce: encrypted.nonce,
      }).finish();
      await framed.write(encryptedPayload);

      uiValues.push(alice.ui);
    }

    const vFrame = await framed.read();
    const vMessage = proto.MTAShare.decode(vFrame);
    const v = new Uint8Array(proto.MTAShare.toObject(vMessage, { bytes: Uint8Array }).additiveShare);
    const u = MTAProtocol.computeAliceShare(uiValues);

    let y: Uint8Array | undefined;
    let verifiedText = 'unavailable in network mode without revealing Bob\'s secret y';

    if (options.debugReveal) {
      const debugRequest = proto.DebugRevealRequest.encode({
        multiplicativeShareX: x,
        additiveShareU: u,
      }).finish();
      await framed.write(debugRequest);

      const debugResponseFrame = await framed.read();
      const debugResponseMessage = proto.DebugRevealResponse.decode(debugResponseFrame);
      const debugResponse = proto.DebugRevealResponse.toObject(debugResponseMessage, {
        bytes: Uint8Array,
      }) as { multiplicativeShareY: Uint8Array; verified: boolean };
      y = new Uint8Array(debugResponse.multiplicativeShareY);
      verifiedText = debugResponse.verified ? 'ok' : 'failed';
    }

    console.log(`[Client] multiplicative share x = 0x${CryptoOps.toHex(x)}`);
    if (y) {
      console.log(`[Client] multiplicative share y = 0x${CryptoOps.toHex(y)}`);
    }
    console.log(`[Client] additive share U      = 0x${CryptoOps.toHex(u)}`);
    console.log(`[Client] additive share V      = 0x${CryptoOps.toHex(v)}`);
    console.log(`[Client] verification         = ${verifiedText}`);
  } finally {
    framed.close();
  }
}

async function main(): Promise<void> {
  const options = parseArgs(process.argv.slice(2));
  if (options.local) {
    await runLocalDemo(options);
    return;
  }

  await runNetworkDemo(options);
}

void main().catch((error: unknown) => {
  const message = error instanceof Error ? error.message : String(error);
  console.error(`fatal: ${message}`);
  process.exit(1);
});
