/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData.NMusic";

export interface TTopic {
  Podcast: string;
}

function createBaseTTopic(): TTopic {
  return { Podcast: "" };
}

export const TTopic = {
  encode(message: TTopic, writer: Writer = Writer.create()): Writer {
    if (message.Podcast !== "") {
      writer.uint32(10).string(message.Podcast);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTopic {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTopic();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Podcast = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTopic {
    return {
      Podcast: isSet(object.podcast) ? String(object.podcast) : "",
    };
  },

  toJSON(message: TTopic): unknown {
    const obj: any = {};
    message.Podcast !== undefined && (obj.podcast = message.Podcast);
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
