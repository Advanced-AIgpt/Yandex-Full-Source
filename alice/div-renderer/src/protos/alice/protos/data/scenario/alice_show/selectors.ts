/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData.NAliceShow";

export interface TDayPart {
  Value: TDayPart_EValue;
}

export enum TDayPart_EValue {
  Undefined = 0,
  Morning = 1,
  Evening = 2,
  Night = 3,
  /** Ambiguous - "Утреннее вечернее шоу" */
  Ambiguous = 100,
  UNRECOGNIZED = -1,
}

export function tDayPart_EValueFromJSON(object: any): TDayPart_EValue {
  switch (object) {
    case 0:
    case "Undefined":
      return TDayPart_EValue.Undefined;
    case 1:
    case "Morning":
      return TDayPart_EValue.Morning;
    case 2:
    case "Evening":
      return TDayPart_EValue.Evening;
    case 3:
    case "Night":
      return TDayPart_EValue.Night;
    case 100:
    case "Ambiguous":
      return TDayPart_EValue.Ambiguous;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TDayPart_EValue.UNRECOGNIZED;
  }
}

export function tDayPart_EValueToJSON(object: TDayPart_EValue): string {
  switch (object) {
    case TDayPart_EValue.Undefined:
      return "Undefined";
    case TDayPart_EValue.Morning:
      return "Morning";
    case TDayPart_EValue.Evening:
      return "Evening";
    case TDayPart_EValue.Night:
      return "Night";
    case TDayPart_EValue.Ambiguous:
      return "Ambiguous";
    default:
      return "UNKNOWN";
  }
}

export interface TAge {
  Value: TAge_EValue;
}

export enum TAge_EValue {
  Undefined = 0,
  Children = 1,
  Adult = 2,
  /** Ambiguous - "Детское взрослое шоу" */
  Ambiguous = 100,
  UNRECOGNIZED = -1,
}

export function tAge_EValueFromJSON(object: any): TAge_EValue {
  switch (object) {
    case 0:
    case "Undefined":
      return TAge_EValue.Undefined;
    case 1:
    case "Children":
      return TAge_EValue.Children;
    case 2:
    case "Adult":
      return TAge_EValue.Adult;
    case 100:
    case "Ambiguous":
      return TAge_EValue.Ambiguous;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAge_EValue.UNRECOGNIZED;
  }
}

export function tAge_EValueToJSON(object: TAge_EValue): string {
  switch (object) {
    case TAge_EValue.Undefined:
      return "Undefined";
    case TAge_EValue.Children:
      return "Children";
    case TAge_EValue.Adult:
      return "Adult";
    case TAge_EValue.Ambiguous:
      return "Ambiguous";
    default:
      return "UNKNOWN";
  }
}

function createBaseTDayPart(): TDayPart {
  return { Value: 0 };
}

export const TDayPart = {
  encode(message: TDayPart, writer: Writer = Writer.create()): Writer {
    if (message.Value !== 0) {
      writer.uint32(8).int32(message.Value);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDayPart {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDayPart();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Value = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDayPart {
    return {
      Value: isSet(object.value) ? tDayPart_EValueFromJSON(object.value) : 0,
    };
  },

  toJSON(message: TDayPart): unknown {
    const obj: any = {};
    message.Value !== undefined &&
      (obj.value = tDayPart_EValueToJSON(message.Value));
    return obj;
  },
};

function createBaseTAge(): TAge {
  return { Value: 0 };
}

export const TAge = {
  encode(message: TAge, writer: Writer = Writer.create()): Writer {
    if (message.Value !== 0) {
      writer.uint32(8).int32(message.Value);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TAge {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAge();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Value = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAge {
    return {
      Value: isSet(object.value) ? tAge_EValueFromJSON(object.value) : 0,
    };
  },

  toJSON(message: TAge): unknown {
    const obj: any = {};
    message.Value !== undefined &&
      (obj.value = tAge_EValueToJSON(message.Value));
    return obj;
  },
};

// If you get a compile-error about 'Constructor<Long> and ... have no overlap',
// add '--ts_proto_opt=esModuleInterop=true' as a flag when calling 'protoc'.
if (util.Long !== Long) {
  util.Long = Long as any;
  configure();
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
