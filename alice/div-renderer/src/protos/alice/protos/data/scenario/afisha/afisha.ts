/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TAfishaTeaserData {
  Title: string;
  ImageUrl: string;
  Date: string;
  Place: string;
  ContentRating: string;
}

function createBaseTAfishaTeaserData(): TAfishaTeaserData {
  return { Title: "", ImageUrl: "", Date: "", Place: "", ContentRating: "" };
}

export const TAfishaTeaserData = {
  encode(message: TAfishaTeaserData, writer: Writer = Writer.create()): Writer {
    if (message.Title !== "") {
      writer.uint32(10).string(message.Title);
    }
    if (message.ImageUrl !== "") {
      writer.uint32(18).string(message.ImageUrl);
    }
    if (message.Date !== "") {
      writer.uint32(26).string(message.Date);
    }
    if (message.Place !== "") {
      writer.uint32(34).string(message.Place);
    }
    if (message.ContentRating !== "") {
      writer.uint32(42).string(message.ContentRating);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TAfishaTeaserData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAfishaTeaserData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Title = reader.string();
          break;
        case 2:
          message.ImageUrl = reader.string();
          break;
        case 3:
          message.Date = reader.string();
          break;
        case 4:
          message.Place = reader.string();
          break;
        case 5:
          message.ContentRating = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAfishaTeaserData {
    return {
      Title: isSet(object.title) ? String(object.title) : "",
      ImageUrl: isSet(object.image_url) ? String(object.image_url) : "",
      Date: isSet(object.date) ? String(object.date) : "",
      Place: isSet(object.place) ? String(object.place) : "",
      ContentRating: isSet(object.content_rating)
        ? String(object.content_rating)
        : "",
    };
  },

  toJSON(message: TAfishaTeaserData): unknown {
    const obj: any = {};
    message.Title !== undefined && (obj.title = message.Title);
    message.ImageUrl !== undefined && (obj.image_url = message.ImageUrl);
    message.Date !== undefined && (obj.date = message.Date);
    message.Place !== undefined && (obj.place = message.Place);
    message.ContentRating !== undefined &&
      (obj.content_rating = message.ContentRating);
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
