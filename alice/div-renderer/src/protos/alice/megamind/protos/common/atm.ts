/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

/** These parameters identify the way of user interatction */
export interface TAnalyticsTrackingModule {
  ProductScenario: string;
  /** The origin of the request/event, like website or remote control */
  Origin: TAnalyticsTrackingModule_EOrigin;
  /**
   * The purpose identifies a user's intent during the action
   * Why the request was sent and what is expected in the response
   * Slug like get_weather, next_track, and so on
   */
  Purpose: string;
  /** Any extra information about the origin, like id of marketing campaigns */
  OriginInfo: string;
}

export enum TAnalyticsTrackingModule_EOrigin {
  Undefined = 0,
  Web = 1,
  Scenario = 2,
  /** @deprecated */
  SmartSpeaker = 3,
  RemoteControl = 4,
  Proactivity = 5,
  Cast = 6,
  Timetable = 7,
  Push = 8,
  /** @deprecated */
  SearchApp = 9,
  ThisClient = 10,
  SmartTv = 11,
  UNRECOGNIZED = -1,
}

export function tAnalyticsTrackingModule_EOriginFromJSON(
  object: any
): TAnalyticsTrackingModule_EOrigin {
  switch (object) {
    case 0:
    case "Undefined":
      return TAnalyticsTrackingModule_EOrigin.Undefined;
    case 1:
    case "Web":
      return TAnalyticsTrackingModule_EOrigin.Web;
    case 2:
    case "Scenario":
      return TAnalyticsTrackingModule_EOrigin.Scenario;
    case 3:
    case "SmartSpeaker":
      return TAnalyticsTrackingModule_EOrigin.SmartSpeaker;
    case 4:
    case "RemoteControl":
      return TAnalyticsTrackingModule_EOrigin.RemoteControl;
    case 5:
    case "Proactivity":
      return TAnalyticsTrackingModule_EOrigin.Proactivity;
    case 6:
    case "Cast":
      return TAnalyticsTrackingModule_EOrigin.Cast;
    case 7:
    case "Timetable":
      return TAnalyticsTrackingModule_EOrigin.Timetable;
    case 8:
    case "Push":
      return TAnalyticsTrackingModule_EOrigin.Push;
    case 9:
    case "SearchApp":
      return TAnalyticsTrackingModule_EOrigin.SearchApp;
    case 10:
    case "ThisClient":
      return TAnalyticsTrackingModule_EOrigin.ThisClient;
    case 11:
    case "SmartTv":
      return TAnalyticsTrackingModule_EOrigin.SmartTv;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnalyticsTrackingModule_EOrigin.UNRECOGNIZED;
  }
}

export function tAnalyticsTrackingModule_EOriginToJSON(
  object: TAnalyticsTrackingModule_EOrigin
): string {
  switch (object) {
    case TAnalyticsTrackingModule_EOrigin.Undefined:
      return "Undefined";
    case TAnalyticsTrackingModule_EOrigin.Web:
      return "Web";
    case TAnalyticsTrackingModule_EOrigin.Scenario:
      return "Scenario";
    case TAnalyticsTrackingModule_EOrigin.SmartSpeaker:
      return "SmartSpeaker";
    case TAnalyticsTrackingModule_EOrigin.RemoteControl:
      return "RemoteControl";
    case TAnalyticsTrackingModule_EOrigin.Proactivity:
      return "Proactivity";
    case TAnalyticsTrackingModule_EOrigin.Cast:
      return "Cast";
    case TAnalyticsTrackingModule_EOrigin.Timetable:
      return "Timetable";
    case TAnalyticsTrackingModule_EOrigin.Push:
      return "Push";
    case TAnalyticsTrackingModule_EOrigin.SearchApp:
      return "SearchApp";
    case TAnalyticsTrackingModule_EOrigin.ThisClient:
      return "ThisClient";
    case TAnalyticsTrackingModule_EOrigin.SmartTv:
      return "SmartTv";
    default:
      return "UNKNOWN";
  }
}

function createBaseTAnalyticsTrackingModule(): TAnalyticsTrackingModule {
  return { ProductScenario: "", Origin: 0, Purpose: "", OriginInfo: "" };
}

export const TAnalyticsTrackingModule = {
  encode(
    message: TAnalyticsTrackingModule,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ProductScenario !== "") {
      writer.uint32(10).string(message.ProductScenario);
    }
    if (message.Origin !== 0) {
      writer.uint32(16).int32(message.Origin);
    }
    if (message.Purpose !== "") {
      writer.uint32(26).string(message.Purpose);
    }
    if (message.OriginInfo !== "") {
      writer.uint32(34).string(message.OriginInfo);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnalyticsTrackingModule {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnalyticsTrackingModule();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ProductScenario = reader.string();
          break;
        case 2:
          message.Origin = reader.int32() as any;
          break;
        case 3:
          message.Purpose = reader.string();
          break;
        case 4:
          message.OriginInfo = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnalyticsTrackingModule {
    return {
      ProductScenario: isSet(object.product_scenario)
        ? String(object.product_scenario)
        : "",
      Origin: isSet(object.origin)
        ? tAnalyticsTrackingModule_EOriginFromJSON(object.origin)
        : 0,
      Purpose: isSet(object.purpose) ? String(object.purpose) : "",
      OriginInfo: isSet(object.origin_info) ? String(object.origin_info) : "",
    };
  },

  toJSON(message: TAnalyticsTrackingModule): unknown {
    const obj: any = {};
    message.ProductScenario !== undefined &&
      (obj.product_scenario = message.ProductScenario);
    message.Origin !== undefined &&
      (obj.origin = tAnalyticsTrackingModule_EOriginToJSON(message.Origin));
    message.Purpose !== undefined && (obj.purpose = message.Purpose);
    message.OriginInfo !== undefined && (obj.origin_info = message.OriginInfo);
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
