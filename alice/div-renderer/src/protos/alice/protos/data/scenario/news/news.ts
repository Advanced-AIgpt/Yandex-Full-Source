/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TNewsItem {
  Text: string;
  Url: string;
  Image?: TNewsItem_TImage;
  Messages: string;
  TurboIconUrl: string;
  Agency: string;
  Logo: string;
  PubDate: number;
  ExtendedNews: TNewsItem[];
}

export interface TNewsItem_TImage {
  Src: string;
  Width: number;
  Height: number;
}

export interface TNewsGalleryData {
  NewsItems: TNewsItem[];
  CurrentNewsItem: number;
  Topic: string;
  Tz: string;
}

export interface TNewsTeaserData {
  NewsItem?: TNewsItem;
  Topic: string;
  Tz: string;
}

export interface TNewsMainScreenData {
  NewsItems: TNewsItem[];
  Topic: string;
  Tz: string;
}

function createBaseTNewsItem(): TNewsItem {
  return {
    Text: "",
    Url: "",
    Image: undefined,
    Messages: "",
    TurboIconUrl: "",
    Agency: "",
    Logo: "",
    PubDate: 0,
    ExtendedNews: [],
  };
}

export const TNewsItem = {
  encode(message: TNewsItem, writer: Writer = Writer.create()): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    if (message.Url !== "") {
      writer.uint32(18).string(message.Url);
    }
    if (message.Image !== undefined) {
      TNewsItem_TImage.encode(message.Image, writer.uint32(26).fork()).ldelim();
    }
    if (message.Messages !== "") {
      writer.uint32(42).string(message.Messages);
    }
    if (message.TurboIconUrl !== "") {
      writer.uint32(50).string(message.TurboIconUrl);
    }
    if (message.Agency !== "") {
      writer.uint32(58).string(message.Agency);
    }
    if (message.Logo !== "") {
      writer.uint32(66).string(message.Logo);
    }
    if (message.PubDate !== 0) {
      writer.uint32(72).uint64(message.PubDate);
    }
    for (const v of message.ExtendedNews) {
      TNewsItem.encode(v!, writer.uint32(82).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsItem {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsItem();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = reader.string();
          break;
        case 2:
          message.Url = reader.string();
          break;
        case 3:
          message.Image = TNewsItem_TImage.decode(reader, reader.uint32());
          break;
        case 5:
          message.Messages = reader.string();
          break;
        case 6:
          message.TurboIconUrl = reader.string();
          break;
        case 7:
          message.Agency = reader.string();
          break;
        case 8:
          message.Logo = reader.string();
          break;
        case 9:
          message.PubDate = longToNumber(reader.uint64() as Long);
          break;
        case 10:
          message.ExtendedNews.push(TNewsItem.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsItem {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
      Url: isSet(object.url) ? String(object.url) : "",
      Image: isSet(object.image)
        ? TNewsItem_TImage.fromJSON(object.image)
        : undefined,
      Messages: isSet(object.messages) ? String(object.messages) : "",
      TurboIconUrl: isSet(object.turbo_icon_url)
        ? String(object.turbo_icon_url)
        : "",
      Agency: isSet(object.agency) ? String(object.agency) : "",
      Logo: isSet(object.logo) ? String(object.logo) : "",
      PubDate: isSet(object.pub_date) ? Number(object.pub_date) : 0,
      ExtendedNews: Array.isArray(object?.extended_news)
        ? object.extended_news.map((e: any) => TNewsItem.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TNewsItem): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    message.Url !== undefined && (obj.url = message.Url);
    message.Image !== undefined &&
      (obj.image = message.Image
        ? TNewsItem_TImage.toJSON(message.Image)
        : undefined);
    message.Messages !== undefined && (obj.messages = message.Messages);
    message.TurboIconUrl !== undefined &&
      (obj.turbo_icon_url = message.TurboIconUrl);
    message.Agency !== undefined && (obj.agency = message.Agency);
    message.Logo !== undefined && (obj.logo = message.Logo);
    message.PubDate !== undefined &&
      (obj.pub_date = Math.round(message.PubDate));
    if (message.ExtendedNews) {
      obj.extended_news = message.ExtendedNews.map((e) =>
        e ? TNewsItem.toJSON(e) : undefined
      );
    } else {
      obj.extended_news = [];
    }
    return obj;
  },
};

function createBaseTNewsItem_TImage(): TNewsItem_TImage {
  return { Src: "", Width: 0, Height: 0 };
}

export const TNewsItem_TImage = {
  encode(message: TNewsItem_TImage, writer: Writer = Writer.create()): Writer {
    if (message.Src !== "") {
      writer.uint32(10).string(message.Src);
    }
    if (message.Width !== 0) {
      writer.uint32(16).int32(message.Width);
    }
    if (message.Height !== 0) {
      writer.uint32(24).int32(message.Height);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsItem_TImage {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsItem_TImage();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Src = reader.string();
          break;
        case 2:
          message.Width = reader.int32();
          break;
        case 3:
          message.Height = reader.int32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsItem_TImage {
    return {
      Src: isSet(object.src) ? String(object.src) : "",
      Width: isSet(object.width) ? Number(object.width) : 0,
      Height: isSet(object.height) ? Number(object.height) : 0,
    };
  },

  toJSON(message: TNewsItem_TImage): unknown {
    const obj: any = {};
    message.Src !== undefined && (obj.src = message.Src);
    message.Width !== undefined && (obj.width = Math.round(message.Width));
    message.Height !== undefined && (obj.height = Math.round(message.Height));
    return obj;
  },
};

function createBaseTNewsGalleryData(): TNewsGalleryData {
  return { NewsItems: [], CurrentNewsItem: 0, Topic: "", Tz: "" };
}

export const TNewsGalleryData = {
  encode(message: TNewsGalleryData, writer: Writer = Writer.create()): Writer {
    for (const v of message.NewsItems) {
      TNewsItem.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    if (message.CurrentNewsItem !== 0) {
      writer.uint32(16).int32(message.CurrentNewsItem);
    }
    if (message.Topic !== "") {
      writer.uint32(26).string(message.Topic);
    }
    if (message.Tz !== "") {
      writer.uint32(34).string(message.Tz);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsGalleryData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsGalleryData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsItems.push(TNewsItem.decode(reader, reader.uint32()));
          break;
        case 2:
          message.CurrentNewsItem = reader.int32();
          break;
        case 3:
          message.Topic = reader.string();
          break;
        case 4:
          message.Tz = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsGalleryData {
    return {
      NewsItems: Array.isArray(object?.news_items)
        ? object.news_items.map((e: any) => TNewsItem.fromJSON(e))
        : [],
      CurrentNewsItem: isSet(object.current_news_item)
        ? Number(object.current_news_item)
        : 0,
      Topic: isSet(object.topic) ? String(object.topic) : "",
      Tz: isSet(object.tz) ? String(object.tz) : "",
    };
  },

  toJSON(message: TNewsGalleryData): unknown {
    const obj: any = {};
    if (message.NewsItems) {
      obj.news_items = message.NewsItems.map((e) =>
        e ? TNewsItem.toJSON(e) : undefined
      );
    } else {
      obj.news_items = [];
    }
    message.CurrentNewsItem !== undefined &&
      (obj.current_news_item = Math.round(message.CurrentNewsItem));
    message.Topic !== undefined && (obj.topic = message.Topic);
    message.Tz !== undefined && (obj.tz = message.Tz);
    return obj;
  },
};

function createBaseTNewsTeaserData(): TNewsTeaserData {
  return { NewsItem: undefined, Topic: "", Tz: "" };
}

export const TNewsTeaserData = {
  encode(message: TNewsTeaserData, writer: Writer = Writer.create()): Writer {
    if (message.NewsItem !== undefined) {
      TNewsItem.encode(message.NewsItem, writer.uint32(10).fork()).ldelim();
    }
    if (message.Topic !== "") {
      writer.uint32(18).string(message.Topic);
    }
    if (message.Tz !== "") {
      writer.uint32(26).string(message.Tz);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsTeaserData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsTeaserData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsItem = TNewsItem.decode(reader, reader.uint32());
          break;
        case 2:
          message.Topic = reader.string();
          break;
        case 3:
          message.Tz = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsTeaserData {
    return {
      NewsItem: isSet(object.news_item)
        ? TNewsItem.fromJSON(object.news_item)
        : undefined,
      Topic: isSet(object.topic) ? String(object.topic) : "",
      Tz: isSet(object.tz) ? String(object.tz) : "",
    };
  },

  toJSON(message: TNewsTeaserData): unknown {
    const obj: any = {};
    message.NewsItem !== undefined &&
      (obj.news_item = message.NewsItem
        ? TNewsItem.toJSON(message.NewsItem)
        : undefined);
    message.Topic !== undefined && (obj.topic = message.Topic);
    message.Tz !== undefined && (obj.tz = message.Tz);
    return obj;
  },
};

function createBaseTNewsMainScreenData(): TNewsMainScreenData {
  return { NewsItems: [], Topic: "", Tz: "" };
}

export const TNewsMainScreenData = {
  encode(
    message: TNewsMainScreenData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.NewsItems) {
      TNewsItem.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    if (message.Topic !== "") {
      writer.uint32(18).string(message.Topic);
    }
    if (message.Tz !== "") {
      writer.uint32(26).string(message.Tz);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsMainScreenData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsMainScreenData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsItems.push(TNewsItem.decode(reader, reader.uint32()));
          break;
        case 2:
          message.Topic = reader.string();
          break;
        case 3:
          message.Tz = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsMainScreenData {
    return {
      NewsItems: Array.isArray(object?.news_items)
        ? object.news_items.map((e: any) => TNewsItem.fromJSON(e))
        : [],
      Topic: isSet(object.topic) ? String(object.topic) : "",
      Tz: isSet(object.tz) ? String(object.tz) : "",
    };
  },

  toJSON(message: TNewsMainScreenData): unknown {
    const obj: any = {};
    if (message.NewsItems) {
      obj.news_items = message.NewsItems.map((e) =>
        e ? TNewsItem.toJSON(e) : undefined
      );
    } else {
      obj.news_items = [];
    }
    message.Topic !== undefined && (obj.topic = message.Topic);
    message.Tz !== undefined && (obj.tz = message.Tz);
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
