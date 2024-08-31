/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TNewsProvider {
  NewsSource: string;
  /** optional */
  Rubric: string;
}

function createBaseTNewsProvider(): TNewsProvider {
  return { NewsSource: "", Rubric: "" };
}

export const TNewsProvider = {
  encode(message: TNewsProvider, writer: Writer = Writer.create()): Writer {
    if (message.NewsSource !== "") {
      writer.uint32(10).string(message.NewsSource);
    }
    if (message.Rubric !== "") {
      writer.uint32(18).string(message.Rubric);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsProvider {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsProvider();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsSource = reader.string();
          break;
        case 2:
          message.Rubric = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsProvider {
    return {
      NewsSource: isSet(object.news_source) ? String(object.news_source) : "",
      Rubric: isSet(object.rubric) ? String(object.rubric) : "",
    };
  },

  toJSON(message: TNewsProvider): unknown {
    const obj: any = {};
    message.NewsSource !== undefined && (obj.news_source = message.NewsSource);
    message.Rubric !== undefined && (obj.rubric = message.Rubric);
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
