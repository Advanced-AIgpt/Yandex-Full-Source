/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

export interface TFrameRequestParams {
  DisableOutputSpeech: boolean;
  DisableShouldListen: boolean;
}

function createBaseTFrameRequestParams(): TFrameRequestParams {
  return { DisableOutputSpeech: false, DisableShouldListen: false };
}

export const TFrameRequestParams = {
  encode(
    message: TFrameRequestParams,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DisableOutputSpeech === true) {
      writer.uint32(8).bool(message.DisableOutputSpeech);
    }
    if (message.DisableShouldListen === true) {
      writer.uint32(16).bool(message.DisableShouldListen);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFrameRequestParams {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFrameRequestParams();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DisableOutputSpeech = reader.bool();
          break;
        case 2:
          message.DisableShouldListen = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFrameRequestParams {
    return {
      DisableOutputSpeech: isSet(object.disable_output_speech)
        ? Boolean(object.disable_output_speech)
        : false,
      DisableShouldListen: isSet(object.disable_should_listen)
        ? Boolean(object.disable_should_listen)
        : false,
    };
  },

  toJSON(message: TFrameRequestParams): unknown {
    const obj: any = {};
    message.DisableOutputSpeech !== undefined &&
      (obj.disable_output_speech = message.DisableOutputSpeech);
    message.DisableShouldListen !== undefined &&
      (obj.disable_should_listen = message.DisableShouldListen);
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
