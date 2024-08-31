/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TFmRadioInfo {
  FmRadioIds: string[];
  SimpleNlu: boolean;
  /** "genre:rock", "mood:happy", etc. */
  ContentMetatag: string;
  TrackId: string;
}

function createBaseTFmRadioInfo(): TFmRadioInfo {
  return { FmRadioIds: [], SimpleNlu: false, ContentMetatag: "", TrackId: "" };
}

export const TFmRadioInfo = {
  encode(message: TFmRadioInfo, writer: Writer = Writer.create()): Writer {
    for (const v of message.FmRadioIds) {
      writer.uint32(10).string(v!);
    }
    if (message.SimpleNlu === true) {
      writer.uint32(16).bool(message.SimpleNlu);
    }
    if (message.ContentMetatag !== "") {
      writer.uint32(26).string(message.ContentMetatag);
    }
    if (message.TrackId !== "") {
      writer.uint32(34).string(message.TrackId);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFmRadioInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFmRadioInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FmRadioIds.push(reader.string());
          break;
        case 2:
          message.SimpleNlu = reader.bool();
          break;
        case 3:
          message.ContentMetatag = reader.string();
          break;
        case 4:
          message.TrackId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFmRadioInfo {
    return {
      FmRadioIds: Array.isArray(object?.fm_radio_ids)
        ? object.fm_radio_ids.map((e: any) => String(e))
        : [],
      SimpleNlu: isSet(object.simple_nlu) ? Boolean(object.simple_nlu) : false,
      ContentMetatag: isSet(object.content_metatag)
        ? String(object.content_metatag)
        : "",
      TrackId: isSet(object.track_id) ? String(object.track_id) : "",
    };
  },

  toJSON(message: TFmRadioInfo): unknown {
    const obj: any = {};
    if (message.FmRadioIds) {
      obj.fm_radio_ids = message.FmRadioIds.map((e) => e);
    } else {
      obj.fm_radio_ids = [];
    }
    message.SimpleNlu !== undefined && (obj.simple_nlu = message.SimpleNlu);
    message.ContentMetatag !== undefined &&
      (obj.content_metatag = message.ContentMetatag);
    message.TrackId !== undefined && (obj.track_id = message.TrackId);
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
