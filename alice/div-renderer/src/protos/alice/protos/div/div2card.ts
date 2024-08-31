/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { TDiv2Id } from "../../../alice/protos/div/div2id";
import { Struct } from "../../../google/protobuf/struct";

export const protobufPackage = "NAlice";

export interface TDiv2Card {
  Body?: { [key: string]: any };
  StringBody: string | undefined;
  /** Whether */
  HideBorders: boolean;
  Text: string;
  /**
   * Templates than can be cached on device between multiple div-card responses
   * map from template name to template content
   */
  GlobalTemplates: { [key: string]: TDiv2Card_Template };
  /** @deprecated */
  CardName: string;
  Id?: TDiv2Id;
}

export interface TDiv2Card_Template {
  Body?: { [key: string]: any };
  StringBody: string | undefined;
}

export interface TDiv2Card_GlobalTemplatesEntry {
  key: string;
  value?: TDiv2Card_Template;
}

function createBaseTDiv2Card(): TDiv2Card {
  return {
    Body: undefined,
    StringBody: undefined,
    HideBorders: false,
    Text: "",
    GlobalTemplates: {},
    CardName: "",
    Id: undefined,
  };
}

export const TDiv2Card = {
  encode(message: TDiv2Card, writer: Writer = Writer.create()): Writer {
    if (message.Body !== undefined) {
      Struct.encode(
        Struct.wrap(message.Body),
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.StringBody !== undefined) {
      writer.uint32(50).string(message.StringBody);
    }
    if (message.HideBorders === true) {
      writer.uint32(16).bool(message.HideBorders);
    }
    if (message.Text !== "") {
      writer.uint32(26).string(message.Text);
    }
    Object.entries(message.GlobalTemplates).forEach(([key, value]) => {
      TDiv2Card_GlobalTemplatesEntry.encode(
        { key: key as any, value },
        writer.uint32(34).fork()
      ).ldelim();
    });
    if (message.CardName !== "") {
      writer.uint32(42).string(message.CardName);
    }
    if (message.Id !== undefined) {
      TDiv2Id.encode(message.Id, writer.uint32(58).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDiv2Card {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDiv2Card();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Body = Struct.unwrap(Struct.decode(reader, reader.uint32()));
          break;
        case 6:
          message.StringBody = reader.string();
          break;
        case 2:
          message.HideBorders = reader.bool();
          break;
        case 3:
          message.Text = reader.string();
          break;
        case 4:
          const entry4 = TDiv2Card_GlobalTemplatesEntry.decode(
            reader,
            reader.uint32()
          );
          if (entry4.value !== undefined) {
            message.GlobalTemplates[entry4.key] = entry4.value;
          }
          break;
        case 5:
          message.CardName = reader.string();
          break;
        case 7:
          message.Id = TDiv2Id.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDiv2Card {
    return {
      Body: isObject(object.body) ? object.body : undefined,
      StringBody: isSet(object.string_body)
        ? String(object.string_body)
        : undefined,
      HideBorders: isSet(object.hide_borders)
        ? Boolean(object.hide_borders)
        : false,
      Text: isSet(object.text) ? String(object.text) : "",
      GlobalTemplates: isObject(object.global_templates)
        ? Object.entries(object.global_templates).reduce<{
            [key: string]: TDiv2Card_Template;
          }>((acc, [key, value]) => {
            acc[key] = TDiv2Card_Template.fromJSON(value);
            return acc;
          }, {})
        : {},
      CardName: isSet(object.card_name) ? String(object.card_name) : "",
      Id: isSet(object.id) ? TDiv2Id.fromJSON(object.id) : undefined,
    };
  },

  toJSON(message: TDiv2Card): unknown {
    const obj: any = {};
    message.Body !== undefined && (obj.body = message.Body);
    message.StringBody !== undefined && (obj.string_body = message.StringBody);
    message.HideBorders !== undefined &&
      (obj.hide_borders = message.HideBorders);
    message.Text !== undefined && (obj.text = message.Text);
    obj.global_templates = {};
    if (message.GlobalTemplates) {
      Object.entries(message.GlobalTemplates).forEach(([k, v]) => {
        obj.global_templates[k] = TDiv2Card_Template.toJSON(v);
      });
    }
    message.CardName !== undefined && (obj.card_name = message.CardName);
    message.Id !== undefined &&
      (obj.id = message.Id ? TDiv2Id.toJSON(message.Id) : undefined);
    return obj;
  },
};

function createBaseTDiv2Card_Template(): TDiv2Card_Template {
  return { Body: undefined, StringBody: undefined };
}

export const TDiv2Card_Template = {
  encode(
    message: TDiv2Card_Template,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Body !== undefined) {
      Struct.encode(
        Struct.wrap(message.Body),
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.StringBody !== undefined) {
      writer.uint32(18).string(message.StringBody);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDiv2Card_Template {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDiv2Card_Template();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Body = Struct.unwrap(Struct.decode(reader, reader.uint32()));
          break;
        case 2:
          message.StringBody = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDiv2Card_Template {
    return {
      Body: isObject(object.body) ? object.body : undefined,
      StringBody: isSet(object.string_body)
        ? String(object.string_body)
        : undefined,
    };
  },

  toJSON(message: TDiv2Card_Template): unknown {
    const obj: any = {};
    message.Body !== undefined && (obj.body = message.Body);
    message.StringBody !== undefined && (obj.string_body = message.StringBody);
    return obj;
  },
};

function createBaseTDiv2Card_GlobalTemplatesEntry(): TDiv2Card_GlobalTemplatesEntry {
  return { key: "", value: undefined };
}

export const TDiv2Card_GlobalTemplatesEntry = {
  encode(
    message: TDiv2Card_GlobalTemplatesEntry,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.key !== "") {
      writer.uint32(10).string(message.key);
    }
    if (message.value !== undefined) {
      TDiv2Card_Template.encode(
        message.value,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDiv2Card_GlobalTemplatesEntry {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDiv2Card_GlobalTemplatesEntry();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.key = reader.string();
          break;
        case 2:
          message.value = TDiv2Card_Template.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDiv2Card_GlobalTemplatesEntry {
    return {
      key: isSet(object.key) ? String(object.key) : "",
      value: isSet(object.value)
        ? TDiv2Card_Template.fromJSON(object.value)
        : undefined,
    };
  },

  toJSON(message: TDiv2Card_GlobalTemplatesEntry): unknown {
    const obj: any = {};
    message.key !== undefined && (obj.key = message.key);
    message.value !== undefined &&
      (obj.value = message.value
        ? TDiv2Card_Template.toJSON(message.value)
        : undefined);
    return obj;
  },
};

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
