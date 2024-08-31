/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TLatLon {
  Latitude: number;
  Longitude: number;
}

function createBaseTLatLon(): TLatLon {
  return { Latitude: 0, Longitude: 0 };
}

export const TLatLon = {
  encode(message: TLatLon, writer: Writer = Writer.create()): Writer {
    if (message.Latitude !== 0) {
      writer.uint32(9).double(message.Latitude);
    }
    if (message.Longitude !== 0) {
      writer.uint32(17).double(message.Longitude);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TLatLon {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLatLon();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Latitude = reader.double();
          break;
        case 2:
          message.Longitude = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLatLon {
    return {
      Latitude: isSet(object.latitude) ? Number(object.latitude) : 0,
      Longitude: isSet(object.longitude) ? Number(object.longitude) : 0,
    };
  },

  toJSON(message: TLatLon): unknown {
    const obj: any = {};
    message.Latitude !== undefined && (obj.latitude = message.Latitude);
    message.Longitude !== undefined && (obj.longitude = message.Longitude);
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
