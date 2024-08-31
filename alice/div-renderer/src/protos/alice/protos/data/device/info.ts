/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

export enum EUserDeviceType {
  UnknownDeviceType = 0,
  LightDeviceType = 1,
  SocketDeviceType = 2,
  SwitchDeviceType = 3,
  HubDeviceType = 4,
  PurifierDeviceType = 5,
  HumidifierDeviceType = 6,
  VacuumCleanerDeviceType = 7,
  CookingDeviceType = 8,
  KettleDeviceType = 9,
  CoffeeMakerDeviceType = 10,
  ThermostatDeviceType = 11,
  AcDeviceType = 12,
  MediaDeviceDeviceType = 13,
  TvDeviceDeviceType = 14,
  ReceiverDeviceType = 15,
  TvBoxDeviceType = 16,
  WashingMachineDeviceType = 17,
  OpenableDeviceType = 18,
  CurtainDeviceType = 19,
  SmartSpeakerDeviceType = 20,
  YandexStationDeviceType = 21,
  YandexStation2DeviceType = 22,
  YandexStationMiniDeviceType = 23,
  DexpSmartBoxDeviceType = 24,
  IrbisADeviceType = 25,
  ElariSmartBeatDeviceType = 26,
  LGXBoomDeviceType = 27,
  JetSmartMusicDeviceType = 28,
  PrestigioSmartMateDeviceType = 29,
  DigmaDiHomeDeviceType = 30,
  JBLLinkPortableDeviceType = 31,
  JBLLinkMusicDeviceType = 32,
  YandexModuleDeviceType = 33,
  RemoteCarDeviceType = 34,
  OtherDeviceType = 35,
  YandexStationMini2DeviceType = 36,
  DishwasherDeviceType = 37,
  MulticookerDeviceType = 38,
  RefrigeratorDeviceType = 39,
  FanDeviceType = 40,
  IronDeviceType = 41,
  SensorDeviceType = 42,
  YandexModule2DeviceType = 43,
  YandexStationMicroDeviceType = 44,
  PetFeederDeviceType = 45,
  YandexStationCentaurDeviceType = 46,
  LightCeilingDeviceType = 47,
  LightLampDeviceType = 48,
  LightStripDeviceType = 49,
  YandexStationMidiDeviceType = 50,
  YandexStationMini2NoClockDeviceType = 51,
  CameraDeviceType = 52,
  YandexStationChironDeviceType = 53,
  YandexStationPholDeviceType = 54,
  UNRECOGNIZED = -1,
}

export function eUserDeviceTypeFromJSON(object: any): EUserDeviceType {
  switch (object) {
    case 0:
    case "UnknownDeviceType":
      return EUserDeviceType.UnknownDeviceType;
    case 1:
    case "LightDeviceType":
      return EUserDeviceType.LightDeviceType;
    case 2:
    case "SocketDeviceType":
      return EUserDeviceType.SocketDeviceType;
    case 3:
    case "SwitchDeviceType":
      return EUserDeviceType.SwitchDeviceType;
    case 4:
    case "HubDeviceType":
      return EUserDeviceType.HubDeviceType;
    case 5:
    case "PurifierDeviceType":
      return EUserDeviceType.PurifierDeviceType;
    case 6:
    case "HumidifierDeviceType":
      return EUserDeviceType.HumidifierDeviceType;
    case 7:
    case "VacuumCleanerDeviceType":
      return EUserDeviceType.VacuumCleanerDeviceType;
    case 8:
    case "CookingDeviceType":
      return EUserDeviceType.CookingDeviceType;
    case 9:
    case "KettleDeviceType":
      return EUserDeviceType.KettleDeviceType;
    case 10:
    case "CoffeeMakerDeviceType":
      return EUserDeviceType.CoffeeMakerDeviceType;
    case 11:
    case "ThermostatDeviceType":
      return EUserDeviceType.ThermostatDeviceType;
    case 12:
    case "AcDeviceType":
      return EUserDeviceType.AcDeviceType;
    case 13:
    case "MediaDeviceDeviceType":
      return EUserDeviceType.MediaDeviceDeviceType;
    case 14:
    case "TvDeviceDeviceType":
      return EUserDeviceType.TvDeviceDeviceType;
    case 15:
    case "ReceiverDeviceType":
      return EUserDeviceType.ReceiverDeviceType;
    case 16:
    case "TvBoxDeviceType":
      return EUserDeviceType.TvBoxDeviceType;
    case 17:
    case "WashingMachineDeviceType":
      return EUserDeviceType.WashingMachineDeviceType;
    case 18:
    case "OpenableDeviceType":
      return EUserDeviceType.OpenableDeviceType;
    case 19:
    case "CurtainDeviceType":
      return EUserDeviceType.CurtainDeviceType;
    case 20:
    case "SmartSpeakerDeviceType":
      return EUserDeviceType.SmartSpeakerDeviceType;
    case 21:
    case "YandexStationDeviceType":
      return EUserDeviceType.YandexStationDeviceType;
    case 22:
    case "YandexStation2DeviceType":
      return EUserDeviceType.YandexStation2DeviceType;
    case 23:
    case "YandexStationMiniDeviceType":
      return EUserDeviceType.YandexStationMiniDeviceType;
    case 24:
    case "DexpSmartBoxDeviceType":
      return EUserDeviceType.DexpSmartBoxDeviceType;
    case 25:
    case "IrbisADeviceType":
      return EUserDeviceType.IrbisADeviceType;
    case 26:
    case "ElariSmartBeatDeviceType":
      return EUserDeviceType.ElariSmartBeatDeviceType;
    case 27:
    case "LGXBoomDeviceType":
      return EUserDeviceType.LGXBoomDeviceType;
    case 28:
    case "JetSmartMusicDeviceType":
      return EUserDeviceType.JetSmartMusicDeviceType;
    case 29:
    case "PrestigioSmartMateDeviceType":
      return EUserDeviceType.PrestigioSmartMateDeviceType;
    case 30:
    case "DigmaDiHomeDeviceType":
      return EUserDeviceType.DigmaDiHomeDeviceType;
    case 31:
    case "JBLLinkPortableDeviceType":
      return EUserDeviceType.JBLLinkPortableDeviceType;
    case 32:
    case "JBLLinkMusicDeviceType":
      return EUserDeviceType.JBLLinkMusicDeviceType;
    case 33:
    case "YandexModuleDeviceType":
      return EUserDeviceType.YandexModuleDeviceType;
    case 34:
    case "RemoteCarDeviceType":
      return EUserDeviceType.RemoteCarDeviceType;
    case 35:
    case "OtherDeviceType":
      return EUserDeviceType.OtherDeviceType;
    case 36:
    case "YandexStationMini2DeviceType":
      return EUserDeviceType.YandexStationMini2DeviceType;
    case 37:
    case "DishwasherDeviceType":
      return EUserDeviceType.DishwasherDeviceType;
    case 38:
    case "MulticookerDeviceType":
      return EUserDeviceType.MulticookerDeviceType;
    case 39:
    case "RefrigeratorDeviceType":
      return EUserDeviceType.RefrigeratorDeviceType;
    case 40:
    case "FanDeviceType":
      return EUserDeviceType.FanDeviceType;
    case 41:
    case "IronDeviceType":
      return EUserDeviceType.IronDeviceType;
    case 42:
    case "SensorDeviceType":
      return EUserDeviceType.SensorDeviceType;
    case 43:
    case "YandexModule2DeviceType":
      return EUserDeviceType.YandexModule2DeviceType;
    case 44:
    case "YandexStationMicroDeviceType":
      return EUserDeviceType.YandexStationMicroDeviceType;
    case 45:
    case "PetFeederDeviceType":
      return EUserDeviceType.PetFeederDeviceType;
    case 46:
    case "YandexStationCentaurDeviceType":
      return EUserDeviceType.YandexStationCentaurDeviceType;
    case 47:
    case "LightCeilingDeviceType":
      return EUserDeviceType.LightCeilingDeviceType;
    case 48:
    case "LightLampDeviceType":
      return EUserDeviceType.LightLampDeviceType;
    case 49:
    case "LightStripDeviceType":
      return EUserDeviceType.LightStripDeviceType;
    case 50:
    case "YandexStationMidiDeviceType":
      return EUserDeviceType.YandexStationMidiDeviceType;
    case 51:
    case "YandexStationMini2NoClockDeviceType":
      return EUserDeviceType.YandexStationMini2NoClockDeviceType;
    case 52:
    case "CameraDeviceType":
      return EUserDeviceType.CameraDeviceType;
    case 53:
    case "YandexStationChironDeviceType":
      return EUserDeviceType.YandexStationChironDeviceType;
    case 54:
    case "YandexStationPholDeviceType":
      return EUserDeviceType.YandexStationPholDeviceType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EUserDeviceType.UNRECOGNIZED;
  }
}

export function eUserDeviceTypeToJSON(object: EUserDeviceType): string {
  switch (object) {
    case EUserDeviceType.UnknownDeviceType:
      return "UnknownDeviceType";
    case EUserDeviceType.LightDeviceType:
      return "LightDeviceType";
    case EUserDeviceType.SocketDeviceType:
      return "SocketDeviceType";
    case EUserDeviceType.SwitchDeviceType:
      return "SwitchDeviceType";
    case EUserDeviceType.HubDeviceType:
      return "HubDeviceType";
    case EUserDeviceType.PurifierDeviceType:
      return "PurifierDeviceType";
    case EUserDeviceType.HumidifierDeviceType:
      return "HumidifierDeviceType";
    case EUserDeviceType.VacuumCleanerDeviceType:
      return "VacuumCleanerDeviceType";
    case EUserDeviceType.CookingDeviceType:
      return "CookingDeviceType";
    case EUserDeviceType.KettleDeviceType:
      return "KettleDeviceType";
    case EUserDeviceType.CoffeeMakerDeviceType:
      return "CoffeeMakerDeviceType";
    case EUserDeviceType.ThermostatDeviceType:
      return "ThermostatDeviceType";
    case EUserDeviceType.AcDeviceType:
      return "AcDeviceType";
    case EUserDeviceType.MediaDeviceDeviceType:
      return "MediaDeviceDeviceType";
    case EUserDeviceType.TvDeviceDeviceType:
      return "TvDeviceDeviceType";
    case EUserDeviceType.ReceiverDeviceType:
      return "ReceiverDeviceType";
    case EUserDeviceType.TvBoxDeviceType:
      return "TvBoxDeviceType";
    case EUserDeviceType.WashingMachineDeviceType:
      return "WashingMachineDeviceType";
    case EUserDeviceType.OpenableDeviceType:
      return "OpenableDeviceType";
    case EUserDeviceType.CurtainDeviceType:
      return "CurtainDeviceType";
    case EUserDeviceType.SmartSpeakerDeviceType:
      return "SmartSpeakerDeviceType";
    case EUserDeviceType.YandexStationDeviceType:
      return "YandexStationDeviceType";
    case EUserDeviceType.YandexStation2DeviceType:
      return "YandexStation2DeviceType";
    case EUserDeviceType.YandexStationMiniDeviceType:
      return "YandexStationMiniDeviceType";
    case EUserDeviceType.DexpSmartBoxDeviceType:
      return "DexpSmartBoxDeviceType";
    case EUserDeviceType.IrbisADeviceType:
      return "IrbisADeviceType";
    case EUserDeviceType.ElariSmartBeatDeviceType:
      return "ElariSmartBeatDeviceType";
    case EUserDeviceType.LGXBoomDeviceType:
      return "LGXBoomDeviceType";
    case EUserDeviceType.JetSmartMusicDeviceType:
      return "JetSmartMusicDeviceType";
    case EUserDeviceType.PrestigioSmartMateDeviceType:
      return "PrestigioSmartMateDeviceType";
    case EUserDeviceType.DigmaDiHomeDeviceType:
      return "DigmaDiHomeDeviceType";
    case EUserDeviceType.JBLLinkPortableDeviceType:
      return "JBLLinkPortableDeviceType";
    case EUserDeviceType.JBLLinkMusicDeviceType:
      return "JBLLinkMusicDeviceType";
    case EUserDeviceType.YandexModuleDeviceType:
      return "YandexModuleDeviceType";
    case EUserDeviceType.RemoteCarDeviceType:
      return "RemoteCarDeviceType";
    case EUserDeviceType.OtherDeviceType:
      return "OtherDeviceType";
    case EUserDeviceType.YandexStationMini2DeviceType:
      return "YandexStationMini2DeviceType";
    case EUserDeviceType.DishwasherDeviceType:
      return "DishwasherDeviceType";
    case EUserDeviceType.MulticookerDeviceType:
      return "MulticookerDeviceType";
    case EUserDeviceType.RefrigeratorDeviceType:
      return "RefrigeratorDeviceType";
    case EUserDeviceType.FanDeviceType:
      return "FanDeviceType";
    case EUserDeviceType.IronDeviceType:
      return "IronDeviceType";
    case EUserDeviceType.SensorDeviceType:
      return "SensorDeviceType";
    case EUserDeviceType.YandexModule2DeviceType:
      return "YandexModule2DeviceType";
    case EUserDeviceType.YandexStationMicroDeviceType:
      return "YandexStationMicroDeviceType";
    case EUserDeviceType.PetFeederDeviceType:
      return "PetFeederDeviceType";
    case EUserDeviceType.YandexStationCentaurDeviceType:
      return "YandexStationCentaurDeviceType";
    case EUserDeviceType.LightCeilingDeviceType:
      return "LightCeilingDeviceType";
    case EUserDeviceType.LightLampDeviceType:
      return "LightLampDeviceType";
    case EUserDeviceType.LightStripDeviceType:
      return "LightStripDeviceType";
    case EUserDeviceType.YandexStationMidiDeviceType:
      return "YandexStationMidiDeviceType";
    case EUserDeviceType.YandexStationMini2NoClockDeviceType:
      return "YandexStationMini2NoClockDeviceType";
    case EUserDeviceType.CameraDeviceType:
      return "CameraDeviceType";
    case EUserDeviceType.YandexStationChironDeviceType:
      return "YandexStationChironDeviceType";
    case EUserDeviceType.YandexStationPholDeviceType:
      return "YandexStationPholDeviceType";
    default:
      return "UNKNOWN";
  }
}

export interface TUserDeviceInfo {
  Manufacturer: string;
  Model: string;
  HwVersion: string;
  SwVersion: string;
}

export interface TUserQuasarInfo {
  DeviceId: string;
  Platform: string;
}

export interface TUserSharingInfo {
  OwnerID: number;
  HouseholdID: string;
}

function createBaseTUserDeviceInfo(): TUserDeviceInfo {
  return { Manufacturer: "", Model: "", HwVersion: "", SwVersion: "" };
}

export const TUserDeviceInfo = {
  encode(message: TUserDeviceInfo, writer: Writer = Writer.create()): Writer {
    if (message.Manufacturer !== "") {
      writer.uint32(10).string(message.Manufacturer);
    }
    if (message.Model !== "") {
      writer.uint32(18).string(message.Model);
    }
    if (message.HwVersion !== "") {
      writer.uint32(26).string(message.HwVersion);
    }
    if (message.SwVersion !== "") {
      writer.uint32(34).string(message.SwVersion);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUserDeviceInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUserDeviceInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Manufacturer = reader.string();
          break;
        case 2:
          message.Model = reader.string();
          break;
        case 3:
          message.HwVersion = reader.string();
          break;
        case 4:
          message.SwVersion = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUserDeviceInfo {
    return {
      Manufacturer: isSet(object.manufacturer)
        ? String(object.manufacturer)
        : "",
      Model: isSet(object.model) ? String(object.model) : "",
      HwVersion: isSet(object.hw_version) ? String(object.hw_version) : "",
      SwVersion: isSet(object.sw_version) ? String(object.sw_version) : "",
    };
  },

  toJSON(message: TUserDeviceInfo): unknown {
    const obj: any = {};
    message.Manufacturer !== undefined &&
      (obj.manufacturer = message.Manufacturer);
    message.Model !== undefined && (obj.model = message.Model);
    message.HwVersion !== undefined && (obj.hw_version = message.HwVersion);
    message.SwVersion !== undefined && (obj.sw_version = message.SwVersion);
    return obj;
  },
};

function createBaseTUserQuasarInfo(): TUserQuasarInfo {
  return { DeviceId: "", Platform: "" };
}

export const TUserQuasarInfo = {
  encode(message: TUserQuasarInfo, writer: Writer = Writer.create()): Writer {
    if (message.DeviceId !== "") {
      writer.uint32(10).string(message.DeviceId);
    }
    if (message.Platform !== "") {
      writer.uint32(18).string(message.Platform);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUserQuasarInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUserQuasarInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceId = reader.string();
          break;
        case 2:
          message.Platform = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUserQuasarInfo {
    return {
      DeviceId: isSet(object.device_id) ? String(object.device_id) : "",
      Platform: isSet(object.platform) ? String(object.platform) : "",
    };
  },

  toJSON(message: TUserQuasarInfo): unknown {
    const obj: any = {};
    message.DeviceId !== undefined && (obj.device_id = message.DeviceId);
    message.Platform !== undefined && (obj.platform = message.Platform);
    return obj;
  },
};

function createBaseTUserSharingInfo(): TUserSharingInfo {
  return { OwnerID: 0, HouseholdID: "" };
}

export const TUserSharingInfo = {
  encode(message: TUserSharingInfo, writer: Writer = Writer.create()): Writer {
    if (message.OwnerID !== 0) {
      writer.uint32(8).uint64(message.OwnerID);
    }
    if (message.HouseholdID !== "") {
      writer.uint32(18).string(message.HouseholdID);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUserSharingInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUserSharingInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.OwnerID = longToNumber(reader.uint64() as Long);
          break;
        case 2:
          message.HouseholdID = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUserSharingInfo {
    return {
      OwnerID: isSet(object.owner_id) ? Number(object.owner_id) : 0,
      HouseholdID: isSet(object.household_id)
        ? String(object.household_id)
        : "",
    };
  },

  toJSON(message: TUserSharingInfo): unknown {
    const obj: any = {};
    message.OwnerID !== undefined &&
      (obj.owner_id = Math.round(message.OwnerID));
    message.HouseholdID !== undefined &&
      (obj.household_id = message.HouseholdID);
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
