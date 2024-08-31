/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { Struct } from "../../../../google/protobuf/struct";

export const protobufPackage = "NAlice.NSmartTv";

export interface TGiftTemplate {}

export interface TTandemTemplate {
  IsTandemDevicesAvailable: boolean;
  IsTandemConnected: boolean;
}

export interface TTemplateRequest {
  /** @deprecated */
  GiftTemplate?: TGiftTemplate | undefined;
  /** @deprecated */
  TandemTemplate?: TTandemTemplate | undefined;
  /** Unordered set of possible featureboarding templates. Featurebording service chooses one and response accordingly */
  RequestedTemplates: TTemplateRequest_TChosenTemplate[];
}

export interface TTemplateRequest_TChosenTemplate {
  GiftTemplate?: TGiftTemplate;
  TandemTemplate?: TTandemTemplate;
}

export interface TReportShownTemplateRequest {
  ShownTemplateName: string;
  ShowTemplateTimestamp: number;
}

export interface TReportShownTemplateResponse {}

export interface TTemplateResponse {
  /** filled if divjson is available, otherwise empty */
  TemplateName: string;
  /** filled if divjson is available */
  DivJson?: { [key: string]: any };
  Ttl: number;
}

function createBaseTGiftTemplate(): TGiftTemplate {
  return {};
}

export const TGiftTemplate = {
  encode(_: TGiftTemplate, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TGiftTemplate {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGiftTemplate();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TGiftTemplate {
    return {};
  },

  toJSON(_: TGiftTemplate): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTandemTemplate(): TTandemTemplate {
  return { IsTandemDevicesAvailable: false, IsTandemConnected: false };
}

export const TTandemTemplate = {
  encode(message: TTandemTemplate, writer: Writer = Writer.create()): Writer {
    if (message.IsTandemDevicesAvailable === true) {
      writer.uint32(8).bool(message.IsTandemDevicesAvailable);
    }
    if (message.IsTandemConnected === true) {
      writer.uint32(16).bool(message.IsTandemConnected);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTandemTemplate {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTandemTemplate();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.IsTandemDevicesAvailable = reader.bool();
          break;
        case 2:
          message.IsTandemConnected = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTandemTemplate {
    return {
      IsTandemDevicesAvailable: isSet(object.is_tandem_devices_available)
        ? Boolean(object.is_tandem_devices_available)
        : false,
      IsTandemConnected: isSet(object.is_tandem_connected)
        ? Boolean(object.is_tandem_connected)
        : false,
    };
  },

  toJSON(message: TTandemTemplate): unknown {
    const obj: any = {};
    message.IsTandemDevicesAvailable !== undefined &&
      (obj.is_tandem_devices_available = message.IsTandemDevicesAvailable);
    message.IsTandemConnected !== undefined &&
      (obj.is_tandem_connected = message.IsTandemConnected);
    return obj;
  },
};

function createBaseTTemplateRequest(): TTemplateRequest {
  return {
    GiftTemplate: undefined,
    TandemTemplate: undefined,
    RequestedTemplates: [],
  };
}

export const TTemplateRequest = {
  encode(message: TTemplateRequest, writer: Writer = Writer.create()): Writer {
    if (message.GiftTemplate !== undefined) {
      TGiftTemplate.encode(
        message.GiftTemplate,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.TandemTemplate !== undefined) {
      TTandemTemplate.encode(
        message.TandemTemplate,
        writer.uint32(18).fork()
      ).ldelim();
    }
    for (const v of message.RequestedTemplates) {
      TTemplateRequest_TChosenTemplate.encode(
        v!,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTemplateRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTemplateRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.GiftTemplate = TGiftTemplate.decode(reader, reader.uint32());
          break;
        case 2:
          message.TandemTemplate = TTandemTemplate.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.RequestedTemplates.push(
            TTemplateRequest_TChosenTemplate.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTemplateRequest {
    return {
      GiftTemplate: isSet(object.gift_template)
        ? TGiftTemplate.fromJSON(object.gift_template)
        : undefined,
      TandemTemplate: isSet(object.tandem_promo_template)
        ? TTandemTemplate.fromJSON(object.tandem_promo_template)
        : undefined,
      RequestedTemplates: Array.isArray(object?.requested_templates)
        ? object.requested_templates.map((e: any) =>
            TTemplateRequest_TChosenTemplate.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TTemplateRequest): unknown {
    const obj: any = {};
    message.GiftTemplate !== undefined &&
      (obj.gift_template = message.GiftTemplate
        ? TGiftTemplate.toJSON(message.GiftTemplate)
        : undefined);
    message.TandemTemplate !== undefined &&
      (obj.tandem_promo_template = message.TandemTemplate
        ? TTandemTemplate.toJSON(message.TandemTemplate)
        : undefined);
    if (message.RequestedTemplates) {
      obj.requested_templates = message.RequestedTemplates.map((e) =>
        e ? TTemplateRequest_TChosenTemplate.toJSON(e) : undefined
      );
    } else {
      obj.requested_templates = [];
    }
    return obj;
  },
};

function createBaseTTemplateRequest_TChosenTemplate(): TTemplateRequest_TChosenTemplate {
  return { GiftTemplate: undefined, TandemTemplate: undefined };
}

export const TTemplateRequest_TChosenTemplate = {
  encode(
    message: TTemplateRequest_TChosenTemplate,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.GiftTemplate !== undefined) {
      TGiftTemplate.encode(
        message.GiftTemplate,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.TandemTemplate !== undefined) {
      TTandemTemplate.encode(
        message.TandemTemplate,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTemplateRequest_TChosenTemplate {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTemplateRequest_TChosenTemplate();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.GiftTemplate = TGiftTemplate.decode(reader, reader.uint32());
          break;
        case 2:
          message.TandemTemplate = TTandemTemplate.decode(
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

  fromJSON(object: any): TTemplateRequest_TChosenTemplate {
    return {
      GiftTemplate: isSet(object.gift_template)
        ? TGiftTemplate.fromJSON(object.gift_template)
        : undefined,
      TandemTemplate: isSet(object.tandem_promo_template)
        ? TTandemTemplate.fromJSON(object.tandem_promo_template)
        : undefined,
    };
  },

  toJSON(message: TTemplateRequest_TChosenTemplate): unknown {
    const obj: any = {};
    message.GiftTemplate !== undefined &&
      (obj.gift_template = message.GiftTemplate
        ? TGiftTemplate.toJSON(message.GiftTemplate)
        : undefined);
    message.TandemTemplate !== undefined &&
      (obj.tandem_promo_template = message.TandemTemplate
        ? TTandemTemplate.toJSON(message.TandemTemplate)
        : undefined);
    return obj;
  },
};

function createBaseTReportShownTemplateRequest(): TReportShownTemplateRequest {
  return { ShownTemplateName: "", ShowTemplateTimestamp: 0 };
}

export const TReportShownTemplateRequest = {
  encode(
    message: TReportShownTemplateRequest,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ShownTemplateName !== "") {
      writer.uint32(10).string(message.ShownTemplateName);
    }
    if (message.ShowTemplateTimestamp !== 0) {
      writer.uint32(16).uint64(message.ShowTemplateTimestamp);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TReportShownTemplateRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTReportShownTemplateRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ShownTemplateName = reader.string();
          break;
        case 2:
          message.ShowTemplateTimestamp = longToNumber(reader.uint64() as Long);
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TReportShownTemplateRequest {
    return {
      ShownTemplateName: isSet(object.shown_template_name)
        ? String(object.shown_template_name)
        : "",
      ShowTemplateTimestamp: isSet(object.show_template_timestamp)
        ? Number(object.show_template_timestamp)
        : 0,
    };
  },

  toJSON(message: TReportShownTemplateRequest): unknown {
    const obj: any = {};
    message.ShownTemplateName !== undefined &&
      (obj.shown_template_name = message.ShownTemplateName);
    message.ShowTemplateTimestamp !== undefined &&
      (obj.show_template_timestamp = Math.round(message.ShowTemplateTimestamp));
    return obj;
  },
};

function createBaseTReportShownTemplateResponse(): TReportShownTemplateResponse {
  return {};
}

export const TReportShownTemplateResponse = {
  encode(
    _: TReportShownTemplateResponse,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TReportShownTemplateResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTReportShownTemplateResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TReportShownTemplateResponse {
    return {};
  },

  toJSON(_: TReportShownTemplateResponse): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTemplateResponse(): TTemplateResponse {
  return { TemplateName: "", DivJson: undefined, Ttl: 0 };
}

export const TTemplateResponse = {
  encode(message: TTemplateResponse, writer: Writer = Writer.create()): Writer {
    if (message.TemplateName !== "") {
      writer.uint32(10).string(message.TemplateName);
    }
    if (message.DivJson !== undefined) {
      Struct.encode(
        Struct.wrap(message.DivJson),
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Ttl !== 0) {
      writer.uint32(24).int32(message.Ttl);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTemplateResponse {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTemplateResponse();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TemplateName = reader.string();
          break;
        case 2:
          message.DivJson = Struct.unwrap(
            Struct.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.Ttl = reader.int32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTemplateResponse {
    return {
      TemplateName: isSet(object.template_name)
        ? String(object.template_name)
        : "",
      DivJson: isObject(object.div_json) ? object.div_json : undefined,
      Ttl: isSet(object.ttl) ? Number(object.ttl) : 0,
    };
  },

  toJSON(message: TTemplateResponse): unknown {
    const obj: any = {};
    message.TemplateName !== undefined &&
      (obj.template_name = message.TemplateName);
    message.DivJson !== undefined && (obj.div_json = message.DivJson);
    message.Ttl !== undefined && (obj.ttl = Math.round(message.Ttl));
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

function isObject(value: any): boolean {
  return typeof value === "object" && value !== null;
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
