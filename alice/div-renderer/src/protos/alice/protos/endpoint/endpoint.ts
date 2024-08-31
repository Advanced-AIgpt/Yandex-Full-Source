/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { Timestamp } from "../../../google/protobuf/timestamp";
import { Any } from "../../../google/protobuf/any";

export const protobufPackage = "NAlice";

export interface TEndpoint {
  Id: string;
  Meta?: TEndpoint_TMeta;
  /**
   * Capability must be a member of NAlice.TCapabilityHolder message
   * from alice/protos/endpoint/capability.proto
   * but never TCapabilityHolder itself due to the fact that TCapabilityHolder depends on every known capability
   */
  Capabilities: Any[];
  Status?: TEndpoint_TStatus;
}

export enum TEndpoint_EEndpointStatus {
  Unknown = 0,
  Offline = 1,
  Online = 2,
  UNRECOGNIZED = -1,
}

export function tEndpoint_EEndpointStatusFromJSON(
  object: any
): TEndpoint_EEndpointStatus {
  switch (object) {
    case 0:
    case "Unknown":
      return TEndpoint_EEndpointStatus.Unknown;
    case 1:
    case "Offline":
      return TEndpoint_EEndpointStatus.Offline;
    case 2:
    case "Online":
      return TEndpoint_EEndpointStatus.Online;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TEndpoint_EEndpointStatus.UNRECOGNIZED;
  }
}

export function tEndpoint_EEndpointStatusToJSON(
  object: TEndpoint_EEndpointStatus
): string {
  switch (object) {
    case TEndpoint_EEndpointStatus.Unknown:
      return "Unknown";
    case TEndpoint_EEndpointStatus.Offline:
      return "Offline";
    case TEndpoint_EEndpointStatus.Online:
      return "Online";
    default:
      return "UNKNOWN";
  }
}

export enum TEndpoint_EEndpointType {
  UnknownEndpointType = 0,
  SpeakerEndpointType = 1,
  LightEndpointType = 2,
  SocketEndpointType = 3,
  SensorEndpointType = 4,
  WebOsTvEndpointType = 5,
  SwitchEndpointType = 6,
  WindowCoveringEndpointType = 7,
  DongleEndpointType = 8,
  UNRECOGNIZED = -1,
}

export function tEndpoint_EEndpointTypeFromJSON(
  object: any
): TEndpoint_EEndpointType {
  switch (object) {
    case 0:
    case "UnknownEndpointType":
      return TEndpoint_EEndpointType.UnknownEndpointType;
    case 1:
    case "SpeakerEndpointType":
      return TEndpoint_EEndpointType.SpeakerEndpointType;
    case 2:
    case "LightEndpointType":
      return TEndpoint_EEndpointType.LightEndpointType;
    case 3:
    case "SocketEndpointType":
      return TEndpoint_EEndpointType.SocketEndpointType;
    case 4:
    case "SensorEndpointType":
      return TEndpoint_EEndpointType.SensorEndpointType;
    case 5:
    case "WebOsTvEndpointType":
      return TEndpoint_EEndpointType.WebOsTvEndpointType;
    case 6:
    case "SwitchEndpointType":
      return TEndpoint_EEndpointType.SwitchEndpointType;
    case 7:
    case "WindowCoveringEndpointType":
      return TEndpoint_EEndpointType.WindowCoveringEndpointType;
    case 8:
    case "DongleEndpointType":
      return TEndpoint_EEndpointType.DongleEndpointType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TEndpoint_EEndpointType.UNRECOGNIZED;
  }
}

export function tEndpoint_EEndpointTypeToJSON(
  object: TEndpoint_EEndpointType
): string {
  switch (object) {
    case TEndpoint_EEndpointType.UnknownEndpointType:
      return "UnknownEndpointType";
    case TEndpoint_EEndpointType.SpeakerEndpointType:
      return "SpeakerEndpointType";
    case TEndpoint_EEndpointType.LightEndpointType:
      return "LightEndpointType";
    case TEndpoint_EEndpointType.SocketEndpointType:
      return "SocketEndpointType";
    case TEndpoint_EEndpointType.SensorEndpointType:
      return "SensorEndpointType";
    case TEndpoint_EEndpointType.WebOsTvEndpointType:
      return "WebOsTvEndpointType";
    case TEndpoint_EEndpointType.SwitchEndpointType:
      return "SwitchEndpointType";
    case TEndpoint_EEndpointType.WindowCoveringEndpointType:
      return "WindowCoveringEndpointType";
    case TEndpoint_EEndpointType.DongleEndpointType:
      return "DongleEndpointType";
    default:
      return "UNKNOWN";
  }
}

export interface TEndpoint_TMeta {
  Type: TEndpoint_EEndpointType;
  DeviceInfo?: TEndpoint_TDeviceInfo;
}

export interface TEndpoint_TStatus {
  Status: TEndpoint_EEndpointStatus;
  UpdatedAt?: Date;
}

export interface TEndpoint_TDeviceInfo {
  Manufacturer: string;
  Model: string;
  HwVersion: string;
  SwVersion: string;
}

function createBaseTEndpoint(): TEndpoint {
  return { Id: "", Meta: undefined, Capabilities: [], Status: undefined };
}

export const TEndpoint = {
  encode(message: TEndpoint, writer: Writer = Writer.create()): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Meta !== undefined) {
      TEndpoint_TMeta.encode(message.Meta, writer.uint32(18).fork()).ldelim();
    }
    for (const v of message.Capabilities) {
      Any.encode(v!, writer.uint32(26).fork()).ldelim();
    }
    if (message.Status !== undefined) {
      TEndpoint_TStatus.encode(
        message.Status,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEndpoint {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpoint();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Meta = TEndpoint_TMeta.decode(reader, reader.uint32());
          break;
        case 3:
          message.Capabilities.push(Any.decode(reader, reader.uint32()));
          break;
        case 4:
          message.Status = TEndpoint_TStatus.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEndpoint {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Meta: isSet(object.meta)
        ? TEndpoint_TMeta.fromJSON(object.meta)
        : undefined,
      Capabilities: Array.isArray(object?.capabilities)
        ? object.capabilities.map((e: any) => Any.fromJSON(e))
        : [],
      Status: isSet(object.status)
        ? TEndpoint_TStatus.fromJSON(object.status)
        : undefined,
    };
  },

  toJSON(message: TEndpoint): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TEndpoint_TMeta.toJSON(message.Meta)
        : undefined);
    if (message.Capabilities) {
      obj.capabilities = message.Capabilities.map((e) =>
        e ? Any.toJSON(e) : undefined
      );
    } else {
      obj.capabilities = [];
    }
    message.Status !== undefined &&
      (obj.status = message.Status
        ? TEndpoint_TStatus.toJSON(message.Status)
        : undefined);
    return obj;
  },
};

function createBaseTEndpoint_TMeta(): TEndpoint_TMeta {
  return { Type: 0, DeviceInfo: undefined };
}

export const TEndpoint_TMeta = {
  encode(message: TEndpoint_TMeta, writer: Writer = Writer.create()): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.DeviceInfo !== undefined) {
      TEndpoint_TDeviceInfo.encode(
        message.DeviceInfo,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEndpoint_TMeta {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpoint_TMeta();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.DeviceInfo = TEndpoint_TDeviceInfo.decode(
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

  fromJSON(object: any): TEndpoint_TMeta {
    return {
      Type: isSet(object.type)
        ? tEndpoint_EEndpointTypeFromJSON(object.type)
        : 0,
      DeviceInfo: isSet(object.device_info)
        ? TEndpoint_TDeviceInfo.fromJSON(object.device_info)
        : undefined,
    };
  },

  toJSON(message: TEndpoint_TMeta): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tEndpoint_EEndpointTypeToJSON(message.Type));
    message.DeviceInfo !== undefined &&
      (obj.device_info = message.DeviceInfo
        ? TEndpoint_TDeviceInfo.toJSON(message.DeviceInfo)
        : undefined);
    return obj;
  },
};

function createBaseTEndpoint_TStatus(): TEndpoint_TStatus {
  return { Status: 0, UpdatedAt: undefined };
}

export const TEndpoint_TStatus = {
  encode(message: TEndpoint_TStatus, writer: Writer = Writer.create()): Writer {
    if (message.Status !== 0) {
      writer.uint32(8).int32(message.Status);
    }
    if (message.UpdatedAt !== undefined) {
      Timestamp.encode(
        toTimestamp(message.UpdatedAt),
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEndpoint_TStatus {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpoint_TStatus();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Status = reader.int32() as any;
          break;
        case 2:
          message.UpdatedAt = fromTimestamp(
            Timestamp.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEndpoint_TStatus {
    return {
      Status: isSet(object.status)
        ? tEndpoint_EEndpointStatusFromJSON(object.status)
        : 0,
      UpdatedAt: isSet(object.updated_at)
        ? fromJsonTimestamp(object.updated_at)
        : undefined,
    };
  },

  toJSON(message: TEndpoint_TStatus): unknown {
    const obj: any = {};
    message.Status !== undefined &&
      (obj.status = tEndpoint_EEndpointStatusToJSON(message.Status));
    message.UpdatedAt !== undefined &&
      (obj.updated_at = message.UpdatedAt.toISOString());
    return obj;
  },
};

function createBaseTEndpoint_TDeviceInfo(): TEndpoint_TDeviceInfo {
  return { Manufacturer: "", Model: "", HwVersion: "", SwVersion: "" };
}

export const TEndpoint_TDeviceInfo = {
  encode(
    message: TEndpoint_TDeviceInfo,
    writer: Writer = Writer.create()
  ): Writer {
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

  decode(input: Reader | Uint8Array, length?: number): TEndpoint_TDeviceInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpoint_TDeviceInfo();
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

  fromJSON(object: any): TEndpoint_TDeviceInfo {
    return {
      Manufacturer: isSet(object.manufacturer)
        ? String(object.manufacturer)
        : "",
      Model: isSet(object.model) ? String(object.model) : "",
      HwVersion: isSet(object.hw_version) ? String(object.hw_version) : "",
      SwVersion: isSet(object.sw_version) ? String(object.sw_version) : "",
    };
  },

  toJSON(message: TEndpoint_TDeviceInfo): unknown {
    const obj: any = {};
    message.Manufacturer !== undefined &&
      (obj.manufacturer = message.Manufacturer);
    message.Model !== undefined && (obj.model = message.Model);
    message.HwVersion !== undefined && (obj.hw_version = message.HwVersion);
    message.SwVersion !== undefined && (obj.sw_version = message.SwVersion);
    return obj;
  },
};

function toTimestamp(date: Date): Timestamp {
  const seconds = date.getTime() / 1_000;
  const nanos = (date.getTime() % 1_000) * 1_000_000;
  return { seconds, nanos };
}

function fromTimestamp(t: Timestamp): Date {
  let millis = t.seconds * 1_000;
  millis += t.nanos / 1_000_000;
  return new Date(millis);
}

function fromJsonTimestamp(o: any): Date {
  if (o instanceof Date) {
    return o;
  } else if (typeof o === "string") {
    return new Date(o);
  } else {
    return fromTimestamp(Timestamp.fromJSON(o));
  }
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
