/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TContactsList {
  Contacts: TContactsList_TContact[];
  Phones: TContactsList_TPhone[];
  IsKnownUuid: boolean;
  Deleted: boolean;
  Truncated: boolean;
  LookupKeyMapSerialized: Uint8Array;
}

/**
 * Contacts with multiple external accounts (phone numbers, telegram accounts, etc)
 * have multiple instances of this structure with the same ContactId and LookupKey
 */
export interface TContactsList_TContact {
  /**
   * Phone number or account name in the external messenger app
   * WhatsApp is special and uses a constant string here, to find out the actual account name,
   * find the phone number contact with the same LookupKey and use the phone number for that.
   */
  AccountName: string;
  /** Examples: com.google, com.viber.voip, org.telegram.messenger, com.whatsapp */
  AccountType: string;
  DisplayName: string;
  FirstName: string;
  MiddleName: string;
  SecondName: string;
  /**
   * Not unique
   * Might change over time, use LookupKey instead
   */
  ContactId: number;
  /** Unique id of the item */
  Id: number;
  LookupKey: string;
  LastTimeContacted: number;
  TimesContacted: number;
  LookupKeyIndex: number;
}

export interface TContactsList_TPhone {
  Id: number;
  AccountType: string;
  LookupKey: string;
  Phone: string;
  /** Examples: mobile, home, work, unknown */
  Type: string;
  IdString: string;
  LookupKeyIndex: number;
}

export interface TUpdateContactsRequest {
  CreatedContacts: TUpdateContactsRequest_TContactInfo[];
  UpdatedContacts: TUpdateContactsRequest_TContactInfo[];
  RemovedContacts: TUpdateContactsRequest_TContactInfo[];
}

export interface TUpdateContactsRequest_TTelegramContactInfo {
  UserId: string;
  Provider: string;
  ContactId: string;
  FirstName: string;
  SecondName: string;
}

export interface TUpdateContactsRequest_TContactInfo {
  TelegramContactInfo?: TUpdateContactsRequest_TTelegramContactInfo | undefined;
}

function createBaseTContactsList(): TContactsList {
  return {
    Contacts: [],
    Phones: [],
    IsKnownUuid: false,
    Deleted: false,
    Truncated: false,
    LookupKeyMapSerialized: new Uint8Array(),
  };
}

export const TContactsList = {
  encode(message: TContactsList, writer: Writer = Writer.create()): Writer {
    for (const v of message.Contacts) {
      TContactsList_TContact.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    for (const v of message.Phones) {
      TContactsList_TPhone.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    if (message.IsKnownUuid === true) {
      writer.uint32(24).bool(message.IsKnownUuid);
    }
    if (message.Deleted === true) {
      writer.uint32(32).bool(message.Deleted);
    }
    if (message.Truncated === true) {
      writer.uint32(40).bool(message.Truncated);
    }
    if (message.LookupKeyMapSerialized.length !== 0) {
      writer.uint32(50).bytes(message.LookupKeyMapSerialized);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TContactsList {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTContactsList();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Contacts.push(
            TContactsList_TContact.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.Phones.push(
            TContactsList_TPhone.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.IsKnownUuid = reader.bool();
          break;
        case 4:
          message.Deleted = reader.bool();
          break;
        case 5:
          message.Truncated = reader.bool();
          break;
        case 6:
          message.LookupKeyMapSerialized = reader.bytes();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TContactsList {
    return {
      Contacts: Array.isArray(object?.contacts)
        ? object.contacts.map((e: any) => TContactsList_TContact.fromJSON(e))
        : [],
      Phones: Array.isArray(object?.phones)
        ? object.phones.map((e: any) => TContactsList_TPhone.fromJSON(e))
        : [],
      IsKnownUuid: isSet(object.is_known_uuid)
        ? Boolean(object.is_known_uuid)
        : false,
      Deleted: isSet(object.deleted) ? Boolean(object.deleted) : false,
      Truncated: isSet(object.truncated) ? Boolean(object.truncated) : false,
      LookupKeyMapSerialized: isSet(object.lookup_key_map_serialized)
        ? bytesFromBase64(object.lookup_key_map_serialized)
        : new Uint8Array(),
    };
  },

  toJSON(message: TContactsList): unknown {
    const obj: any = {};
    if (message.Contacts) {
      obj.contacts = message.Contacts.map((e) =>
        e ? TContactsList_TContact.toJSON(e) : undefined
      );
    } else {
      obj.contacts = [];
    }
    if (message.Phones) {
      obj.phones = message.Phones.map((e) =>
        e ? TContactsList_TPhone.toJSON(e) : undefined
      );
    } else {
      obj.phones = [];
    }
    message.IsKnownUuid !== undefined &&
      (obj.is_known_uuid = message.IsKnownUuid);
    message.Deleted !== undefined && (obj.deleted = message.Deleted);
    message.Truncated !== undefined && (obj.truncated = message.Truncated);
    message.LookupKeyMapSerialized !== undefined &&
      (obj.lookup_key_map_serialized = base64FromBytes(
        message.LookupKeyMapSerialized !== undefined
          ? message.LookupKeyMapSerialized
          : new Uint8Array()
      ));
    return obj;
  },
};

function createBaseTContactsList_TContact(): TContactsList_TContact {
  return {
    AccountName: "",
    AccountType: "",
    DisplayName: "",
    FirstName: "",
    MiddleName: "",
    SecondName: "",
    ContactId: 0,
    Id: 0,
    LookupKey: "",
    LastTimeContacted: 0,
    TimesContacted: 0,
    LookupKeyIndex: 0,
  };
}

export const TContactsList_TContact = {
  encode(
    message: TContactsList_TContact,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.AccountName !== "") {
      writer.uint32(10).string(message.AccountName);
    }
    if (message.AccountType !== "") {
      writer.uint32(18).string(message.AccountType);
    }
    if (message.DisplayName !== "") {
      writer.uint32(26).string(message.DisplayName);
    }
    if (message.FirstName !== "") {
      writer.uint32(34).string(message.FirstName);
    }
    if (message.MiddleName !== "") {
      writer.uint32(42).string(message.MiddleName);
    }
    if (message.SecondName !== "") {
      writer.uint32(50).string(message.SecondName);
    }
    if (message.ContactId !== 0) {
      writer.uint32(56).int64(message.ContactId);
    }
    if (message.Id !== 0) {
      writer.uint32(64).int32(message.Id);
    }
    if (message.LookupKey !== "") {
      writer.uint32(74).string(message.LookupKey);
    }
    if (message.LastTimeContacted !== 0) {
      writer.uint32(80).uint64(message.LastTimeContacted);
    }
    if (message.TimesContacted !== 0) {
      writer.uint32(88).uint32(message.TimesContacted);
    }
    if (message.LookupKeyIndex !== 0) {
      writer.uint32(96).uint32(message.LookupKeyIndex);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TContactsList_TContact {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTContactsList_TContact();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AccountName = reader.string();
          break;
        case 2:
          message.AccountType = reader.string();
          break;
        case 3:
          message.DisplayName = reader.string();
          break;
        case 4:
          message.FirstName = reader.string();
          break;
        case 5:
          message.MiddleName = reader.string();
          break;
        case 6:
          message.SecondName = reader.string();
          break;
        case 7:
          message.ContactId = longToNumber(reader.int64() as Long);
          break;
        case 8:
          message.Id = reader.int32();
          break;
        case 9:
          message.LookupKey = reader.string();
          break;
        case 10:
          message.LastTimeContacted = longToNumber(reader.uint64() as Long);
          break;
        case 11:
          message.TimesContacted = reader.uint32();
          break;
        case 12:
          message.LookupKeyIndex = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TContactsList_TContact {
    return {
      AccountName: isSet(object.account_name)
        ? String(object.account_name)
        : "",
      AccountType: isSet(object.account_type)
        ? String(object.account_type)
        : "",
      DisplayName: isSet(object.display_name)
        ? String(object.display_name)
        : "",
      FirstName: isSet(object.first_name) ? String(object.first_name) : "",
      MiddleName: isSet(object.middle_name) ? String(object.middle_name) : "",
      SecondName: isSet(object.second_name) ? String(object.second_name) : "",
      ContactId: isSet(object.contact_id) ? Number(object.contact_id) : 0,
      Id: isSet(object._id) ? Number(object._id) : 0,
      LookupKey: isSet(object.lookup_key) ? String(object.lookup_key) : "",
      LastTimeContacted: isSet(object.last_time_contacted)
        ? Number(object.last_time_contacted)
        : 0,
      TimesContacted: isSet(object.times_contacted)
        ? Number(object.times_contacted)
        : 0,
      LookupKeyIndex: isSet(object.lookup_key_index)
        ? Number(object.lookup_key_index)
        : 0,
    };
  },

  toJSON(message: TContactsList_TContact): unknown {
    const obj: any = {};
    message.AccountName !== undefined &&
      (obj.account_name = message.AccountName);
    message.AccountType !== undefined &&
      (obj.account_type = message.AccountType);
    message.DisplayName !== undefined &&
      (obj.display_name = message.DisplayName);
    message.FirstName !== undefined && (obj.first_name = message.FirstName);
    message.MiddleName !== undefined && (obj.middle_name = message.MiddleName);
    message.SecondName !== undefined && (obj.second_name = message.SecondName);
    message.ContactId !== undefined &&
      (obj.contact_id = Math.round(message.ContactId));
    message.Id !== undefined && (obj._id = Math.round(message.Id));
    message.LookupKey !== undefined && (obj.lookup_key = message.LookupKey);
    message.LastTimeContacted !== undefined &&
      (obj.last_time_contacted = Math.round(message.LastTimeContacted));
    message.TimesContacted !== undefined &&
      (obj.times_contacted = Math.round(message.TimesContacted));
    message.LookupKeyIndex !== undefined &&
      (obj.lookup_key_index = Math.round(message.LookupKeyIndex));
    return obj;
  },
};

function createBaseTContactsList_TPhone(): TContactsList_TPhone {
  return {
    Id: 0,
    AccountType: "",
    LookupKey: "",
    Phone: "",
    Type: "",
    IdString: "",
    LookupKeyIndex: 0,
  };
}

export const TContactsList_TPhone = {
  encode(
    message: TContactsList_TPhone,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== 0) {
      writer.uint32(8).int32(message.Id);
    }
    if (message.AccountType !== "") {
      writer.uint32(18).string(message.AccountType);
    }
    if (message.LookupKey !== "") {
      writer.uint32(26).string(message.LookupKey);
    }
    if (message.Phone !== "") {
      writer.uint32(34).string(message.Phone);
    }
    if (message.Type !== "") {
      writer.uint32(42).string(message.Type);
    }
    if (message.IdString !== "") {
      writer.uint32(50).string(message.IdString);
    }
    if (message.LookupKeyIndex !== 0) {
      writer.uint32(56).uint32(message.LookupKeyIndex);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TContactsList_TPhone {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTContactsList_TPhone();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.int32();
          break;
        case 2:
          message.AccountType = reader.string();
          break;
        case 3:
          message.LookupKey = reader.string();
          break;
        case 4:
          message.Phone = reader.string();
          break;
        case 5:
          message.Type = reader.string();
          break;
        case 6:
          message.IdString = reader.string();
          break;
        case 7:
          message.LookupKeyIndex = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TContactsList_TPhone {
    return {
      Id: isSet(object._id) ? Number(object._id) : 0,
      AccountType: isSet(object.account_type)
        ? String(object.account_type)
        : "",
      LookupKey: isSet(object.lookup_key) ? String(object.lookup_key) : "",
      Phone: isSet(object.phone) ? String(object.phone) : "",
      Type: isSet(object.type) ? String(object.type) : "",
      IdString: isSet(object._id_string) ? String(object._id_string) : "",
      LookupKeyIndex: isSet(object.lookup_key_index)
        ? Number(object.lookup_key_index)
        : 0,
    };
  },

  toJSON(message: TContactsList_TPhone): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj._id = Math.round(message.Id));
    message.AccountType !== undefined &&
      (obj.account_type = message.AccountType);
    message.LookupKey !== undefined && (obj.lookup_key = message.LookupKey);
    message.Phone !== undefined && (obj.phone = message.Phone);
    message.Type !== undefined && (obj.type = message.Type);
    message.IdString !== undefined && (obj._id_string = message.IdString);
    message.LookupKeyIndex !== undefined &&
      (obj.lookup_key_index = Math.round(message.LookupKeyIndex));
    return obj;
  },
};

function createBaseTUpdateContactsRequest(): TUpdateContactsRequest {
  return { CreatedContacts: [], UpdatedContacts: [], RemovedContacts: [] };
}

export const TUpdateContactsRequest = {
  encode(
    message: TUpdateContactsRequest,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.CreatedContacts) {
      TUpdateContactsRequest_TContactInfo.encode(
        v!,
        writer.uint32(58).fork()
      ).ldelim();
    }
    for (const v of message.UpdatedContacts) {
      TUpdateContactsRequest_TContactInfo.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    for (const v of message.RemovedContacts) {
      TUpdateContactsRequest_TContactInfo.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUpdateContactsRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUpdateContactsRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 7:
          message.CreatedContacts.push(
            TUpdateContactsRequest_TContactInfo.decode(reader, reader.uint32())
          );
          break;
        case 1:
          message.UpdatedContacts.push(
            TUpdateContactsRequest_TContactInfo.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.RemovedContacts.push(
            TUpdateContactsRequest_TContactInfo.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUpdateContactsRequest {
    return {
      CreatedContacts: Array.isArray(object?.created_contacts)
        ? object.created_contacts.map((e: any) =>
            TUpdateContactsRequest_TContactInfo.fromJSON(e)
          )
        : [],
      UpdatedContacts: Array.isArray(object?.updated_contacts)
        ? object.updated_contacts.map((e: any) =>
            TUpdateContactsRequest_TContactInfo.fromJSON(e)
          )
        : [],
      RemovedContacts: Array.isArray(object?.removed_contacts)
        ? object.removed_contacts.map((e: any) =>
            TUpdateContactsRequest_TContactInfo.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TUpdateContactsRequest): unknown {
    const obj: any = {};
    if (message.CreatedContacts) {
      obj.created_contacts = message.CreatedContacts.map((e) =>
        e ? TUpdateContactsRequest_TContactInfo.toJSON(e) : undefined
      );
    } else {
      obj.created_contacts = [];
    }
    if (message.UpdatedContacts) {
      obj.updated_contacts = message.UpdatedContacts.map((e) =>
        e ? TUpdateContactsRequest_TContactInfo.toJSON(e) : undefined
      );
    } else {
      obj.updated_contacts = [];
    }
    if (message.RemovedContacts) {
      obj.removed_contacts = message.RemovedContacts.map((e) =>
        e ? TUpdateContactsRequest_TContactInfo.toJSON(e) : undefined
      );
    } else {
      obj.removed_contacts = [];
    }
    return obj;
  },
};

function createBaseTUpdateContactsRequest_TTelegramContactInfo(): TUpdateContactsRequest_TTelegramContactInfo {
  return {
    UserId: "",
    Provider: "",
    ContactId: "",
    FirstName: "",
    SecondName: "",
  };
}

export const TUpdateContactsRequest_TTelegramContactInfo = {
  encode(
    message: TUpdateContactsRequest_TTelegramContactInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== "") {
      writer.uint32(10).string(message.UserId);
    }
    if (message.Provider !== "") {
      writer.uint32(18).string(message.Provider);
    }
    if (message.ContactId !== "") {
      writer.uint32(26).string(message.ContactId);
    }
    if (message.FirstName !== "") {
      writer.uint32(34).string(message.FirstName);
    }
    if (message.SecondName !== "") {
      writer.uint32(50).string(message.SecondName);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TUpdateContactsRequest_TTelegramContactInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUpdateContactsRequest_TTelegramContactInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = reader.string();
          break;
        case 2:
          message.Provider = reader.string();
          break;
        case 3:
          message.ContactId = reader.string();
          break;
        case 4:
          message.FirstName = reader.string();
          break;
        case 6:
          message.SecondName = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUpdateContactsRequest_TTelegramContactInfo {
    return {
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      Provider: isSet(object.provider) ? String(object.provider) : "",
      ContactId: isSet(object.contact_id) ? String(object.contact_id) : "",
      FirstName: isSet(object.first_name) ? String(object.first_name) : "",
      SecondName: isSet(object.second_name) ? String(object.second_name) : "",
    };
  },

  toJSON(message: TUpdateContactsRequest_TTelegramContactInfo): unknown {
    const obj: any = {};
    message.UserId !== undefined && (obj.user_id = message.UserId);
    message.Provider !== undefined && (obj.provider = message.Provider);
    message.ContactId !== undefined && (obj.contact_id = message.ContactId);
    message.FirstName !== undefined && (obj.first_name = message.FirstName);
    message.SecondName !== undefined && (obj.second_name = message.SecondName);
    return obj;
  },
};

function createBaseTUpdateContactsRequest_TContactInfo(): TUpdateContactsRequest_TContactInfo {
  return { TelegramContactInfo: undefined };
}

export const TUpdateContactsRequest_TContactInfo = {
  encode(
    message: TUpdateContactsRequest_TContactInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TelegramContactInfo !== undefined) {
      TUpdateContactsRequest_TTelegramContactInfo.encode(
        message.TelegramContactInfo,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TUpdateContactsRequest_TContactInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUpdateContactsRequest_TContactInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TelegramContactInfo =
            TUpdateContactsRequest_TTelegramContactInfo.decode(
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

  fromJSON(object: any): TUpdateContactsRequest_TContactInfo {
    return {
      TelegramContactInfo: isSet(object.telegram_contact_info)
        ? TUpdateContactsRequest_TTelegramContactInfo.fromJSON(
            object.telegram_contact_info
          )
        : undefined,
    };
  },

  toJSON(message: TUpdateContactsRequest_TContactInfo): unknown {
    const obj: any = {};
    message.TelegramContactInfo !== undefined &&
      (obj.telegram_contact_info = message.TelegramContactInfo
        ? TUpdateContactsRequest_TTelegramContactInfo.toJSON(
            message.TelegramContactInfo
          )
        : undefined);
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

const atob: (b64: string) => string =
  globalThis.atob ||
  ((b64) => globalThis.Buffer.from(b64, "base64").toString("binary"));
function bytesFromBase64(b64: string): Uint8Array {
  const bin = atob(b64);
  const arr = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; ++i) {
    arr[i] = bin.charCodeAt(i);
  }
  return arr;
}

const btoa: (bin: string) => string =
  globalThis.btoa ||
  ((bin) => globalThis.Buffer.from(bin, "binary").toString("base64"));
function base64FromBytes(arr: Uint8Array): string {
  const bin: string[] = [];
  for (const byte of arr) {
    bin.push(String.fromCharCode(byte));
  }
  return btoa(bin.join(""));
}

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
