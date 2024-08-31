/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NTv";

export interface TCatalogTag {
  Name: string;
  Id: string;
  Priority: number;
  TagType: TCatalogTag_ETagType;
}

export enum TCatalogTag_ETagType {
  TT_UNKNOWN = 0,
  TT_CONTENT_TYPE = 1,
  TT_GENRE = 2,
  TT_COUNTRY = 3,
  TT_YEAR = 4,
  TT_AGE_RATING = 5,
  TT_QUALITY = 6,
  TT_SORT_ORDER = 7,
  UNRECOGNIZED = -1,
}

export function tCatalogTag_ETagTypeFromJSON(
  object: any
): TCatalogTag_ETagType {
  switch (object) {
    case 0:
    case "TT_UNKNOWN":
      return TCatalogTag_ETagType.TT_UNKNOWN;
    case 1:
    case "TT_CONTENT_TYPE":
      return TCatalogTag_ETagType.TT_CONTENT_TYPE;
    case 2:
    case "TT_GENRE":
      return TCatalogTag_ETagType.TT_GENRE;
    case 3:
    case "TT_COUNTRY":
      return TCatalogTag_ETagType.TT_COUNTRY;
    case 4:
    case "TT_YEAR":
      return TCatalogTag_ETagType.TT_YEAR;
    case 5:
    case "TT_AGE_RATING":
      return TCatalogTag_ETagType.TT_AGE_RATING;
    case 6:
    case "TT_QUALITY":
      return TCatalogTag_ETagType.TT_QUALITY;
    case 7:
    case "TT_SORT_ORDER":
      return TCatalogTag_ETagType.TT_SORT_ORDER;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TCatalogTag_ETagType.UNRECOGNIZED;
  }
}

export function tCatalogTag_ETagTypeToJSON(
  object: TCatalogTag_ETagType
): string {
  switch (object) {
    case TCatalogTag_ETagType.TT_UNKNOWN:
      return "TT_UNKNOWN";
    case TCatalogTag_ETagType.TT_CONTENT_TYPE:
      return "TT_CONTENT_TYPE";
    case TCatalogTag_ETagType.TT_GENRE:
      return "TT_GENRE";
    case TCatalogTag_ETagType.TT_COUNTRY:
      return "TT_COUNTRY";
    case TCatalogTag_ETagType.TT_YEAR:
      return "TT_YEAR";
    case TCatalogTag_ETagType.TT_AGE_RATING:
      return "TT_AGE_RATING";
    case TCatalogTag_ETagType.TT_QUALITY:
      return "TT_QUALITY";
    case TCatalogTag_ETagType.TT_SORT_ORDER:
      return "TT_SORT_ORDER";
    default:
      return "UNKNOWN";
  }
}

export interface TCatalogTags {
  CatalogTagValue: TCatalogTag[];
}

export interface TGetTagsRequest {
  SelectedTags: TCatalogTag[];
}

export interface TGetTagsResponse {
  SelectedTags: TCatalogTag[];
  AvailableTags: TCatalogTag[];
}

function createBaseTCatalogTag(): TCatalogTag {
  return { Name: "", Id: "", Priority: 0, TagType: 0 };
}

export const TCatalogTag = {
  encode(message: TCatalogTag, writer: Writer = Writer.create()): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    if (message.Id !== "") {
      writer.uint32(18).string(message.Id);
    }
    if (message.Priority !== 0) {
      writer.uint32(24).uint32(message.Priority);
    }
    if (message.TagType !== 0) {
      writer.uint32(32).int32(message.TagType);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCatalogTag {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCatalogTag();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Id = reader.string();
          break;
        case 3:
          message.Priority = reader.uint32();
          break;
        case 4:
          message.TagType = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCatalogTag {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Id: isSet(object.id) ? String(object.id) : "",
      Priority: isSet(object.priority) ? Number(object.priority) : 0,
      TagType: isSet(object.tag_type)
        ? tCatalogTag_ETagTypeFromJSON(object.tag_type)
        : 0,
    };
  },

  toJSON(message: TCatalogTag): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Id !== undefined && (obj.id = message.Id);
    message.Priority !== undefined &&
      (obj.priority = Math.round(message.Priority));
    message.TagType !== undefined &&
      (obj.tag_type = tCatalogTag_ETagTypeToJSON(message.TagType));
    return obj;
  },
};

function createBaseTCatalogTags(): TCatalogTags {
  return { CatalogTagValue: [] };
}

export const TCatalogTags = {
  encode(message: TCatalogTags, writer: Writer = Writer.create()): Writer {
    for (const v of message.CatalogTagValue) {
      TCatalogTag.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCatalogTags {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCatalogTags();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CatalogTagValue.push(
            TCatalogTag.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCatalogTags {
    return {
      CatalogTagValue: Array.isArray(object?.catalog_tag_value)
        ? object.catalog_tag_value.map((e: any) => TCatalogTag.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TCatalogTags): unknown {
    const obj: any = {};
    if (message.CatalogTagValue) {
      obj.catalog_tag_value = message.CatalogTagValue.map((e) =>
        e ? TCatalogTag.toJSON(e) : undefined
      );
    } else {
      obj.catalog_tag_value = [];
    }
    return obj;
  },
};

function createBaseTGetTagsRequest(): TGetTagsRequest {
  return { SelectedTags: [] };
}

export const TGetTagsRequest = {
  encode(message: TGetTagsRequest, writer: Writer = Writer.create()): Writer {
    for (const v of message.SelectedTags) {
      TCatalogTag.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TGetTagsRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetTagsRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SelectedTags.push(
            TCatalogTag.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetTagsRequest {
    return {
      SelectedTags: Array.isArray(object?.selected_tags)
        ? object.selected_tags.map((e: any) => TCatalogTag.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TGetTagsRequest): unknown {
    const obj: any = {};
    if (message.SelectedTags) {
      obj.selected_tags = message.SelectedTags.map((e) =>
        e ? TCatalogTag.toJSON(e) : undefined
      );
    } else {
      obj.selected_tags = [];
    }
    return obj;
  },
};

function createBaseTGetTagsResponse(): TGetTagsResponse {
  return { SelectedTags: [], AvailableTags: [] };
}

export const TGetTagsResponse = {
  encode(message: TGetTagsResponse, writer: Writer = Writer.create()): Writer {
    for (const v of message.SelectedTags) {
      TCatalogTag.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    for (const v of message.AvailableTags) {
      TCatalogTag.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TGetTagsResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetTagsResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SelectedTags.push(
            TCatalogTag.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.AvailableTags.push(
            TCatalogTag.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetTagsResponse {
    return {
      SelectedTags: Array.isArray(object?.selected_tags)
        ? object.selected_tags.map((e: any) => TCatalogTag.fromJSON(e))
        : [],
      AvailableTags: Array.isArray(object?.available_tags)
        ? object.available_tags.map((e: any) => TCatalogTag.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TGetTagsResponse): unknown {
    const obj: any = {};
    if (message.SelectedTags) {
      obj.selected_tags = message.SelectedTags.map((e) =>
        e ? TCatalogTag.toJSON(e) : undefined
      );
    } else {
      obj.selected_tags = [];
    }
    if (message.AvailableTags) {
      obj.available_tags = message.AvailableTags.map((e) =>
        e ? TCatalogTag.toJSON(e) : undefined
      );
    } else {
      obj.available_tags = [];
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
