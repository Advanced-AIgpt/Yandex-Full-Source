/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

/** These parameters identify the source which formed given object */
export interface TOrigin {
  DeviceId: string | undefined;
  Uuid: string | undefined;
}

function createBaseTOrigin(): TOrigin {
  return { DeviceId: undefined, Uuid: undefined };
}

export const TOrigin = {
  encode(message: TOrigin, writer: Writer = Writer.create()): Writer {
    if (message.DeviceId !== undefined) {
      writer.uint32(10).string(message.DeviceId);
    }
    if (message.Uuid !== undefined) {
      writer.uint32(18).string(message.Uuid);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrigin {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrigin();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceId = reader.string();
          break;
        case 2:
          message.Uuid = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrigin {
    return {
      DeviceId: isSet(object.device_id) ? String(object.device_id) : undefined,
      Uuid: isSet(object.uuid) ? String(object.uuid) : undefined,
    };
  },

  toJSON(message: TOrigin): unknown {
    const obj: any = {};
    message.DeviceId !== undefined && (obj.device_id = message.DeviceId);
    message.Uuid !== undefined && (obj.uuid = message.Uuid);
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
