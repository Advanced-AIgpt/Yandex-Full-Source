/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TVideoCallMainScreenData {
  TelegramCardData?: TVideoCallMainScreenData_TTelegramCardData | undefined;
}

export interface TVideoCallMainScreenData_TTelegramCardData {
  LoggedIn: boolean;
  ContactsUploaded: boolean;
  UserId: string;
  FavoriteContactData: TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData[];
}

export interface TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData {
  DisplayName: string;
  UserId: string;
  LookupKey: string;
}

export interface TVideoCallContactChoosingData {
  ContactData: TProviderContactData[];
}

export interface TIncomingTelegramCallData {
  UserId: string;
  CallId: string;
  Caller?: TProviderContactData;
}

export interface TOutgoingTelegramCallData {
  UserId: string;
  Recipient?: TProviderContactData;
}

export interface TCurrentTelegramCallData {
  UserId: string;
  CallId: string;
  Recipient?: TProviderContactData;
}

export interface TProviderContactList {
  ContactData: TProviderContactData[];
}

export interface TProviderContactData {
  TelegramContactData?: TProviderContactData_TTelegramContactData | undefined;
}

export interface TProviderContactData_TTelegramContactData {
  DisplayName: string;
  UserId: string;
}

function createBaseTVideoCallMainScreenData(): TVideoCallMainScreenData {
  return { TelegramCardData: undefined };
}

export const TVideoCallMainScreenData = {
  encode(
    message: TVideoCallMainScreenData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TelegramCardData !== undefined) {
      TVideoCallMainScreenData_TTelegramCardData.encode(
        message.TelegramCardData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallMainScreenData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallMainScreenData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TelegramCardData =
            TVideoCallMainScreenData_TTelegramCardData.decode(
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

  fromJSON(object: any): TVideoCallMainScreenData {
    return {
      TelegramCardData: isSet(object.telegram_card_data)
        ? TVideoCallMainScreenData_TTelegramCardData.fromJSON(
            object.telegram_card_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallMainScreenData): unknown {
    const obj: any = {};
    message.TelegramCardData !== undefined &&
      (obj.telegram_card_data = message.TelegramCardData
        ? TVideoCallMainScreenData_TTelegramCardData.toJSON(
            message.TelegramCardData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallMainScreenData_TTelegramCardData(): TVideoCallMainScreenData_TTelegramCardData {
  return {
    LoggedIn: false,
    ContactsUploaded: false,
    UserId: "",
    FavoriteContactData: [],
  };
}

export const TVideoCallMainScreenData_TTelegramCardData = {
  encode(
    message: TVideoCallMainScreenData_TTelegramCardData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.LoggedIn === true) {
      writer.uint32(8).bool(message.LoggedIn);
    }
    if (message.ContactsUploaded === true) {
      writer.uint32(16).bool(message.ContactsUploaded);
    }
    if (message.UserId !== "") {
      writer.uint32(26).string(message.UserId);
    }
    for (const v of message.FavoriteContactData) {
      TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData.encode(
        v!,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallMainScreenData_TTelegramCardData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallMainScreenData_TTelegramCardData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LoggedIn = reader.bool();
          break;
        case 2:
          message.ContactsUploaded = reader.bool();
          break;
        case 3:
          message.UserId = reader.string();
          break;
        case 4:
          message.FavoriteContactData.push(
            TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData.decode(
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

  fromJSON(object: any): TVideoCallMainScreenData_TTelegramCardData {
    return {
      LoggedIn: isSet(object.logged_in) ? Boolean(object.logged_in) : false,
      ContactsUploaded: isSet(object.contacts_uploaded)
        ? Boolean(object.contacts_uploaded)
        : false,
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      FavoriteContactData: Array.isArray(object?.favorite_contact_data)
        ? object.favorite_contact_data.map((e: any) =>
            TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData.fromJSON(
              e
            )
          )
        : [],
    };
  },

  toJSON(message: TVideoCallMainScreenData_TTelegramCardData): unknown {
    const obj: any = {};
    message.LoggedIn !== undefined && (obj.logged_in = message.LoggedIn);
    message.ContactsUploaded !== undefined &&
      (obj.contacts_uploaded = message.ContactsUploaded);
    message.UserId !== undefined && (obj.user_id = message.UserId);
    if (message.FavoriteContactData) {
      obj.favorite_contact_data = message.FavoriteContactData.map((e) =>
        e
          ? TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData.toJSON(
              e
            )
          : undefined
      );
    } else {
      obj.favorite_contact_data = [];
    }
    return obj;
  },
};

function createBaseTVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData(): TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData {
  return { DisplayName: "", UserId: "", LookupKey: "" };
}

export const TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData = {
  encode(
    message: TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DisplayName !== "") {
      writer.uint32(10).string(message.DisplayName);
    }
    if (message.UserId !== "") {
      writer.uint32(18).string(message.UserId);
    }
    if (message.LookupKey !== "") {
      writer.uint32(26).string(message.LookupKey);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DisplayName = reader.string();
          break;
        case 2:
          message.UserId = reader.string();
          break;
        case 3:
          message.LookupKey = reader.string();
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
  ): TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData {
    return {
      DisplayName: isSet(object.display_name)
        ? String(object.display_name)
        : "",
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      LookupKey: isSet(object.lookup_key) ? String(object.lookup_key) : "",
    };
  },

  toJSON(
    message: TVideoCallMainScreenData_TTelegramCardData_TFavoriteContactData
  ): unknown {
    const obj: any = {};
    message.DisplayName !== undefined &&
      (obj.display_name = message.DisplayName);
    message.UserId !== undefined && (obj.user_id = message.UserId);
    message.LookupKey !== undefined && (obj.lookup_key = message.LookupKey);
    return obj;
  },
};

function createBaseTVideoCallContactChoosingData(): TVideoCallContactChoosingData {
  return { ContactData: [] };
}

export const TVideoCallContactChoosingData = {
  encode(
    message: TVideoCallContactChoosingData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.ContactData) {
      TProviderContactData.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallContactChoosingData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallContactChoosingData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContactData.push(
            TProviderContactData.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallContactChoosingData {
    return {
      ContactData: Array.isArray(object?.contact_data)
        ? object.contact_data.map((e: any) => TProviderContactData.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TVideoCallContactChoosingData): unknown {
    const obj: any = {};
    if (message.ContactData) {
      obj.contact_data = message.ContactData.map((e) =>
        e ? TProviderContactData.toJSON(e) : undefined
      );
    } else {
      obj.contact_data = [];
    }
    return obj;
  },
};

function createBaseTIncomingTelegramCallData(): TIncomingTelegramCallData {
  return { UserId: "", CallId: "", Caller: undefined };
}

export const TIncomingTelegramCallData = {
  encode(
    message: TIncomingTelegramCallData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== "") {
      writer.uint32(10).string(message.UserId);
    }
    if (message.CallId !== "") {
      writer.uint32(18).string(message.CallId);
    }
    if (message.Caller !== undefined) {
      TProviderContactData.encode(
        message.Caller,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIncomingTelegramCallData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIncomingTelegramCallData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = reader.string();
          break;
        case 2:
          message.CallId = reader.string();
          break;
        case 3:
          message.Caller = TProviderContactData.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIncomingTelegramCallData {
    return {
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      CallId: isSet(object.call_id) ? String(object.call_id) : "",
      Caller: isSet(object.caller)
        ? TProviderContactData.fromJSON(object.caller)
        : undefined,
    };
  },

  toJSON(message: TIncomingTelegramCallData): unknown {
    const obj: any = {};
    message.UserId !== undefined && (obj.user_id = message.UserId);
    message.CallId !== undefined && (obj.call_id = message.CallId);
    message.Caller !== undefined &&
      (obj.caller = message.Caller
        ? TProviderContactData.toJSON(message.Caller)
        : undefined);
    return obj;
  },
};

function createBaseTOutgoingTelegramCallData(): TOutgoingTelegramCallData {
  return { UserId: "", Recipient: undefined };
}

export const TOutgoingTelegramCallData = {
  encode(
    message: TOutgoingTelegramCallData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== "") {
      writer.uint32(10).string(message.UserId);
    }
    if (message.Recipient !== undefined) {
      TProviderContactData.encode(
        message.Recipient,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOutgoingTelegramCallData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOutgoingTelegramCallData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = reader.string();
          break;
        case 2:
          message.Recipient = TProviderContactData.decode(
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

  fromJSON(object: any): TOutgoingTelegramCallData {
    return {
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      Recipient: isSet(object.recipient)
        ? TProviderContactData.fromJSON(object.recipient)
        : undefined,
    };
  },

  toJSON(message: TOutgoingTelegramCallData): unknown {
    const obj: any = {};
    message.UserId !== undefined && (obj.user_id = message.UserId);
    message.Recipient !== undefined &&
      (obj.recipient = message.Recipient
        ? TProviderContactData.toJSON(message.Recipient)
        : undefined);
    return obj;
  },
};

function createBaseTCurrentTelegramCallData(): TCurrentTelegramCallData {
  return { UserId: "", CallId: "", Recipient: undefined };
}

export const TCurrentTelegramCallData = {
  encode(
    message: TCurrentTelegramCallData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== "") {
      writer.uint32(10).string(message.UserId);
    }
    if (message.CallId !== "") {
      writer.uint32(18).string(message.CallId);
    }
    if (message.Recipient !== undefined) {
      TProviderContactData.encode(
        message.Recipient,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCurrentTelegramCallData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCurrentTelegramCallData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = reader.string();
          break;
        case 2:
          message.CallId = reader.string();
          break;
        case 3:
          message.Recipient = TProviderContactData.decode(
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

  fromJSON(object: any): TCurrentTelegramCallData {
    return {
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      CallId: isSet(object.call_id) ? String(object.call_id) : "",
      Recipient: isSet(object.recipient)
        ? TProviderContactData.fromJSON(object.recipient)
        : undefined,
    };
  },

  toJSON(message: TCurrentTelegramCallData): unknown {
    const obj: any = {};
    message.UserId !== undefined && (obj.user_id = message.UserId);
    message.CallId !== undefined && (obj.call_id = message.CallId);
    message.Recipient !== undefined &&
      (obj.recipient = message.Recipient
        ? TProviderContactData.toJSON(message.Recipient)
        : undefined);
    return obj;
  },
};

function createBaseTProviderContactList(): TProviderContactList {
  return { ContactData: [] };
}

export const TProviderContactList = {
  encode(
    message: TProviderContactList,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.ContactData) {
      TProviderContactData.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TProviderContactList {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTProviderContactList();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContactData.push(
            TProviderContactData.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TProviderContactList {
    return {
      ContactData: Array.isArray(object?.contact_data)
        ? object.contact_data.map((e: any) => TProviderContactData.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TProviderContactList): unknown {
    const obj: any = {};
    if (message.ContactData) {
      obj.contact_data = message.ContactData.map((e) =>
        e ? TProviderContactData.toJSON(e) : undefined
      );
    } else {
      obj.contact_data = [];
    }
    return obj;
  },
};

function createBaseTProviderContactData(): TProviderContactData {
  return { TelegramContactData: undefined };
}

export const TProviderContactData = {
  encode(
    message: TProviderContactData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TelegramContactData !== undefined) {
      TProviderContactData_TTelegramContactData.encode(
        message.TelegramContactData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TProviderContactData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTProviderContactData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TelegramContactData =
            TProviderContactData_TTelegramContactData.decode(
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

  fromJSON(object: any): TProviderContactData {
    return {
      TelegramContactData: isSet(object.telegram_contact_data)
        ? TProviderContactData_TTelegramContactData.fromJSON(
            object.telegram_contact_data
          )
        : undefined,
    };
  },

  toJSON(message: TProviderContactData): unknown {
    const obj: any = {};
    message.TelegramContactData !== undefined &&
      (obj.telegram_contact_data = message.TelegramContactData
        ? TProviderContactData_TTelegramContactData.toJSON(
            message.TelegramContactData
          )
        : undefined);
    return obj;
  },
};

function createBaseTProviderContactData_TTelegramContactData(): TProviderContactData_TTelegramContactData {
  return { DisplayName: "", UserId: "" };
}

export const TProviderContactData_TTelegramContactData = {
  encode(
    message: TProviderContactData_TTelegramContactData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DisplayName !== "") {
      writer.uint32(10).string(message.DisplayName);
    }
    if (message.UserId !== "") {
      writer.uint32(18).string(message.UserId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TProviderContactData_TTelegramContactData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTProviderContactData_TTelegramContactData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DisplayName = reader.string();
          break;
        case 2:
          message.UserId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TProviderContactData_TTelegramContactData {
    return {
      DisplayName: isSet(object.display_name)
        ? String(object.display_name)
        : "",
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
    };
  },

  toJSON(message: TProviderContactData_TTelegramContactData): unknown {
    const obj: any = {};
    message.DisplayName !== undefined &&
      (obj.display_name = message.DisplayName);
    message.UserId !== undefined && (obj.user_id = message.UserId);
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
