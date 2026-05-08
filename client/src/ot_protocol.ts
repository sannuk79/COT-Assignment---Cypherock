import { randomBytes } from 'crypto';

import { CryptoOps } from './crypto_ops';

export interface OTInit {
  pointA: Uint8Array;
  bitIndex: number;
}

export interface OTEncrypted {
  encryptedM0: Uint8Array;
  encryptedM1: Uint8Array;
  nonce: Uint8Array;
}

export class AliceOT {
  readonly a: Uint8Array;
  readonly A: Uint8Array;

  constructor(
    readonly m0: Uint8Array,
    readonly m1: Uint8Array,
    readonly ui: Uint8Array,
  ) {
    this.a = CryptoOps.generateRandomScalar();
    this.A = CryptoOps.scalarMultiplyG(this.a);
  }

  generateInit(bitIndex: number): OTInit {
    return {
      pointA: this.A,
      bitIndex,
    };
  }

  processResponse(pointB: Uint8Array): OTEncrypted {
    const k0 = CryptoOps.deriveKey(
      CryptoOps.getXCoordinate(CryptoOps.scalarMultiplyPoint(this.a, pointB)),
    );
    const bMinusA = CryptoOps.pointSubtract(pointB, this.A);
    const k1 = CryptoOps.deriveKey(
      CryptoOps.getXCoordinate(CryptoOps.scalarMultiplyPoint(this.a, bMinusA)),
    );

    return {
      encryptedM0: CryptoOps.xorEncrypt(this.m0, k0),
      encryptedM1: CryptoOps.xorEncrypt(this.m1, k1),
      nonce: randomBytes(12),
    };
  }
}

export class MTAProtocol {
  static computeAliceShare(uiValues: Uint8Array[]): Uint8Array {
    let sum = 0n;
    for (let i = 0; i < uiValues.length; i += 1) {
      sum = (sum + (CryptoOps.bytesToBigInt(uiValues[i]) << BigInt(i))) % CryptoOps.ORDER;
    }
    return CryptoOps.bigIntToBytes((CryptoOps.ORDER - sum) % CryptoOps.ORDER, 32);
  }

  static computeBobShare(mcValues: Uint8Array[]): Uint8Array {
    let sum = 0n;
    for (let i = 0; i < mcValues.length; i += 1) {
      sum = (sum + (CryptoOps.bytesToBigInt(mcValues[i]) << BigInt(i))) % CryptoOps.ORDER;
    }
    return CryptoOps.bigIntToBytes(sum, 32);
  }

  static verify(x: Uint8Array, y: Uint8Array, u: Uint8Array, v: Uint8Array): boolean {
    const xy = (CryptoOps.bytesToBigInt(x) * CryptoOps.bytesToBigInt(y)) % CryptoOps.ORDER;
    const uv = (CryptoOps.bytesToBigInt(u) + CryptoOps.bytesToBigInt(v)) % CryptoOps.ORDER;
    return xy === uv;
  }
}
