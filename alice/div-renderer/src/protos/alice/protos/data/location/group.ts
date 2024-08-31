/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import {
  EUserDeviceType,
  TUserSharingInfo,
  eUserDeviceTypeFromJSON,
  eUserDeviceTypeToJSON,
} from "../../../../alice/protos/data/device/info";

export const protobufPackage = "NAlice";

export interface TUserGroup {
  Id: string;
  Name: string;
  Type: EUserDeviceType;
  Aliases: string[];
  HouseholdId: string;
  AnalyticsType: string;
  AnalyticsName: string;
  SharingInfo?: TUserSharingInfo;
}

function createBaseTUserGroup(): TUserGroup {
  return {
    Id: "",
    Name: "",
    Type: 0,
    Aliases: [],
    HouseholdId: "",
    AnalyticsType: "",
    AnalyticsName: "",
    SharingInfo: undefined,
  };
}

export const TUserGroup = {
  encode(message: TUserGroup, writer: Writer = Writer.create()): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    if (message.Type !== 0) {
      writer.uint32(24).int32(message.Type);
    }
    for (const v of message.Aliases) {
      writer.uint32(34).string(v!);
    }
    if (message.HouseholdId !== "") {
      writer.uint32(42).string(message.HouseholdId);
    }
    if (message.AnalyticsType !== "") {
      writer.uint32(50).string(message.AnalyticsType);
    }
    if (message.AnalyticsName !== "") {
      writer.uint32(58).string(message.AnalyticsName);
    }
    if (message.SharingInfo !== undefined) {
      TUserSharingInfo.encode(
        message.SharingInfo,
        writer.uint32(66).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUserGroup {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUserGroup();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        case 3:
          message.Type = reader.int32() as any;
          break;
        case 4:
          message.Aliases.push(reader.string());
          break;
        case 5:
          message.HouseholdId = reader.string();
          break;
        case 6:
          message.AnalyticsType = reader.string();
          break;
        case 7:
          message.AnalyticsName = reader.string();
          break;
        case 8:
          message.SharingInfo = TUserSharingInfo.decode(
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

  fromJSON(object: any): TUserGroup {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      Type: isSet(object.type) ? eUserDeviceTypeFromJSON(object.type) : 0,
      Aliases: Array.isArray(object?.aliases)
        ? object.aliases.map((e: any) => String(e))
        : [],
      HouseholdId: isSet(object.household_id)
        ? String(object.household_id)
        : "",
      AnalyticsType: isSet(object.analytics_type)
        ? String(object.analytics_type)
        : "",
      AnalyticsName: isSet(object.analytics_name)
        ? String(object.analytics_name)
        : "",
      SharingInfo: isSet(object.sharing_info)
        ? TUserSharingInfo.fromJSON(object.sharing_info)
        : undefined,
    };
  },

  toJSON(message: TUserGroup): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    message.Type !== undefined &&
      (obj.type = eUserDeviceTypeToJSON(message.Type));
    if (message.Aliases) {
      obj.aliases = message.Aliases.map((e) => e);
    } else {
      obj.aliases = [];
    }
    message.HouseholdId !== undefined &&
      (obj.household_id = message.HouseholdId);
    message.AnalyticsType !== undefined &&
      (obj.analytics_type = message.AnalyticsType);
    message.AnalyticsName !== undefined &&
      (obj.analytics_name = message.AnalyticsName);
    message.SharingInfo !== undefined &&
      (obj.sharing_info = message.SharingInfo
        ? TUserSharingInfo.toJSON(message.SharingInfo)
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

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
