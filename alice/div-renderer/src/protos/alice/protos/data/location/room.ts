/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { TUserSharingInfo } from "../../../../alice/protos/data/device/info";

export const protobufPackage = "NAlice";

export interface TUserRoom {
  Id: string;
  Name: string;
  HouseholdId: string;
  SharingInfo?: TUserSharingInfo;
}

function createBaseTUserRoom(): TUserRoom {
  return { Id: "", Name: "", HouseholdId: "", SharingInfo: undefined };
}

export const TUserRoom = {
  encode(message: TUserRoom, writer: Writer = Writer.create()): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    if (message.HouseholdId !== "") {
      writer.uint32(26).string(message.HouseholdId);
    }
    if (message.SharingInfo !== undefined) {
      TUserSharingInfo.encode(
        message.SharingInfo,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUserRoom {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUserRoom();
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
          message.HouseholdId = reader.string();
          break;
        case 4:
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

  fromJSON(object: any): TUserRoom {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      HouseholdId: isSet(object.household_id)
        ? String(object.household_id)
        : "",
      SharingInfo: isSet(object.sharing_info)
        ? TUserSharingInfo.fromJSON(object.sharing_info)
        : undefined,
    };
  },

  toJSON(message: TUserRoom): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    message.HouseholdId !== undefined &&
      (obj.household_id = message.HouseholdId);
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
