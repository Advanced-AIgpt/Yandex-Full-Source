/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { Any } from "../../../../google/protobuf/any";
import { TEndpoint_TStatus } from "../../../../alice/protos/endpoint/endpoint";
import { Timestamp } from "../../../../google/protobuf/timestamp";

export const protobufPackage = "NAlice";

export interface TCapabilityEvent {
  Timestamp?: Date;
  /**
   * Event must be a member of NAlice.TCapabilityEventHolder message
   * from alice/protos/endpoint/events/all/all.proto
   * but never TCapabilityEventHolder itself due to the fact that TCapabilityEventHolder depends on every known capability event
   */
  Event?: Any;
}

export interface TEndpointCapabilityEvents {
  EndpointId: string;
  Events: TCapabilityEvent[];
}

export interface TEndpointEvents {
  EndpointId: string;
  EndpointStatus?: TEndpoint_TStatus;
  CapabilityEvents: TCapabilityEvent[];
}

export interface TEndpointEventsBatch {
  Batch: TEndpointEvents[];
}

function createBaseTCapabilityEvent(): TCapabilityEvent {
  return { Timestamp: undefined, Event: undefined };
}

export const TCapabilityEvent = {
  encode(message: TCapabilityEvent, writer: Writer = Writer.create()): Writer {
    if (message.Timestamp !== undefined) {
      Timestamp.encode(
        toTimestamp(message.Timestamp),
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Event !== undefined) {
      Any.encode(message.Event, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCapabilityEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCapabilityEvent();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Timestamp = fromTimestamp(
            Timestamp.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.Event = Any.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCapabilityEvent {
    return {
      Timestamp: isSet(object.timestamp)
        ? fromJsonTimestamp(object.timestamp)
        : undefined,
      Event: isSet(object.event) ? Any.fromJSON(object.event) : undefined,
    };
  },

  toJSON(message: TCapabilityEvent): unknown {
    const obj: any = {};
    message.Timestamp !== undefined &&
      (obj.timestamp = message.Timestamp.toISOString());
    message.Event !== undefined &&
      (obj.event = message.Event ? Any.toJSON(message.Event) : undefined);
    return obj;
  },
};

function createBaseTEndpointCapabilityEvents(): TEndpointCapabilityEvents {
  return { EndpointId: "", Events: [] };
}

export const TEndpointCapabilityEvents = {
  encode(
    message: TEndpointCapabilityEvents,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EndpointId !== "") {
      writer.uint32(10).string(message.EndpointId);
    }
    for (const v of message.Events) {
      TCapabilityEvent.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointCapabilityEvents {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointCapabilityEvents();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EndpointId = reader.string();
          break;
        case 2:
          message.Events.push(TCapabilityEvent.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEndpointCapabilityEvents {
    return {
      EndpointId: isSet(object.endpoint_id) ? String(object.endpoint_id) : "",
      Events: Array.isArray(object?.events)
        ? object.events.map((e: any) => TCapabilityEvent.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TEndpointCapabilityEvents): unknown {
    const obj: any = {};
    message.EndpointId !== undefined && (obj.endpoint_id = message.EndpointId);
    if (message.Events) {
      obj.events = message.Events.map((e) =>
        e ? TCapabilityEvent.toJSON(e) : undefined
      );
    } else {
      obj.events = [];
    }
    return obj;
  },
};

function createBaseTEndpointEvents(): TEndpointEvents {
  return { EndpointId: "", EndpointStatus: undefined, CapabilityEvents: [] };
}

export const TEndpointEvents = {
  encode(message: TEndpointEvents, writer: Writer = Writer.create()): Writer {
    if (message.EndpointId !== "") {
      writer.uint32(10).string(message.EndpointId);
    }
    if (message.EndpointStatus !== undefined) {
      TEndpoint_TStatus.encode(
        message.EndpointStatus,
        writer.uint32(18).fork()
      ).ldelim();
    }
    for (const v of message.CapabilityEvents) {
      TCapabilityEvent.encode(v!, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEndpointEvents {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointEvents();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EndpointId = reader.string();
          break;
        case 2:
          message.EndpointStatus = TEndpoint_TStatus.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.CapabilityEvents.push(
            TCapabilityEvent.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEndpointEvents {
    return {
      EndpointId: isSet(object.endpoint_id) ? String(object.endpoint_id) : "",
      EndpointStatus: isSet(object.endpoint_status)
        ? TEndpoint_TStatus.fromJSON(object.endpoint_status)
        : undefined,
      CapabilityEvents: Array.isArray(object?.capability_events)
        ? object.capability_events.map((e: any) => TCapabilityEvent.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TEndpointEvents): unknown {
    const obj: any = {};
    message.EndpointId !== undefined && (obj.endpoint_id = message.EndpointId);
    message.EndpointStatus !== undefined &&
      (obj.endpoint_status = message.EndpointStatus
        ? TEndpoint_TStatus.toJSON(message.EndpointStatus)
        : undefined);
    if (message.CapabilityEvents) {
      obj.capability_events = message.CapabilityEvents.map((e) =>
        e ? TCapabilityEvent.toJSON(e) : undefined
      );
    } else {
      obj.capability_events = [];
    }
    return obj;
  },
};

function createBaseTEndpointEventsBatch(): TEndpointEventsBatch {
  return { Batch: [] };
}

export const TEndpointEventsBatch = {
  encode(
    message: TEndpointEventsBatch,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.Batch) {
      TEndpointEvents.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEndpointEventsBatch {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointEventsBatch();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Batch.push(TEndpointEvents.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEndpointEventsBatch {
    return {
      Batch: Array.isArray(object?.batch)
        ? object.batch.map((e: any) => TEndpointEvents.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TEndpointEventsBatch): unknown {
    const obj: any = {};
    if (message.Batch) {
      obj.batch = message.Batch.map((e) =>
        e ? TEndpointEvents.toJSON(e) : undefined
      );
    } else {
      obj.batch = [];
    }
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
