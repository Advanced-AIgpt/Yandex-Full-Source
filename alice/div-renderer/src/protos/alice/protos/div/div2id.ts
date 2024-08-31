/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice";

/** special composite identifier to identifiy div views inside the system */
export interface TDiv2Id {
  /** logical card/screen/view name */
  CardName: string;
  /** special id to unique identify the card (when CardName is not enough), optional */
  CardId: string;
}

function createBaseTDiv2Id(): TDiv2Id {
  return { CardName: "", CardId: "" };
}

export const TDiv2Id = {
  encode(message: TDiv2Id, writer: Writer = Writer.create()): Writer {
    if (message.CardName !== "") {
      writer.uint32(10).string(message.CardName);
    }
    if (message.CardId !== "") {
      writer.uint32(18).string(message.CardId);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDiv2Id {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDiv2Id();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CardName = reader.string();
          break;
        case 2:
          message.CardId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDiv2Id {
    return {
      CardName: isSet(object.card_name) ? String(object.card_name) : "",
      CardId: isSet(object.card_id) ? String(object.card_id) : "",
    };
  },

  toJSON(message: TDiv2Id): unknown {
    const obj: any = {};
    message.CardName !== undefined && (obj.card_name = message.CardName);
    message.CardId !== undefined && (obj.card_id = message.CardId);
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
