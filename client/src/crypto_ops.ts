import { createHash, randomBytes } from 'crypto';

type AffinePoint = {
  infinity: boolean;
  x: bigint;
  y: bigint;
};

export class CryptoOps {
  static readonly ORDER =
    BigInt('0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141');

  private static readonly FIELD =
    BigInt('0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F');

  private static readonly GENERATOR: AffinePoint = {
    infinity: false,
    x: BigInt('55066263022277343669578718895168534326250603453777594175500187360389116729240'),
    y: BigInt('32670510020758816978083085130507043184471273380659243275938904335757337482424'),
  };

  static generateRandomScalar(): Uint8Array {
    while (true) {
      const candidate = randomBytes(32);
      const value = CryptoOps.bytesToBigInt(candidate);
      if (value !== 0n && value < CryptoOps.ORDER) {
        return candidate;
      }
    }
  }

  static scalarMultiplyG(scalar: Uint8Array): Uint8Array {
    return CryptoOps.encodeCompressedPoint(
      CryptoOps.scalarMultiply(CryptoOps.bytesToBigInt(scalar), CryptoOps.GENERATOR),
    );
  }

  static scalarMultiplyPoint(scalar: Uint8Array, point: Uint8Array): Uint8Array {
    const decoded = CryptoOps.decodeCompressedPoint(point);
    return CryptoOps.encodeCompressedPoint(
      CryptoOps.scalarMultiply(CryptoOps.bytesToBigInt(scalar), decoded),
    );
  }

  static pointAdd(lhs: Uint8Array, rhs: Uint8Array): Uint8Array {
    return CryptoOps.encodeCompressedPoint(
      CryptoOps.addPoints(
        CryptoOps.decodeCompressedPoint(lhs),
        CryptoOps.decodeCompressedPoint(rhs),
      ),
    );
  }

  static pointNegate(point: Uint8Array): Uint8Array {
    return CryptoOps.encodeCompressedPoint(
      CryptoOps.negatePoint(CryptoOps.decodeCompressedPoint(point)),
    );
  }

  static pointSubtract(lhs: Uint8Array, rhs: Uint8Array): Uint8Array {
    return CryptoOps.encodeCompressedPoint(
      CryptoOps.addPoints(
        CryptoOps.decodeCompressedPoint(lhs),
        CryptoOps.negatePoint(CryptoOps.decodeCompressedPoint(rhs)),
      ),
    );
  }

  static getXCoordinate(point: Uint8Array): Uint8Array {
    return CryptoOps.bigIntToBytes(CryptoOps.decodeCompressedPoint(point).x, 32);
  }

  static deriveKey(input: Uint8Array): Uint8Array {
    return createHash('sha256').update(input).digest();
  }

  static xorEncrypt(message: Uint8Array, key: Uint8Array): Uint8Array {
    const out = new Uint8Array(message.length);
    for (let i = 0; i < message.length; i += 1) {
      out[i] = message[i] ^ key[i];
    }
    return out;
  }

  static modAdd(lhs: Uint8Array, rhs: Uint8Array): Uint8Array {
    const value = (CryptoOps.bytesToBigInt(lhs) + CryptoOps.bytesToBigInt(rhs)) % CryptoOps.ORDER;
    return CryptoOps.bigIntToBytes(value, 32);
  }

  static modMul(lhs: Uint8Array, rhs: Uint8Array): Uint8Array {
    const value = (CryptoOps.bytesToBigInt(lhs) * CryptoOps.bytesToBigInt(rhs)) % CryptoOps.ORDER;
    return CryptoOps.bigIntToBytes(value, 32);
  }

  static toHex(bytes: Uint8Array): string {
    return Buffer.from(bytes).toString('hex');
  }

  static scalarFromHex(hex: string): Uint8Array {
    const normalized = hex.toLowerCase().replace(/^0x/, '');
    if (!/^[0-9a-f]+$/.test(normalized)) {
      throw new Error('scalar must be hex encoded');
    }
    if (normalized.length > 64) {
      throw new Error('scalar must fit in 32 bytes');
    }

    const padded = normalized.padStart(64, '0');
    const bytes = Uint8Array.from(Buffer.from(padded, 'hex'));
    const value = CryptoOps.bytesToBigInt(bytes);
    if (value === 0n || value >= CryptoOps.ORDER) {
      throw new Error('scalar must be in range [1, n-1]');
    }

    return bytes;
  }

  static bytesToBigInt(bytes: Uint8Array): bigint {
    let value = 0n;
    for (const byte of bytes) {
      value = (value << 8n) | BigInt(byte);
    }
    return value;
  }

  static bigIntToBytes(value: bigint, length: number): Uint8Array {
    const out = new Uint8Array(length);
    let remaining = value;

    for (let i = length - 1; i >= 0; i -= 1) {
      out[i] = Number(remaining & 0xffn);
      remaining >>= 8n;
    }

    return out;
  }

  private static mod(value: bigint, modulus: bigint): bigint {
    const reduced = value % modulus;
    return reduced >= 0n ? reduced : reduced + modulus;
  }

  private static modPow(base: bigint, exponent: bigint, modulus: bigint): bigint {
    let result = 1n;
    let current = CryptoOps.mod(base, modulus);
    let power = exponent;

    while (power > 0n) {
      if ((power & 1n) !== 0n) {
        result = CryptoOps.mod(result * current, modulus);
      }
      power >>= 1n;
      current = CryptoOps.mod(current * current, modulus);
    }

    return result;
  }

  private static modInverse(value: bigint, modulus: bigint): bigint {
    let t = 0n;
    let newT = 1n;
    let r = modulus;
    let newR = CryptoOps.mod(value, modulus);

    while (newR !== 0n) {
      const quotient = r / newR;
      [t, newT] = [newT, t - quotient * newT];
      [r, newR] = [newR, r - quotient * newR];
    }

    if (r !== 1n) {
      throw new Error('mod inverse does not exist');
    }

    return t < 0n ? t + modulus : t;
  }

  private static negatePoint(point: AffinePoint): AffinePoint {
    if (point.infinity) {
      return point;
    }
    return {
      infinity: false,
      x: point.x,
      y: CryptoOps.mod(-point.y, CryptoOps.FIELD),
    };
  }

  private static addPoints(lhs: AffinePoint, rhs: AffinePoint): AffinePoint {
    if (lhs.infinity) {
      return rhs;
    }
    if (rhs.infinity) {
      return lhs;
    }
    if (lhs.x === rhs.x && CryptoOps.mod(lhs.y + rhs.y, CryptoOps.FIELD) === 0n) {
      return { infinity: true, x: 0n, y: 0n };
    }

    let lambda = 0n;
    if (lhs.x === rhs.x && lhs.y === rhs.y) {
      if (lhs.y === 0n) {
        return { infinity: true, x: 0n, y: 0n };
      }
      lambda = CryptoOps.mod(
        (3n * lhs.x * lhs.x) * CryptoOps.modInverse(2n * lhs.y, CryptoOps.FIELD),
        CryptoOps.FIELD,
      );
    } else {
      lambda = CryptoOps.mod(
        (rhs.y - lhs.y) * CryptoOps.modInverse(rhs.x - lhs.x, CryptoOps.FIELD),
        CryptoOps.FIELD,
      );
    }

    const x3 = CryptoOps.mod(lambda * lambda - lhs.x - rhs.x, CryptoOps.FIELD);
    const y3 = CryptoOps.mod(lambda * (lhs.x - x3) - lhs.y, CryptoOps.FIELD);
    return { infinity: false, x: x3, y: y3 };
  }

  private static scalarMultiply(scalar: bigint, point: AffinePoint): AffinePoint {
    if (scalar <= 0n || scalar >= CryptoOps.ORDER) {
      throw new Error('scalar must be in range [1, n-1]');
    }

    let result: AffinePoint = { infinity: true, x: 0n, y: 0n };
    let addend = point;
    let k = scalar;

    while (k > 0n) {
      if ((k & 1n) !== 0n) {
        result = CryptoOps.addPoints(result, addend);
      }
      k >>= 1n;
      if (k > 0n) {
        addend = CryptoOps.addPoints(addend, addend);
      }
    }

    return result;
  }

  private static decodeCompressedPoint(point: Uint8Array): AffinePoint {
    if (point.length !== 33) {
      throw new Error('compressed secp256k1 point must be 33 bytes');
    }

    const prefix = point[0];
    if (prefix !== 0x02 && prefix !== 0x03) {
      throw new Error('invalid compressed point prefix');
    }

    const x = CryptoOps.bytesToBigInt(point.subarray(1));
    const rhs = CryptoOps.mod(x * x * x + 7n, CryptoOps.FIELD);
    const yCandidate = CryptoOps.modPow(rhs, (CryptoOps.FIELD + 1n) / 4n, CryptoOps.FIELD);

    if (CryptoOps.mod(yCandidate * yCandidate, CryptoOps.FIELD) !== rhs) {
      throw new Error('invalid compressed point');
    }

    const isOdd = (yCandidate & 1n) === 1n;
    const y = isOdd === (prefix === 0x03) ? yCandidate : CryptoOps.FIELD - yCandidate;
    return { infinity: false, x, y };
  }

  private static encodeCompressedPoint(point: AffinePoint): Uint8Array {
    if (point.infinity) {
      throw new Error('cannot encode point at infinity');
    }

    const out = new Uint8Array(33);
    out[0] = (point.y & 1n) === 1n ? 0x03 : 0x02;
    out.set(CryptoOps.bigIntToBytes(point.x, 32), 1);
    return out;
  }
}
