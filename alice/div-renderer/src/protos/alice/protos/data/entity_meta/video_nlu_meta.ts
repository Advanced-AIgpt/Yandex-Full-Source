/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

export interface TVideoGalleryItemMeta {
  Duration: number;
  Episode: number;
  EpisodesCount: number;
  Genre: string;
  MinAge: number;
  ProviderName: string;
  Rating: number;
  ReleaseYear: number;
  Season: number;
  SeasonsCount: number;
  Type: string;
  ViewCount: number;
  Position: number;
}

function createBaseTVideoGalleryItemMeta(): TVideoGalleryItemMeta {
  return {
    Duration: 0,
    Episode: 0,
    EpisodesCount: 0,
    Genre: "",
    MinAge: 0,
    ProviderName: "",
    Rating: 0,
    ReleaseYear: 0,
    Season: 0,
    SeasonsCount: 0,
    Type: "",
    ViewCount: 0,
    Position: 0,
  };
}

export const TVideoGalleryItemMeta = {
  encode(
    message: TVideoGalleryItemMeta,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Duration !== 0) {
      writer.uint32(8).uint32(message.Duration);
    }
    if (message.Episode !== 0) {
      writer.uint32(16).uint32(message.Episode);
    }
    if (message.EpisodesCount !== 0) {
      writer.uint32(24).uint32(message.EpisodesCount);
    }
    if (message.Genre !== "") {
      writer.uint32(34).string(message.Genre);
    }
    if (message.MinAge !== 0) {
      writer.uint32(40).uint32(message.MinAge);
    }
    if (message.ProviderName !== "") {
      writer.uint32(50).string(message.ProviderName);
    }
    if (message.Rating !== 0) {
      writer.uint32(57).double(message.Rating);
    }
    if (message.ReleaseYear !== 0) {
      writer.uint32(64).uint32(message.ReleaseYear);
    }
    if (message.Season !== 0) {
      writer.uint32(72).uint32(message.Season);
    }
    if (message.SeasonsCount !== 0) {
      writer.uint32(80).uint32(message.SeasonsCount);
    }
    if (message.Type !== "") {
      writer.uint32(90).string(message.Type);
    }
    if (message.ViewCount !== 0) {
      writer.uint32(96).uint64(message.ViewCount);
    }
    if (message.Position !== 0) {
      writer.uint32(104).uint32(message.Position);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TVideoGalleryItemMeta {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoGalleryItemMeta();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Duration = reader.uint32();
          break;
        case 2:
          message.Episode = reader.uint32();
          break;
        case 3:
          message.EpisodesCount = reader.uint32();
          break;
        case 4:
          message.Genre = reader.string();
          break;
        case 5:
          message.MinAge = reader.uint32();
          break;
        case 6:
          message.ProviderName = reader.string();
          break;
        case 7:
          message.Rating = reader.double();
          break;
        case 8:
          message.ReleaseYear = reader.uint32();
          break;
        case 9:
          message.Season = reader.uint32();
          break;
        case 10:
          message.SeasonsCount = reader.uint32();
          break;
        case 11:
          message.Type = reader.string();
          break;
        case 12:
          message.ViewCount = longToNumber(reader.uint64() as Long);
          break;
        case 13:
          message.Position = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoGalleryItemMeta {
    return {
      Duration: isSet(object.duration) ? Number(object.duration) : 0,
      Episode: isSet(object.episode) ? Number(object.episode) : 0,
      EpisodesCount: isSet(object.episodes_count)
        ? Number(object.episodes_count)
        : 0,
      Genre: isSet(object.genre) ? String(object.genre) : "",
      MinAge: isSet(object.min_age) ? Number(object.min_age) : 0,
      ProviderName: isSet(object.provider_name)
        ? String(object.provider_name)
        : "",
      Rating: isSet(object.rating) ? Number(object.rating) : 0,
      ReleaseYear: isSet(object.release_year) ? Number(object.release_year) : 0,
      Season: isSet(object.season) ? Number(object.season) : 0,
      SeasonsCount: isSet(object.seasons_count)
        ? Number(object.seasons_count)
        : 0,
      Type: isSet(object.type) ? String(object.type) : "",
      ViewCount: isSet(object.view_count) ? Number(object.view_count) : 0,
      Position: isSet(object.position) ? Number(object.position) : 0,
    };
  },

  toJSON(message: TVideoGalleryItemMeta): unknown {
    const obj: any = {};
    message.Duration !== undefined &&
      (obj.duration = Math.round(message.Duration));
    message.Episode !== undefined &&
      (obj.episode = Math.round(message.Episode));
    message.EpisodesCount !== undefined &&
      (obj.episodes_count = Math.round(message.EpisodesCount));
    message.Genre !== undefined && (obj.genre = message.Genre);
    message.MinAge !== undefined && (obj.min_age = Math.round(message.MinAge));
    message.ProviderName !== undefined &&
      (obj.provider_name = message.ProviderName);
    message.Rating !== undefined && (obj.rating = message.Rating);
    message.ReleaseYear !== undefined &&
      (obj.release_year = Math.round(message.ReleaseYear));
    message.Season !== undefined && (obj.season = Math.round(message.Season));
    message.SeasonsCount !== undefined &&
      (obj.seasons_count = Math.round(message.SeasonsCount));
    message.Type !== undefined && (obj.type = message.Type);
    message.ViewCount !== undefined &&
      (obj.view_count = Math.round(message.ViewCount));
    message.Position !== undefined &&
      (obj.position = Math.round(message.Position));
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
