/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NYT";

export interface EWrapperFieldFlag {}

export enum EWrapperFieldFlag_Enum {
  ANY = 0,
  OTHER_COLUMNS = 1,
  ENUM_INT = 2,
  ENUM_STRING = 3,
  /**
   * SERIALIZATION_YT - Fields marked by this flag may correspond to composite types in table,
   * e.g. message to Struct, repeated to List etc.
   */
  SERIALIZATION_YT = 4,
  /**
   * SERIALIZATION_PROTOBUF - Message fields marked by this flag correspond to String column in table
   * (containing serialized representation of the field).
   */
  SERIALIZATION_PROTOBUF = 5,
  /** REQUIRED_LIST - repeated field corresponds to List<T> type. */
  REQUIRED_LIST = 6,
  /** OPTIONAL_LIST - repeated field corresponds to Optional<List<T>> type. */
  OPTIONAL_LIST = 7,
  /**
   * MAP_AS_LIST_OF_STRUCTS_LEGACY - map<Key, Value> corresponds to List<Struct<key: Key, value: String>>, i.e.
   * the type for Value is inferred as if it had SERIALIZATION_PROTOBUF flag.
   */
  MAP_AS_LIST_OF_STRUCTS_LEGACY = 8,
  /**
   * MAP_AS_LIST_OF_STRUCTS - map<Key, Value> corresponds to List<Struct<key: Key, value: Inferred(Value)>>,
   * where Inferred(Value) is the type inferred as if Value had SERIALIZATION_YT flag,
   * e.g. Struct for message type.
   */
  MAP_AS_LIST_OF_STRUCTS = 9,
  /**
   * MAP_AS_DICT - map<Key, Value> corresponds to Dict<Key, Inferred(Value)>.
   * (the option is experimental, see YT-13314)
   */
  MAP_AS_DICT = 10,
  /**
   * MAP_AS_OPTIONAL_DICT - map<Key, Value> corresponds to Optional<Dict<Key, Inferred(Value)>>.
   * (the option is experimental, see YT-13314)
   */
  MAP_AS_OPTIONAL_DICT = 11,
  /** EMBEDDED - ignore this field but unfold subfields instead while (de)serializing */
  EMBEDDED = 12,
  UNRECOGNIZED = -1,
}

export function eWrapperFieldFlag_EnumFromJSON(
  object: any
): EWrapperFieldFlag_Enum {
  switch (object) {
    case 0:
    case "ANY":
      return EWrapperFieldFlag_Enum.ANY;
    case 1:
    case "OTHER_COLUMNS":
      return EWrapperFieldFlag_Enum.OTHER_COLUMNS;
    case 2:
    case "ENUM_INT":
      return EWrapperFieldFlag_Enum.ENUM_INT;
    case 3:
    case "ENUM_STRING":
      return EWrapperFieldFlag_Enum.ENUM_STRING;
    case 4:
    case "SERIALIZATION_YT":
      return EWrapperFieldFlag_Enum.SERIALIZATION_YT;
    case 5:
    case "SERIALIZATION_PROTOBUF":
      return EWrapperFieldFlag_Enum.SERIALIZATION_PROTOBUF;
    case 6:
    case "REQUIRED_LIST":
      return EWrapperFieldFlag_Enum.REQUIRED_LIST;
    case 7:
    case "OPTIONAL_LIST":
      return EWrapperFieldFlag_Enum.OPTIONAL_LIST;
    case 8:
    case "MAP_AS_LIST_OF_STRUCTS_LEGACY":
      return EWrapperFieldFlag_Enum.MAP_AS_LIST_OF_STRUCTS_LEGACY;
    case 9:
    case "MAP_AS_LIST_OF_STRUCTS":
      return EWrapperFieldFlag_Enum.MAP_AS_LIST_OF_STRUCTS;
    case 10:
    case "MAP_AS_DICT":
      return EWrapperFieldFlag_Enum.MAP_AS_DICT;
    case 11:
    case "MAP_AS_OPTIONAL_DICT":
      return EWrapperFieldFlag_Enum.MAP_AS_OPTIONAL_DICT;
    case 12:
    case "EMBEDDED":
      return EWrapperFieldFlag_Enum.EMBEDDED;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EWrapperFieldFlag_Enum.UNRECOGNIZED;
  }
}

export function eWrapperFieldFlag_EnumToJSON(
  object: EWrapperFieldFlag_Enum
): string {
  switch (object) {
    case EWrapperFieldFlag_Enum.ANY:
      return "ANY";
    case EWrapperFieldFlag_Enum.OTHER_COLUMNS:
      return "OTHER_COLUMNS";
    case EWrapperFieldFlag_Enum.ENUM_INT:
      return "ENUM_INT";
    case EWrapperFieldFlag_Enum.ENUM_STRING:
      return "ENUM_STRING";
    case EWrapperFieldFlag_Enum.SERIALIZATION_YT:
      return "SERIALIZATION_YT";
    case EWrapperFieldFlag_Enum.SERIALIZATION_PROTOBUF:
      return "SERIALIZATION_PROTOBUF";
    case EWrapperFieldFlag_Enum.REQUIRED_LIST:
      return "REQUIRED_LIST";
    case EWrapperFieldFlag_Enum.OPTIONAL_LIST:
      return "OPTIONAL_LIST";
    case EWrapperFieldFlag_Enum.MAP_AS_LIST_OF_STRUCTS_LEGACY:
      return "MAP_AS_LIST_OF_STRUCTS_LEGACY";
    case EWrapperFieldFlag_Enum.MAP_AS_LIST_OF_STRUCTS:
      return "MAP_AS_LIST_OF_STRUCTS";
    case EWrapperFieldFlag_Enum.MAP_AS_DICT:
      return "MAP_AS_DICT";
    case EWrapperFieldFlag_Enum.MAP_AS_OPTIONAL_DICT:
      return "MAP_AS_OPTIONAL_DICT";
    case EWrapperFieldFlag_Enum.EMBEDDED:
      return "EMBEDDED";
    default:
      return "UNKNOWN";
  }
}

export interface EWrapperMessageFlag {}

export enum EWrapperMessageFlag_Enum {
  DEPRECATED_SORT_FIELDS_AS_IN_PROTO_FILE = 0,
  SORT_FIELDS_BY_FIELD_NUMBER = 1,
  UNRECOGNIZED = -1,
}

export function eWrapperMessageFlag_EnumFromJSON(
  object: any
): EWrapperMessageFlag_Enum {
  switch (object) {
    case 0:
    case "DEPRECATED_SORT_FIELDS_AS_IN_PROTO_FILE":
      return EWrapperMessageFlag_Enum.DEPRECATED_SORT_FIELDS_AS_IN_PROTO_FILE;
    case 1:
    case "SORT_FIELDS_BY_FIELD_NUMBER":
      return EWrapperMessageFlag_Enum.SORT_FIELDS_BY_FIELD_NUMBER;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EWrapperMessageFlag_Enum.UNRECOGNIZED;
  }
}

export function eWrapperMessageFlag_EnumToJSON(
  object: EWrapperMessageFlag_Enum
): string {
  switch (object) {
    case EWrapperMessageFlag_Enum.DEPRECATED_SORT_FIELDS_AS_IN_PROTO_FILE:
      return "DEPRECATED_SORT_FIELDS_AS_IN_PROTO_FILE";
    case EWrapperMessageFlag_Enum.SORT_FIELDS_BY_FIELD_NUMBER:
      return "SORT_FIELDS_BY_FIELD_NUMBER";
    default:
      return "UNKNOWN";
  }
}

export interface EWrapperOneofFlag {}

export enum EWrapperOneofFlag_Enum {
  /**
   * SEPARATE_FIELDS - Each field inside oneof group corresponds to a regular field of the
   * parent message.
   */
  SEPARATE_FIELDS = 12,
  /** VARIANT - Oneof group corresponds to Variant over Struct. */
  VARIANT = 13,
  UNRECOGNIZED = -1,
}

export function eWrapperOneofFlag_EnumFromJSON(
  object: any
): EWrapperOneofFlag_Enum {
  switch (object) {
    case 12:
    case "SEPARATE_FIELDS":
      return EWrapperOneofFlag_Enum.SEPARATE_FIELDS;
    case 13:
    case "VARIANT":
      return EWrapperOneofFlag_Enum.VARIANT;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EWrapperOneofFlag_Enum.UNRECOGNIZED;
  }
}

export function eWrapperOneofFlag_EnumToJSON(
  object: EWrapperOneofFlag_Enum
): string {
  switch (object) {
    case EWrapperOneofFlag_Enum.SEPARATE_FIELDS:
      return "SEPARATE_FIELDS";
    case EWrapperOneofFlag_Enum.VARIANT:
      return "VARIANT";
    default:
      return "UNKNOWN";
  }
}

function createBaseEWrapperFieldFlag(): EWrapperFieldFlag {
  return {};
}

export const EWrapperFieldFlag = {
  encode(_: EWrapperFieldFlag, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): EWrapperFieldFlag {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseEWrapperFieldFlag();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): EWrapperFieldFlag {
    return {};
  },

  toJSON(_: EWrapperFieldFlag): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseEWrapperMessageFlag(): EWrapperMessageFlag {
  return {};
}

export const EWrapperMessageFlag = {
  encode(_: EWrapperMessageFlag, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): EWrapperMessageFlag {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseEWrapperMessageFlag();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): EWrapperMessageFlag {
    return {};
  },

  toJSON(_: EWrapperMessageFlag): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseEWrapperOneofFlag(): EWrapperOneofFlag {
  return {};
}

export const EWrapperOneofFlag = {
  encode(_: EWrapperOneofFlag, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): EWrapperOneofFlag {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseEWrapperOneofFlag();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): EWrapperOneofFlag {
    return {};
  },

  toJSON(_: EWrapperOneofFlag): unknown {
    const obj: any = {};
    return obj;
  },
};

// If you get a compile-error about 'Constructor<Long> and ... have no overlap',
// add '--ts_proto_opt=esModuleInterop=true' as a flag when calling 'protoc'.
if (util.Long !== Long) {
  util.Long = Long as any;
  configure();
}
