/* eslint-disable */
import { util, configure } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

/** It is a flag, so the number must be a power of 2. */
export enum EAccess {
  A_PUBLIC = 0,
  A_PRIVATE_REQUEST = 1,
  A_PRIVATE_RESPONSE = 2,
  A_PRIVATE_EVENTLOG = 4,
  UNRECOGNIZED = -1,
}

export function eAccessFromJSON(object: any): EAccess {
  switch (object) {
    case 0:
    case "A_PUBLIC":
      return EAccess.A_PUBLIC;
    case 1:
    case "A_PRIVATE_REQUEST":
      return EAccess.A_PRIVATE_REQUEST;
    case 2:
    case "A_PRIVATE_RESPONSE":
      return EAccess.A_PRIVATE_RESPONSE;
    case 4:
    case "A_PRIVATE_EVENTLOG":
      return EAccess.A_PRIVATE_EVENTLOG;
    case -1:
    case "UNRECOGNIZED":
    default:
      return EAccess.UNRECOGNIZED;
  }
}

export function eAccessToJSON(object: EAccess): string {
  switch (object) {
    case EAccess.A_PUBLIC:
      return "A_PUBLIC";
    case EAccess.A_PRIVATE_REQUEST:
      return "A_PRIVATE_REQUEST";
    case EAccess.A_PRIVATE_RESPONSE:
      return "A_PRIVATE_RESPONSE";
    case EAccess.A_PRIVATE_EVENTLOG:
      return "A_PRIVATE_EVENTLOG";
    default:
      return "UNKNOWN";
  }
}

// If you get a compile-error about 'Constructor<Long> and ... have no overlap',
// add '--ts_proto_opt=esModuleInterop=true' as a flag when calling 'protoc'.
if (util.Long !== Long) {
  util.Long = Long as any;
  configure();
}
