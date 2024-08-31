/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData.NIoT";

export interface TEnrollmentStatus {
  Success: boolean;
  FailureReason: TEnrollmentStatus_EFailureReason;
  FailureReasonDetails: string;
}

export enum TEnrollmentStatus_EFailureReason {
  Unknown = 0,
  /** ClientTimeout - Elapsed enrollment TTL */
  ClientTimeout = 1,
  /** ScenarioError - Got error in Voiceprint scenario */
  ScenarioError = 2,
  /** PassportError - Got error, resolving token in passport */
  PassportError = 3,
  /** RequestedByUser - Received multiaccount_remove_account_directive */
  RequestedByUser = 4,
  UNRECOGNIZED = -1,
}

export function tEnrollmentStatus_EFailureReasonFromJSON(
  object: any
): TEnrollmentStatus_EFailureReason {
  switch (object) {
    case 0:
    case "Unknown":
      return TEnrollmentStatus_EFailureReason.Unknown;
    case 1:
    case "ClientTimeout":
      return TEnrollmentStatus_EFailureReason.ClientTimeout;
    case 2:
    case "ScenarioError":
      return TEnrollmentStatus_EFailureReason.ScenarioError;
    case 3:
    case "PassportError":
      return TEnrollmentStatus_EFailureReason.PassportError;
    case 4:
    case "RequestedByUser":
      return TEnrollmentStatus_EFailureReason.RequestedByUser;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TEnrollmentStatus_EFailureReason.UNRECOGNIZED;
  }
}

export function tEnrollmentStatus_EFailureReasonToJSON(
  object: TEnrollmentStatus_EFailureReason
): string {
  switch (object) {
    case TEnrollmentStatus_EFailureReason.Unknown:
      return "Unknown";
    case TEnrollmentStatus_EFailureReason.ClientTimeout:
      return "ClientTimeout";
    case TEnrollmentStatus_EFailureReason.ScenarioError:
      return "ScenarioError";
    case TEnrollmentStatus_EFailureReason.PassportError:
      return "PassportError";
    case TEnrollmentStatus_EFailureReason.RequestedByUser:
      return "RequestedByUser";
    default:
      return "UNKNOWN";
  }
}

function createBaseTEnrollmentStatus(): TEnrollmentStatus {
  return { Success: false, FailureReason: 0, FailureReasonDetails: "" };
}

export const TEnrollmentStatus = {
  encode(message: TEnrollmentStatus, writer: Writer = Writer.create()): Writer {
    if (message.Success === true) {
      writer.uint32(8).bool(message.Success);
    }
    if (message.FailureReason !== 0) {
      writer.uint32(16).int32(message.FailureReason);
    }
    if (message.FailureReasonDetails !== "") {
      writer.uint32(26).string(message.FailureReasonDetails);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEnrollmentStatus {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEnrollmentStatus();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Success = reader.bool();
          break;
        case 2:
          message.FailureReason = reader.int32() as any;
          break;
        case 3:
          message.FailureReasonDetails = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEnrollmentStatus {
    return {
      Success: isSet(object.success) ? Boolean(object.success) : false,
      FailureReason: isSet(object.failure_reason)
        ? tEnrollmentStatus_EFailureReasonFromJSON(object.failure_reason)
        : 0,
      FailureReasonDetails: isSet(object.failure_reason_details)
        ? String(object.failure_reason_details)
        : "",
    };
  },

  toJSON(message: TEnrollmentStatus): unknown {
    const obj: any = {};
    message.Success !== undefined && (obj.success = message.Success);
    message.FailureReason !== undefined &&
      (obj.failure_reason = tEnrollmentStatus_EFailureReasonToJSON(
        message.FailureReason
      ));
    message.FailureReasonDetails !== undefined &&
      (obj.failure_reason_details = message.FailureReasonDetails);
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
