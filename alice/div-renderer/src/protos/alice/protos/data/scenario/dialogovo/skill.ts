/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TDialogovoSkillCardData {
  SkillInfo?: TDialogovoSkillCardData_TSkillInfo;
  SkillRequest?: TDialogovoSkillCardData_TSkillRequest;
  SkillResponse?: TDialogovoSkillCardData_TSkillResponse;
}

export interface TDialogovoSkillCardData_TSkillInfo {
  Name: string;
  Logo: string;
  SkillId: string;
}

export interface TDialogovoSkillCardData_TSkillRequest {
  Text: string;
}

export interface TDialogovoSkillCardData_TSkillResponse {
  TextResponse?:
    | TDialogovoSkillCardData_TSkillResponse_TTextResponse
    | undefined;
  BigImageResponse?:
    | TDialogovoSkillCardData_TSkillResponse_TBigImageResponse
    | undefined;
  ImageGalleryResponse?:
    | TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse
    | undefined;
  ItemsListResponse?:
    | TDialogovoSkillCardData_TSkillResponse_TItemsListResponse
    | undefined;
  buttons: TDialogovoSkillCardData_TSkillResponse_TButton[];
  suggests: TDialogovoSkillCardData_TSkillResponse_TSuggest[];
}

export interface TDialogovoSkillCardData_TSkillResponse_TButton {
  Text: string;
  Url: string;
  Payload: string;
}

export interface TDialogovoSkillCardData_TSkillResponse_TSuggest {
  Text: string;
  Url: string;
  Payload: string;
}

export interface TDialogovoSkillCardData_TSkillResponse_TTextResponse {
  Text: string;
}

export interface TDialogovoSkillCardData_TSkillResponse_TImageItem {
  ImageUrl: string;
  Title: string;
  Description: string;
  Button?: TDialogovoSkillCardData_TSkillResponse_TButton;
}

export interface TDialogovoSkillCardData_TSkillResponse_TBigImageResponse {
  ImageItem?: TDialogovoSkillCardData_TSkillResponse_TImageItem;
}

export interface TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse {
  ImageItems: TDialogovoSkillCardData_TSkillResponse_TImageItem[];
}

export interface TDialogovoSkillCardData_TSkillResponse_TItemsListHeader {
  Text: string;
}

export interface TDialogovoSkillCardData_TSkillResponse_TItemsListFooter {
  Text: string;
  Button?: TDialogovoSkillCardData_TSkillResponse_TButton;
}

export interface TDialogovoSkillCardData_TSkillResponse_TItemsListResponse {
  ItemsLisetHeader?: TDialogovoSkillCardData_TSkillResponse_TItemsListHeader;
  ImageItems: TDialogovoSkillCardData_TSkillResponse_TImageItem[];
  ItemsLisetFooter?: TDialogovoSkillCardData_TSkillResponse_TItemsListFooter;
}

export interface TDialogovoSkillTeaserData {
  SkillInfo?: TDialogovoSkillTeaserData_TSkillInfo;
  ImageUrl: string;
  Text: string;
  Title: string;
  Action: string;
}

export interface TDialogovoSkillTeaserData_TSkillInfo {
  Name: string;
  Logo: string;
  SkillId: string;
}

function createBaseTDialogovoSkillCardData(): TDialogovoSkillCardData {
  return {
    SkillInfo: undefined,
    SkillRequest: undefined,
    SkillResponse: undefined,
  };
}

export const TDialogovoSkillCardData = {
  encode(
    message: TDialogovoSkillCardData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SkillInfo !== undefined) {
      TDialogovoSkillCardData_TSkillInfo.encode(
        message.SkillInfo,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.SkillRequest !== undefined) {
      TDialogovoSkillCardData_TSkillRequest.encode(
        message.SkillRequest,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.SkillResponse !== undefined) {
      TDialogovoSkillCardData_TSkillResponse.encode(
        message.SkillResponse,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDialogovoSkillCardData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillCardData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SkillInfo = TDialogovoSkillCardData_TSkillInfo.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.SkillRequest = TDialogovoSkillCardData_TSkillRequest.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.SkillResponse = TDialogovoSkillCardData_TSkillResponse.decode(
            reader,
            reader.uint32()
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData {
    return {
      SkillInfo: isSet(object.skill_info)
        ? TDialogovoSkillCardData_TSkillInfo.fromJSON(object.skill_info)
        : undefined,
      SkillRequest: isSet(object.skill_request)
        ? TDialogovoSkillCardData_TSkillRequest.fromJSON(object.skill_request)
        : undefined,
      SkillResponse: isSet(object.skill_response)
        ? TDialogovoSkillCardData_TSkillResponse.fromJSON(object.skill_response)
        : undefined,
    };
  },

  toJSON(message: TDialogovoSkillCardData): unknown {
    const obj: any = {};
    message.SkillInfo !== undefined &&
      (obj.skill_info = message.SkillInfo
        ? TDialogovoSkillCardData_TSkillInfo.toJSON(message.SkillInfo)
        : undefined);
    message.SkillRequest !== undefined &&
      (obj.skill_request = message.SkillRequest
        ? TDialogovoSkillCardData_TSkillRequest.toJSON(message.SkillRequest)
        : undefined);
    message.SkillResponse !== undefined &&
      (obj.skill_response = message.SkillResponse
        ? TDialogovoSkillCardData_TSkillResponse.toJSON(message.SkillResponse)
        : undefined);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillInfo(): TDialogovoSkillCardData_TSkillInfo {
  return { Name: "", Logo: "", SkillId: "" };
}

export const TDialogovoSkillCardData_TSkillInfo = {
  encode(
    message: TDialogovoSkillCardData_TSkillInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    if (message.Logo !== "") {
      writer.uint32(18).string(message.Logo);
    }
    if (message.SkillId !== "") {
      writer.uint32(26).string(message.SkillId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillCardData_TSkillInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Logo = reader.string();
          break;
        case 3:
          message.SkillId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillInfo {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Logo: isSet(object.logo) ? String(object.logo) : "",
      SkillId: isSet(object.skill_id) ? String(object.skill_id) : "",
    };
  },

  toJSON(message: TDialogovoSkillCardData_TSkillInfo): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Logo !== undefined && (obj.logo = message.Logo);
    message.SkillId !== undefined && (obj.skill_id = message.SkillId);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillRequest(): TDialogovoSkillCardData_TSkillRequest {
  return { Text: "" };
}

export const TDialogovoSkillCardData_TSkillRequest = {
  encode(
    message: TDialogovoSkillCardData_TSkillRequest,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillCardData_TSkillRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillRequest {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
    };
  },

  toJSON(message: TDialogovoSkillCardData_TSkillRequest): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse(): TDialogovoSkillCardData_TSkillResponse {
  return {
    TextResponse: undefined,
    BigImageResponse: undefined,
    ImageGalleryResponse: undefined,
    ItemsListResponse: undefined,
    buttons: [],
    suggests: [],
  };
}

export const TDialogovoSkillCardData_TSkillResponse = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TextResponse !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TTextResponse.encode(
        message.TextResponse,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.BigImageResponse !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TBigImageResponse.encode(
        message.BigImageResponse,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ImageGalleryResponse !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse.encode(
        message.ImageGalleryResponse,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.ItemsListResponse !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TItemsListResponse.encode(
        message.ItemsListResponse,
        writer.uint32(34).fork()
      ).ldelim();
    }
    for (const v of message.buttons) {
      TDialogovoSkillCardData_TSkillResponse_TButton.encode(
        v!,
        writer.uint32(42).fork()
      ).ldelim();
    }
    for (const v of message.suggests) {
      TDialogovoSkillCardData_TSkillResponse_TSuggest.encode(
        v!,
        writer.uint32(50).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillCardData_TSkillResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TextResponse =
            TDialogovoSkillCardData_TSkillResponse_TTextResponse.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.BigImageResponse =
            TDialogovoSkillCardData_TSkillResponse_TBigImageResponse.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.ImageGalleryResponse =
            TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.ItemsListResponse =
            TDialogovoSkillCardData_TSkillResponse_TItemsListResponse.decode(
              reader,
              reader.uint32()
            );
          break;
        case 5:
          message.buttons.push(
            TDialogovoSkillCardData_TSkillResponse_TButton.decode(
              reader,
              reader.uint32()
            )
          );
          break;
        case 6:
          message.suggests.push(
            TDialogovoSkillCardData_TSkillResponse_TSuggest.decode(
              reader,
              reader.uint32()
            )
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillResponse {
    return {
      TextResponse: isSet(object.text_response)
        ? TDialogovoSkillCardData_TSkillResponse_TTextResponse.fromJSON(
            object.text_response
          )
        : undefined,
      BigImageResponse: isSet(object.big_image_response)
        ? TDialogovoSkillCardData_TSkillResponse_TBigImageResponse.fromJSON(
            object.big_image_response
          )
        : undefined,
      ImageGalleryResponse: isSet(object.image_gallery_response)
        ? TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse.fromJSON(
            object.image_gallery_response
          )
        : undefined,
      ItemsListResponse: isSet(object.items_list_response)
        ? TDialogovoSkillCardData_TSkillResponse_TItemsListResponse.fromJSON(
            object.items_list_response
          )
        : undefined,
      buttons: Array.isArray(object?.buttons)
        ? object.buttons.map((e: any) =>
            TDialogovoSkillCardData_TSkillResponse_TButton.fromJSON(e)
          )
        : [],
      suggests: Array.isArray(object?.suggests)
        ? object.suggests.map((e: any) =>
            TDialogovoSkillCardData_TSkillResponse_TSuggest.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TDialogovoSkillCardData_TSkillResponse): unknown {
    const obj: any = {};
    message.TextResponse !== undefined &&
      (obj.text_response = message.TextResponse
        ? TDialogovoSkillCardData_TSkillResponse_TTextResponse.toJSON(
            message.TextResponse
          )
        : undefined);
    message.BigImageResponse !== undefined &&
      (obj.big_image_response = message.BigImageResponse
        ? TDialogovoSkillCardData_TSkillResponse_TBigImageResponse.toJSON(
            message.BigImageResponse
          )
        : undefined);
    message.ImageGalleryResponse !== undefined &&
      (obj.image_gallery_response = message.ImageGalleryResponse
        ? TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse.toJSON(
            message.ImageGalleryResponse
          )
        : undefined);
    message.ItemsListResponse !== undefined &&
      (obj.items_list_response = message.ItemsListResponse
        ? TDialogovoSkillCardData_TSkillResponse_TItemsListResponse.toJSON(
            message.ItemsListResponse
          )
        : undefined);
    if (message.buttons) {
      obj.buttons = message.buttons.map((e) =>
        e ? TDialogovoSkillCardData_TSkillResponse_TButton.toJSON(e) : undefined
      );
    } else {
      obj.buttons = [];
    }
    if (message.suggests) {
      obj.suggests = message.suggests.map((e) =>
        e
          ? TDialogovoSkillCardData_TSkillResponse_TSuggest.toJSON(e)
          : undefined
      );
    } else {
      obj.suggests = [];
    }
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TButton(): TDialogovoSkillCardData_TSkillResponse_TButton {
  return { Text: "", Url: "", Payload: "" };
}

export const TDialogovoSkillCardData_TSkillResponse_TButton = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TButton,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    if (message.Url !== "") {
      writer.uint32(18).string(message.Url);
    }
    if (message.Payload !== "") {
      writer.uint32(26).string(message.Payload);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TButton {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillCardData_TSkillResponse_TButton();
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
          message.Payload = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillResponse_TButton {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
      Url: isSet(object.url) ? String(object.url) : "",
      Payload: isSet(object.payload) ? String(object.payload) : "",
    };
  },

  toJSON(message: TDialogovoSkillCardData_TSkillResponse_TButton): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    message.Url !== undefined && (obj.url = message.Url);
    message.Payload !== undefined && (obj.payload = message.Payload);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TSuggest(): TDialogovoSkillCardData_TSkillResponse_TSuggest {
  return { Text: "", Url: "", Payload: "" };
}

export const TDialogovoSkillCardData_TSkillResponse_TSuggest = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TSuggest,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    if (message.Url !== "") {
      writer.uint32(18).string(message.Url);
    }
    if (message.Payload !== "") {
      writer.uint32(26).string(message.Payload);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TSuggest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillCardData_TSkillResponse_TSuggest();
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
          message.Payload = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillResponse_TSuggest {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
      Url: isSet(object.url) ? String(object.url) : "",
      Payload: isSet(object.payload) ? String(object.payload) : "",
    };
  },

  toJSON(message: TDialogovoSkillCardData_TSkillResponse_TSuggest): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    message.Url !== undefined && (obj.url = message.Url);
    message.Payload !== undefined && (obj.payload = message.Payload);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TTextResponse(): TDialogovoSkillCardData_TSkillResponse_TTextResponse {
  return { Text: "" };
}

export const TDialogovoSkillCardData_TSkillResponse_TTextResponse = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TTextResponse,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TTextResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TTextResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillResponse_TTextResponse {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
    };
  },

  toJSON(
    message: TDialogovoSkillCardData_TSkillResponse_TTextResponse
  ): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TImageItem(): TDialogovoSkillCardData_TSkillResponse_TImageItem {
  return { ImageUrl: "", Title: "", Description: "", Button: undefined };
}

export const TDialogovoSkillCardData_TSkillResponse_TImageItem = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TImageItem,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ImageUrl !== "") {
      writer.uint32(10).string(message.ImageUrl);
    }
    if (message.Title !== "") {
      writer.uint32(18).string(message.Title);
    }
    if (message.Description !== "") {
      writer.uint32(26).string(message.Description);
    }
    if (message.Button !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TButton.encode(
        message.Button,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TImageItem {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TImageItem();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ImageUrl = reader.string();
          break;
        case 2:
          message.Title = reader.string();
          break;
        case 3:
          message.Description = reader.string();
          break;
        case 4:
          message.Button =
            TDialogovoSkillCardData_TSkillResponse_TButton.decode(
              reader,
              reader.uint32()
            );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillCardData_TSkillResponse_TImageItem {
    return {
      ImageUrl: isSet(object.image_url) ? String(object.image_url) : "",
      Title: isSet(object.title) ? String(object.title) : "",
      Description: isSet(object.description) ? String(object.description) : "",
      Button: isSet(object.button)
        ? TDialogovoSkillCardData_TSkillResponse_TButton.fromJSON(object.button)
        : undefined,
    };
  },

  toJSON(message: TDialogovoSkillCardData_TSkillResponse_TImageItem): unknown {
    const obj: any = {};
    message.ImageUrl !== undefined && (obj.image_url = message.ImageUrl);
    message.Title !== undefined && (obj.title = message.Title);
    message.Description !== undefined &&
      (obj.description = message.Description);
    message.Button !== undefined &&
      (obj.button = message.Button
        ? TDialogovoSkillCardData_TSkillResponse_TButton.toJSON(message.Button)
        : undefined);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TBigImageResponse(): TDialogovoSkillCardData_TSkillResponse_TBigImageResponse {
  return { ImageItem: undefined };
}

export const TDialogovoSkillCardData_TSkillResponse_TBigImageResponse = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TBigImageResponse,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ImageItem !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TImageItem.encode(
        message.ImageItem,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TBigImageResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TBigImageResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ImageItem =
            TDialogovoSkillCardData_TSkillResponse_TImageItem.decode(
              reader,
              reader.uint32()
            );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(
    object: any
  ): TDialogovoSkillCardData_TSkillResponse_TBigImageResponse {
    return {
      ImageItem: isSet(object.image_item)
        ? TDialogovoSkillCardData_TSkillResponse_TImageItem.fromJSON(
            object.image_item
          )
        : undefined,
    };
  },

  toJSON(
    message: TDialogovoSkillCardData_TSkillResponse_TBigImageResponse
  ): unknown {
    const obj: any = {};
    message.ImageItem !== undefined &&
      (obj.image_item = message.ImageItem
        ? TDialogovoSkillCardData_TSkillResponse_TImageItem.toJSON(
            message.ImageItem
          )
        : undefined);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse(): TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse {
  return { ImageItems: [] };
}

export const TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.ImageItems) {
      TDialogovoSkillCardData_TSkillResponse_TImageItem.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ImageItems.push(
            TDialogovoSkillCardData_TSkillResponse_TImageItem.decode(
              reader,
              reader.uint32()
            )
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(
    object: any
  ): TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse {
    return {
      ImageItems: Array.isArray(object?.image_items)
        ? object.image_items.map((e: any) =>
            TDialogovoSkillCardData_TSkillResponse_TImageItem.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(
    message: TDialogovoSkillCardData_TSkillResponse_TImageGalleryResponse
  ): unknown {
    const obj: any = {};
    if (message.ImageItems) {
      obj.image_items = message.ImageItems.map((e) =>
        e
          ? TDialogovoSkillCardData_TSkillResponse_TImageItem.toJSON(e)
          : undefined
      );
    } else {
      obj.image_items = [];
    }
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TItemsListHeader(): TDialogovoSkillCardData_TSkillResponse_TItemsListHeader {
  return { Text: "" };
}

export const TDialogovoSkillCardData_TSkillResponse_TItemsListHeader = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TItemsListHeader,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TItemsListHeader {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TItemsListHeader();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(
    object: any
  ): TDialogovoSkillCardData_TSkillResponse_TItemsListHeader {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
    };
  },

  toJSON(
    message: TDialogovoSkillCardData_TSkillResponse_TItemsListHeader
  ): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TItemsListFooter(): TDialogovoSkillCardData_TSkillResponse_TItemsListFooter {
  return { Text: "", Button: undefined };
}

export const TDialogovoSkillCardData_TSkillResponse_TItemsListFooter = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TItemsListFooter,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    if (message.Button !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TButton.encode(
        message.Button,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TItemsListFooter {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TItemsListFooter();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = reader.string();
          break;
        case 2:
          message.Button =
            TDialogovoSkillCardData_TSkillResponse_TButton.decode(
              reader,
              reader.uint32()
            );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(
    object: any
  ): TDialogovoSkillCardData_TSkillResponse_TItemsListFooter {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
      Button: isSet(object.button)
        ? TDialogovoSkillCardData_TSkillResponse_TButton.fromJSON(object.button)
        : undefined,
    };
  },

  toJSON(
    message: TDialogovoSkillCardData_TSkillResponse_TItemsListFooter
  ): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    message.Button !== undefined &&
      (obj.button = message.Button
        ? TDialogovoSkillCardData_TSkillResponse_TButton.toJSON(message.Button)
        : undefined);
    return obj;
  },
};

function createBaseTDialogovoSkillCardData_TSkillResponse_TItemsListResponse(): TDialogovoSkillCardData_TSkillResponse_TItemsListResponse {
  return {
    ItemsLisetHeader: undefined,
    ImageItems: [],
    ItemsLisetFooter: undefined,
  };
}

export const TDialogovoSkillCardData_TSkillResponse_TItemsListResponse = {
  encode(
    message: TDialogovoSkillCardData_TSkillResponse_TItemsListResponse,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ItemsLisetHeader !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TItemsListHeader.encode(
        message.ItemsLisetHeader,
        writer.uint32(10).fork()
      ).ldelim();
    }
    for (const v of message.ImageItems) {
      TDialogovoSkillCardData_TSkillResponse_TImageItem.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ItemsLisetFooter !== undefined) {
      TDialogovoSkillCardData_TSkillResponse_TItemsListFooter.encode(
        message.ItemsLisetFooter,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillCardData_TSkillResponse_TItemsListResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTDialogovoSkillCardData_TSkillResponse_TItemsListResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ItemsLisetHeader =
            TDialogovoSkillCardData_TSkillResponse_TItemsListHeader.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.ImageItems.push(
            TDialogovoSkillCardData_TSkillResponse_TImageItem.decode(
              reader,
              reader.uint32()
            )
          );
          break;
        case 3:
          message.ItemsLisetFooter =
            TDialogovoSkillCardData_TSkillResponse_TItemsListFooter.decode(
              reader,
              reader.uint32()
            );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(
    object: any
  ): TDialogovoSkillCardData_TSkillResponse_TItemsListResponse {
    return {
      ItemsLisetHeader: isSet(object.items_list_header)
        ? TDialogovoSkillCardData_TSkillResponse_TItemsListHeader.fromJSON(
            object.items_list_header
          )
        : undefined,
      ImageItems: Array.isArray(object?.image_items)
        ? object.image_items.map((e: any) =>
            TDialogovoSkillCardData_TSkillResponse_TImageItem.fromJSON(e)
          )
        : [],
      ItemsLisetFooter: isSet(object.items_list_footer)
        ? TDialogovoSkillCardData_TSkillResponse_TItemsListFooter.fromJSON(
            object.items_list_footer
          )
        : undefined,
    };
  },

  toJSON(
    message: TDialogovoSkillCardData_TSkillResponse_TItemsListResponse
  ): unknown {
    const obj: any = {};
    message.ItemsLisetHeader !== undefined &&
      (obj.items_list_header = message.ItemsLisetHeader
        ? TDialogovoSkillCardData_TSkillResponse_TItemsListHeader.toJSON(
            message.ItemsLisetHeader
          )
        : undefined);
    if (message.ImageItems) {
      obj.image_items = message.ImageItems.map((e) =>
        e
          ? TDialogovoSkillCardData_TSkillResponse_TImageItem.toJSON(e)
          : undefined
      );
    } else {
      obj.image_items = [];
    }
    message.ItemsLisetFooter !== undefined &&
      (obj.items_list_footer = message.ItemsLisetFooter
        ? TDialogovoSkillCardData_TSkillResponse_TItemsListFooter.toJSON(
            message.ItemsLisetFooter
          )
        : undefined);
    return obj;
  },
};

function createBaseTDialogovoSkillTeaserData(): TDialogovoSkillTeaserData {
  return {
    SkillInfo: undefined,
    ImageUrl: "",
    Text: "",
    Title: "",
    Action: "",
  };
}

export const TDialogovoSkillTeaserData = {
  encode(
    message: TDialogovoSkillTeaserData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SkillInfo !== undefined) {
      TDialogovoSkillTeaserData_TSkillInfo.encode(
        message.SkillInfo,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.ImageUrl !== "") {
      writer.uint32(18).string(message.ImageUrl);
    }
    if (message.Text !== "") {
      writer.uint32(26).string(message.Text);
    }
    if (message.Title !== "") {
      writer.uint32(34).string(message.Title);
    }
    if (message.Action !== "") {
      writer.uint32(42).string(message.Action);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillTeaserData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillTeaserData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SkillInfo = TDialogovoSkillTeaserData_TSkillInfo.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.ImageUrl = reader.string();
          break;
        case 3:
          message.Text = reader.string();
          break;
        case 4:
          message.Title = reader.string();
          break;
        case 5:
          message.Action = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillTeaserData {
    return {
      SkillInfo: isSet(object.skill_info)
        ? TDialogovoSkillTeaserData_TSkillInfo.fromJSON(object.skill_info)
        : undefined,
      ImageUrl: isSet(object.image_url) ? String(object.image_url) : "",
      Text: isSet(object.text) ? String(object.text) : "",
      Title: isSet(object.title) ? String(object.title) : "",
      Action: isSet(object.action) ? String(object.action) : "",
    };
  },

  toJSON(message: TDialogovoSkillTeaserData): unknown {
    const obj: any = {};
    message.SkillInfo !== undefined &&
      (obj.skill_info = message.SkillInfo
        ? TDialogovoSkillTeaserData_TSkillInfo.toJSON(message.SkillInfo)
        : undefined);
    message.ImageUrl !== undefined && (obj.image_url = message.ImageUrl);
    message.Text !== undefined && (obj.text = message.Text);
    message.Title !== undefined && (obj.title = message.Title);
    message.Action !== undefined && (obj.action = message.Action);
    return obj;
  },
};

function createBaseTDialogovoSkillTeaserData_TSkillInfo(): TDialogovoSkillTeaserData_TSkillInfo {
  return { Name: "", Logo: "", SkillId: "" };
}

export const TDialogovoSkillTeaserData_TSkillInfo = {
  encode(
    message: TDialogovoSkillTeaserData_TSkillInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    if (message.Logo !== "") {
      writer.uint32(18).string(message.Logo);
    }
    if (message.SkillId !== "") {
      writer.uint32(26).string(message.SkillId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDialogovoSkillTeaserData_TSkillInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDialogovoSkillTeaserData_TSkillInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Logo = reader.string();
          break;
        case 3:
          message.SkillId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDialogovoSkillTeaserData_TSkillInfo {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Logo: isSet(object.logo) ? String(object.logo) : "",
      SkillId: isSet(object.skill_id) ? String(object.skill_id) : "",
    };
  },

  toJSON(message: TDialogovoSkillTeaserData_TSkillInfo): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Logo !== undefined && (obj.logo = message.Logo);
    message.SkillId !== undefined && (obj.skill_id = message.SkillId);
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
