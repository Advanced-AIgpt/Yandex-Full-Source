/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

/** SupportedFeatures possible types */
export interface TFeatureType {}

export enum TFeatureType_EFeatureType {
  /** SimpleSupport - Just return IsSupported("xxx") */
  SimpleSupport = 0,
  /** SupportUnsupportTrue - Check buth IsSupported("xxx") and IsUnsupported("xxx"), return true by default */
  SupportUnsupportTrue = 1,
  /** SupportUnsupportFalse - Check buth IsSupported("xxx") and IsUnsupported("xxx"), return false by default */
  SupportUnsupportFalse = 2,
  /** CustomCode - This feature requires custom user-defined code */
  CustomCode = 3,
  UNRECOGNIZED = -1,
}

export function tFeatureType_EFeatureTypeFromJSON(
  object: any
): TFeatureType_EFeatureType {
  switch (object) {
    case 0:
    case "SimpleSupport":
      return TFeatureType_EFeatureType.SimpleSupport;
    case 1:
    case "SupportUnsupportTrue":
      return TFeatureType_EFeatureType.SupportUnsupportTrue;
    case 2:
    case "SupportUnsupportFalse":
      return TFeatureType_EFeatureType.SupportUnsupportFalse;
    case 3:
    case "CustomCode":
      return TFeatureType_EFeatureType.CustomCode;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TFeatureType_EFeatureType.UNRECOGNIZED;
  }
}

export function tFeatureType_EFeatureTypeToJSON(
  object: TFeatureType_EFeatureType
): string {
  switch (object) {
    case TFeatureType_EFeatureType.SimpleSupport:
      return "SimpleSupport";
    case TFeatureType_EFeatureType.SupportUnsupportTrue:
      return "SupportUnsupportTrue";
    case TFeatureType_EFeatureType.SupportUnsupportFalse:
      return "SupportUnsupportFalse";
    case TFeatureType_EFeatureType.CustomCode:
      return "CustomCode";
    default:
      return "UNKNOWN";
  }
}

/** Type of language dependent String fileds (may contain actual localized texts) */
export interface TLanguageDependent {}

export enum TLanguageDependent_EType {
  /** None - This string field doesn't contain any language specific values */
  None = 0,
  /** NlgText - This string must be created from NLG, use <Text> channel */
  NlgText = 1,
  UNRECOGNIZED = -1,
}

export function tLanguageDependent_ETypeFromJSON(
  object: any
): TLanguageDependent_EType {
  switch (object) {
    case 0:
    case "None":
      return TLanguageDependent_EType.None;
    case 1:
    case "NlgText":
      return TLanguageDependent_EType.NlgText;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TLanguageDependent_EType.UNRECOGNIZED;
  }
}

export function tLanguageDependent_ETypeToJSON(
  object: TLanguageDependent_EType
): string {
  switch (object) {
    case TLanguageDependent_EType.None:
      return "None";
    case TLanguageDependent_EType.NlgText:
      return "NlgText";
    default:
      return "UNKNOWN";
  }
}

function createBaseTFeatureType(): TFeatureType {
  return {};
}

export const TFeatureType = {
  encode(_: TFeatureType, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFeatureType {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFeatureType();
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

  fromJSON(_: any): TFeatureType {
    return {};
  },

  toJSON(_: TFeatureType): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTLanguageDependent(): TLanguageDependent {
  return {};
}

export const TLanguageDependent = {
  encode(_: TLanguageDependent, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TLanguageDependent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLanguageDependent();
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

  fromJSON(_: any): TLanguageDependent {
    return {};
  },

  toJSON(_: TLanguageDependent): unknown {
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
