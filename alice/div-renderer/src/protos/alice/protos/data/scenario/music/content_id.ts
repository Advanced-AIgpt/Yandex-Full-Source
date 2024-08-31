/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData.NMusic";

export interface TContentId {
  Type: TContentId_EContentType;
  Id: string;
  /** some types (as Radio) have multiple ids */
  Ids: string[];
}

export enum TContentId_EContentType {
  Track = 0,
  Album = 1,
  Artist = 2,
  Playlist = 3,
  Radio = 4,
  Generative = 5,
  FmRadio = 6,
  UNRECOGNIZED = -1,
}

export function tContentId_EContentTypeFromJSON(
  object: any
): TContentId_EContentType {
  switch (object) {
    case 0:
    case "Track":
      return TContentId_EContentType.Track;
    case 1:
    case "Album":
      return TContentId_EContentType.Album;
    case 2:
    case "Artist":
      return TContentId_EContentType.Artist;
    case 3:
    case "Playlist":
      return TContentId_EContentType.Playlist;
    case 4:
    case "Radio":
      return TContentId_EContentType.Radio;
    case 5:
    case "Generative":
      return TContentId_EContentType.Generative;
    case 6:
    case "FmRadio":
      return TContentId_EContentType.FmRadio;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TContentId_EContentType.UNRECOGNIZED;
  }
}

export function tContentId_EContentTypeToJSON(
  object: TContentId_EContentType
): string {
  switch (object) {
    case TContentId_EContentType.Track:
      return "Track";
    case TContentId_EContentType.Album:
      return "Album";
    case TContentId_EContentType.Artist:
      return "Artist";
    case TContentId_EContentType.Playlist:
      return "Playlist";
    case TContentId_EContentType.Radio:
      return "Radio";
    case TContentId_EContentType.Generative:
      return "Generative";
    case TContentId_EContentType.FmRadio:
      return "FmRadio";
    default:
      return "UNKNOWN";
  }
}

function createBaseTContentId(): TContentId {
  return { Type: 0, Id: "", Ids: [] };
}

export const TContentId = {
  encode(message: TContentId, writer: Writer = Writer.create()): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.Id !== "") {
      writer.uint32(18).string(message.Id);
    }
    for (const v of message.Ids) {
      writer.uint32(26).string(v!);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TContentId {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTContentId();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.Id = reader.string();
          break;
        case 3:
          message.Ids.push(reader.string());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TContentId {
    return {
      Type: isSet(object.type)
        ? tContentId_EContentTypeFromJSON(object.type)
        : 0,
      Id: isSet(object.id) ? String(object.id) : "",
      Ids: Array.isArray(object?.ids)
        ? object.ids.map((e: any) => String(e))
        : [],
    };
  },

  toJSON(message: TContentId): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tContentId_EContentTypeToJSON(message.Type));
    message.Id !== undefined && (obj.id = message.Id);
    if (message.Ids) {
      obj.ids = message.Ids.map((e) => e);
    } else {
      obj.ids = [];
    }
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
