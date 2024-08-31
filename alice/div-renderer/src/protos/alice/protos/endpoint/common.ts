/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

export enum EUnit {
  NoUnit = 0,
  PercentUnit = 1,
  TemperatureKelvinUnit = 2,
  TemperatureCelsiusUnit = 3,
  PressureAtmUnit = 4,
  PressurePascalUnit = 5,
  PressureBarUnit = 6,
  PressureMmHgUnit = 7,
  LuxUnit = 8,
  PPBUnit = 9,
  AmpereUnit = 10,
  VoltUnit = 11,
  WattUnit = 12,
  UNRECOGNIZED = -1,
}

export function eUnitFromJSON(object: any): EUnit {
  switch (object) {
    case 0:
    case "NoUnit":
      return EUnit.NoUnit;
    case 1:
    case "PercentUnit":
      return EUnit.PercentUnit;
    case 2:
    case "TemperatureKelvinUnit":
      return EUnit.TemperatureKelvinUnit;
    case 3:
    case "TemperatureCelsiusUnit":
      return EUnit.TemperatureCelsiusUnit;
    case 4:
    case "PressureAtmUnit":
      return EUnit.PressureAtmUnit;
    case 5:
    case "PressurePascalUnit":
      return EUnit.PressurePascalUnit;
    case 6:
    case "PressureBarUnit":
      return EUnit.PressureBarUnit;
    case 7:
    case "PressureMmHgUnit":
      return EUnit.PressureMmHgUnit;
    case 8:
    case "LuxUnit":
      return EUnit.LuxUnit;
    case 9:
    case "PPBUnit":
      return EUnit.PPBUnit;
    case 10:
    case "AmpereUnit":
      return EUnit.AmpereUnit;
    case 11:
    case "VoltUnit":
      return EUnit.VoltUnit;
    case 12:
    case "WattUnit":
      return EUnit.WattUnit;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EUnit.UNRECOGNIZED;
  }
}

export function eUnitToJSON(object: EUnit): string {
  switch (object) {
    case EUnit.NoUnit:
      return "NoUnit";
    case EUnit.PercentUnit:
      return "PercentUnit";
    case EUnit.TemperatureKelvinUnit:
      return "TemperatureKelvinUnit";
    case EUnit.TemperatureCelsiusUnit:
      return "TemperatureCelsiusUnit";
    case EUnit.PressureAtmUnit:
      return "PressureAtmUnit";
    case EUnit.PressurePascalUnit:
      return "PressurePascalUnit";
    case EUnit.PressureBarUnit:
      return "PressureBarUnit";
    case EUnit.PressureMmHgUnit:
      return "PressureMmHgUnit";
    case EUnit.LuxUnit:
      return "LuxUnit";
    case EUnit.PPBUnit:
      return "PPBUnit";
    case EUnit.AmpereUnit:
      return "AmpereUnit";
    case EUnit.VoltUnit:
      return "VoltUnit";
    case EUnit.WattUnit:
      return "WattUnit";
    default:
      return "UNKNOWN";
  }
}

export interface TRange {
  Min: number;
  Max: number;
  Precision: number;
}

export interface TPositiveIntegerRange {
  Min: number;
  Max: number;
}

function createBaseTRange(): TRange {
  return { Min: 0, Max: 0, Precision: 0 };
}

export const TRange = {
  encode(message: TRange, writer: Writer = Writer.create()): Writer {
    if (message.Min !== 0) {
      writer.uint32(9).double(message.Min);
    }
    if (message.Max !== 0) {
      writer.uint32(17).double(message.Max);
    }
    if (message.Precision !== 0) {
      writer.uint32(25).double(message.Precision);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TRange {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRange();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Min = reader.double();
          break;
        case 2:
          message.Max = reader.double();
          break;
        case 3:
          message.Precision = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRange {
    return {
      Min: isSet(object.min) ? Number(object.min) : 0,
      Max: isSet(object.max) ? Number(object.max) : 0,
      Precision: isSet(object.precision) ? Number(object.precision) : 0,
    };
  },

  toJSON(message: TRange): unknown {
    const obj: any = {};
    message.Min !== undefined && (obj.min = message.Min);
    message.Max !== undefined && (obj.max = message.Max);
    message.Precision !== undefined && (obj.precision = message.Precision);
    return obj;
  },
};

function createBaseTPositiveIntegerRange(): TPositiveIntegerRange {
  return { Min: 0, Max: 0 };
}

export const TPositiveIntegerRange = {
  encode(
    message: TPositiveIntegerRange,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Min !== 0) {
      writer.uint32(8).uint64(message.Min);
    }
    if (message.Max !== 0) {
      writer.uint32(16).uint64(message.Max);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TPositiveIntegerRange {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPositiveIntegerRange();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Min = longToNumber(reader.uint64() as Long);
          break;
        case 2:
          message.Max = longToNumber(reader.uint64() as Long);
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPositiveIntegerRange {
    return {
      Min: isSet(object.min) ? Number(object.min) : 0,
      Max: isSet(object.max) ? Number(object.max) : 0,
    };
  },

  toJSON(message: TPositiveIntegerRange): unknown {
    const obj: any = {};
    message.Min !== undefined && (obj.min = Math.round(message.Min));
    message.Max !== undefined && (obj.max = Math.round(message.Max));
    return obj;
  },
};

declare var self: any | undefined;
declare var window: any | undefined;
declare var global: any | undefined;
var globalThis: any = (() => {
  if (typeof globalThis !== "undefined") return globalThis;
  if (typeof self !== "undefined") return self;
  if (typeof window !== "undefined") return window;
  if (typeof global !== "undefined") return global;
  throw "Unable to locate global object";
})();

function longToNumber(long: Long): number {
  if (long.gt(Number.MAX_SAFE_INTEGER)) {
    throw new globalThis.Error("Value is larger than Number.MAX_SAFE_INTEGER");
  }
  return long.toNumber();
}

// If you get a compile-error about 'Constructor<Long> and ... have no overlap',
// add '--ts_proto_opt=esModuleInterop=true' as a flag when calling 'protoc'.
if (util.Long !== Long) {
  util.Long = Long as any;
  configure();
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
