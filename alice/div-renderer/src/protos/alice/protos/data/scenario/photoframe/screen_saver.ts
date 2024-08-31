/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TScreenSaverData {
  ImageUrl: string;
}

function createBaseTScreenSaverData(): TScreenSaverData {
  return { ImageUrl: "" };
}

export const TScreenSaverData = {
  encode(message: TScreenSaverData, writer: Writer = Writer.create()): Writer {
    if (message.ImageUrl !== "") {
      writer.uint32(10).string(message.ImageUrl);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TScreenSaverData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTScreenSaverData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ImageUrl = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TScreenSaverData {
    return {
      ImageUrl: isSet(object.image_url) ? String(object.image_url) : "",
    };
  },

  toJSON(message: TScreenSaverData): unknown {
    const obj: any = {};
    message.ImageUrl !== undefined && (obj.image_url = message.ImageUrl);
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
