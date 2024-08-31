/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

/** @deprecated */
export interface TCentaurWidgetData {
  Fixed: boolean;
  VacantWidgetData?: TCentaurWidgetData_TVacantWidgetData | undefined;
  MusicWidgetData?: TCentaurWidgetData_TMusicWidgetData | undefined;
  NotificationWidgetData?:
    | TCentaurWidgetData_TNotificationWidgetData
    | undefined;
  WeatherWidgetData?: TCentaurWidgetData_TWeatherWidgetData | undefined;
  NewsWidgetData?: TCentaurWidgetData_TNewsWidgetData | undefined;
  OpenWebviewWidgetData?: TCentaurWidgetData_TOpenWebviewWidgetData | undefined;
  TrafficWidgetData?: TCentaurWidgetData_TTrafficWidgetData | undefined;
  VideoCallWidgetData?: TCentaurWidgetData_TVideoCallWidgetData | undefined;
}

export interface TCentaurWidgetData_TVacantWidgetData {}

export interface TCentaurWidgetData_TMusicWidgetData {}

export interface TCentaurWidgetData_TNotificationWidgetData {}

export interface TCentaurWidgetData_TWeatherWidgetData {}

export interface TCentaurWidgetData_TNewsWidgetData {}

export interface TCentaurWidgetData_TTrafficWidgetData {}

export interface TCentaurWidgetData_TVideoCallWidgetData {}

export interface TCentaurWidgetData_TOpenWebviewWidgetData {
  Application: string | undefined;
  Url: string | undefined;
}

export interface TCentaurWidgetConfigData {
  Id: string;
  WidgetType: string;
  CustomWidgetTypeData?: TCentaurWidgetConfigData_TCustomWidgetTypeData;
  Fixed: boolean;
}

export interface TCentaurWidgetConfigData_TCustomWidgetTypeData {}

export interface TWidgetPosition {
  Column: number;
  Row: number;
}

function createBaseTCentaurWidgetData(): TCentaurWidgetData {
  return {
    Fixed: false,
    VacantWidgetData: undefined,
    MusicWidgetData: undefined,
    NotificationWidgetData: undefined,
    WeatherWidgetData: undefined,
    NewsWidgetData: undefined,
    OpenWebviewWidgetData: undefined,
    TrafficWidgetData: undefined,
    VideoCallWidgetData: undefined,
  };
}

export const TCentaurWidgetData = {
  encode(
    message: TCentaurWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Fixed === true) {
      writer.uint32(8).bool(message.Fixed);
    }
    if (message.VacantWidgetData !== undefined) {
      TCentaurWidgetData_TVacantWidgetData.encode(
        message.VacantWidgetData,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.MusicWidgetData !== undefined) {
      TCentaurWidgetData_TMusicWidgetData.encode(
        message.MusicWidgetData,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.NotificationWidgetData !== undefined) {
      TCentaurWidgetData_TNotificationWidgetData.encode(
        message.NotificationWidgetData,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.WeatherWidgetData !== undefined) {
      TCentaurWidgetData_TWeatherWidgetData.encode(
        message.WeatherWidgetData,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.NewsWidgetData !== undefined) {
      TCentaurWidgetData_TNewsWidgetData.encode(
        message.NewsWidgetData,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.OpenWebviewWidgetData !== undefined) {
      TCentaurWidgetData_TOpenWebviewWidgetData.encode(
        message.OpenWebviewWidgetData,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.TrafficWidgetData !== undefined) {
      TCentaurWidgetData_TTrafficWidgetData.encode(
        message.TrafficWidgetData,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.VideoCallWidgetData !== undefined) {
      TCentaurWidgetData_TVideoCallWidgetData.encode(
        message.VideoCallWidgetData,
        writer.uint32(74).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCentaurWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Fixed = reader.bool();
          break;
        case 2:
          message.VacantWidgetData =
            TCentaurWidgetData_TVacantWidgetData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.MusicWidgetData = TCentaurWidgetData_TMusicWidgetData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.NotificationWidgetData =
            TCentaurWidgetData_TNotificationWidgetData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 5:
          message.WeatherWidgetData =
            TCentaurWidgetData_TWeatherWidgetData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.NewsWidgetData = TCentaurWidgetData_TNewsWidgetData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 7:
          message.OpenWebviewWidgetData =
            TCentaurWidgetData_TOpenWebviewWidgetData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 8:
          message.TrafficWidgetData =
            TCentaurWidgetData_TTrafficWidgetData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 9:
          message.VideoCallWidgetData =
            TCentaurWidgetData_TVideoCallWidgetData.decode(
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

  fromJSON(object: any): TCentaurWidgetData {
    return {
      Fixed: isSet(object.fixed) ? Boolean(object.fixed) : false,
      VacantWidgetData: isSet(object.vacant_widget_data)
        ? TCentaurWidgetData_TVacantWidgetData.fromJSON(
            object.vacant_widget_data
          )
        : undefined,
      MusicWidgetData: isSet(object.music_widget_data)
        ? TCentaurWidgetData_TMusicWidgetData.fromJSON(object.music_widget_data)
        : undefined,
      NotificationWidgetData: isSet(object.notification_widget_data)
        ? TCentaurWidgetData_TNotificationWidgetData.fromJSON(
            object.notification_widget_data
          )
        : undefined,
      WeatherWidgetData: isSet(object.weather_widget_data)
        ? TCentaurWidgetData_TWeatherWidgetData.fromJSON(
            object.weather_widget_data
          )
        : undefined,
      NewsWidgetData: isSet(object.news_widget_data)
        ? TCentaurWidgetData_TNewsWidgetData.fromJSON(object.news_widget_data)
        : undefined,
      OpenWebviewWidgetData: isSet(object.open_webview_widget_data)
        ? TCentaurWidgetData_TOpenWebviewWidgetData.fromJSON(
            object.open_webview_widget_data
          )
        : undefined,
      TrafficWidgetData: isSet(object.traffic_widget_data)
        ? TCentaurWidgetData_TTrafficWidgetData.fromJSON(
            object.traffic_widget_data
          )
        : undefined,
      VideoCallWidgetData: isSet(object.video_call_widget_data)
        ? TCentaurWidgetData_TVideoCallWidgetData.fromJSON(
            object.video_call_widget_data
          )
        : undefined,
    };
  },

  toJSON(message: TCentaurWidgetData): unknown {
    const obj: any = {};
    message.Fixed !== undefined && (obj.fixed = message.Fixed);
    message.VacantWidgetData !== undefined &&
      (obj.vacant_widget_data = message.VacantWidgetData
        ? TCentaurWidgetData_TVacantWidgetData.toJSON(message.VacantWidgetData)
        : undefined);
    message.MusicWidgetData !== undefined &&
      (obj.music_widget_data = message.MusicWidgetData
        ? TCentaurWidgetData_TMusicWidgetData.toJSON(message.MusicWidgetData)
        : undefined);
    message.NotificationWidgetData !== undefined &&
      (obj.notification_widget_data = message.NotificationWidgetData
        ? TCentaurWidgetData_TNotificationWidgetData.toJSON(
            message.NotificationWidgetData
          )
        : undefined);
    message.WeatherWidgetData !== undefined &&
      (obj.weather_widget_data = message.WeatherWidgetData
        ? TCentaurWidgetData_TWeatherWidgetData.toJSON(
            message.WeatherWidgetData
          )
        : undefined);
    message.NewsWidgetData !== undefined &&
      (obj.news_widget_data = message.NewsWidgetData
        ? TCentaurWidgetData_TNewsWidgetData.toJSON(message.NewsWidgetData)
        : undefined);
    message.OpenWebviewWidgetData !== undefined &&
      (obj.open_webview_widget_data = message.OpenWebviewWidgetData
        ? TCentaurWidgetData_TOpenWebviewWidgetData.toJSON(
            message.OpenWebviewWidgetData
          )
        : undefined);
    message.TrafficWidgetData !== undefined &&
      (obj.traffic_widget_data = message.TrafficWidgetData
        ? TCentaurWidgetData_TTrafficWidgetData.toJSON(
            message.TrafficWidgetData
          )
        : undefined);
    message.VideoCallWidgetData !== undefined &&
      (obj.video_call_widget_data = message.VideoCallWidgetData
        ? TCentaurWidgetData_TVideoCallWidgetData.toJSON(
            message.VideoCallWidgetData
          )
        : undefined);
    return obj;
  },
};

function createBaseTCentaurWidgetData_TVacantWidgetData(): TCentaurWidgetData_TVacantWidgetData {
  return {};
}

export const TCentaurWidgetData_TVacantWidgetData = {
  encode(
    _: TCentaurWidgetData_TVacantWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TVacantWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TVacantWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TVacantWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TVacantWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TMusicWidgetData(): TCentaurWidgetData_TMusicWidgetData {
  return {};
}

export const TCentaurWidgetData_TMusicWidgetData = {
  encode(
    _: TCentaurWidgetData_TMusicWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TMusicWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TMusicWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TMusicWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TMusicWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TNotificationWidgetData(): TCentaurWidgetData_TNotificationWidgetData {
  return {};
}

export const TCentaurWidgetData_TNotificationWidgetData = {
  encode(
    _: TCentaurWidgetData_TNotificationWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TNotificationWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TNotificationWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TNotificationWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TNotificationWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TWeatherWidgetData(): TCentaurWidgetData_TWeatherWidgetData {
  return {};
}

export const TCentaurWidgetData_TWeatherWidgetData = {
  encode(
    _: TCentaurWidgetData_TWeatherWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TWeatherWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TWeatherWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TWeatherWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TWeatherWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TNewsWidgetData(): TCentaurWidgetData_TNewsWidgetData {
  return {};
}

export const TCentaurWidgetData_TNewsWidgetData = {
  encode(
    _: TCentaurWidgetData_TNewsWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TNewsWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TNewsWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TNewsWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TNewsWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TTrafficWidgetData(): TCentaurWidgetData_TTrafficWidgetData {
  return {};
}

export const TCentaurWidgetData_TTrafficWidgetData = {
  encode(
    _: TCentaurWidgetData_TTrafficWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TTrafficWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TTrafficWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TTrafficWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TTrafficWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TVideoCallWidgetData(): TCentaurWidgetData_TVideoCallWidgetData {
  return {};
}

export const TCentaurWidgetData_TVideoCallWidgetData = {
  encode(
    _: TCentaurWidgetData_TVideoCallWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TVideoCallWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TVideoCallWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetData_TVideoCallWidgetData {
    return {};
  },

  toJSON(_: TCentaurWidgetData_TVideoCallWidgetData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurWidgetData_TOpenWebviewWidgetData(): TCentaurWidgetData_TOpenWebviewWidgetData {
  return { Application: undefined, Url: undefined };
}

export const TCentaurWidgetData_TOpenWebviewWidgetData = {
  encode(
    message: TCentaurWidgetData_TOpenWebviewWidgetData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Application !== undefined) {
      writer.uint32(10).string(message.Application);
    }
    if (message.Url !== undefined) {
      writer.uint32(18).string(message.Url);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetData_TOpenWebviewWidgetData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetData_TOpenWebviewWidgetData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Application = reader.string();
          break;
        case 2:
          message.Url = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCentaurWidgetData_TOpenWebviewWidgetData {
    return {
      Application: isSet(object.application)
        ? String(object.application)
        : undefined,
      Url: isSet(object.url) ? String(object.url) : undefined,
    };
  },

  toJSON(message: TCentaurWidgetData_TOpenWebviewWidgetData): unknown {
    const obj: any = {};
    message.Application !== undefined &&
      (obj.application = message.Application);
    message.Url !== undefined && (obj.url = message.Url);
    return obj;
  },
};

function createBaseTCentaurWidgetConfigData(): TCentaurWidgetConfigData {
  return {
    Id: "",
    WidgetType: "",
    CustomWidgetTypeData: undefined,
    Fixed: false,
  };
}

export const TCentaurWidgetConfigData = {
  encode(
    message: TCentaurWidgetConfigData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.WidgetType !== "") {
      writer.uint32(18).string(message.WidgetType);
    }
    if (message.CustomWidgetTypeData !== undefined) {
      TCentaurWidgetConfigData_TCustomWidgetTypeData.encode(
        message.CustomWidgetTypeData,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Fixed === true) {
      writer.uint32(32).bool(message.Fixed);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetConfigData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetConfigData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.WidgetType = reader.string();
          break;
        case 3:
          message.CustomWidgetTypeData =
            TCentaurWidgetConfigData_TCustomWidgetTypeData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.Fixed = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCentaurWidgetConfigData {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      WidgetType: isSet(object.widget_type) ? String(object.widget_type) : "",
      CustomWidgetTypeData: isSet(object.custom_widget_type_data)
        ? TCentaurWidgetConfigData_TCustomWidgetTypeData.fromJSON(
            object.custom_widget_type_data
          )
        : undefined,
      Fixed: isSet(object.fixed) ? Boolean(object.fixed) : false,
    };
  },

  toJSON(message: TCentaurWidgetConfigData): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.WidgetType !== undefined && (obj.widget_type = message.WidgetType);
    message.CustomWidgetTypeData !== undefined &&
      (obj.custom_widget_type_data = message.CustomWidgetTypeData
        ? TCentaurWidgetConfigData_TCustomWidgetTypeData.toJSON(
            message.CustomWidgetTypeData
          )
        : undefined);
    message.Fixed !== undefined && (obj.fixed = message.Fixed);
    return obj;
  },
};

function createBaseTCentaurWidgetConfigData_TCustomWidgetTypeData(): TCentaurWidgetConfigData_TCustomWidgetTypeData {
  return {};
}

export const TCentaurWidgetConfigData_TCustomWidgetTypeData = {
  encode(
    _: TCentaurWidgetConfigData_TCustomWidgetTypeData,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurWidgetConfigData_TCustomWidgetTypeData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurWidgetConfigData_TCustomWidgetTypeData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(_: any): TCentaurWidgetConfigData_TCustomWidgetTypeData {
    return {};
  },

  toJSON(_: TCentaurWidgetConfigData_TCustomWidgetTypeData): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTWidgetPosition(): TWidgetPosition {
  return { Column: 0, Row: 0 };
}

export const TWidgetPosition = {
  encode(message: TWidgetPosition, writer: Writer = Writer.create()): Writer {
    if (message.Column !== 0) {
      writer.uint32(8).int32(message.Column);
    }
    if (message.Row !== 0) {
      writer.uint32(16).int32(message.Row);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWidgetPosition {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWidgetPosition();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Column = reader.int32();
          break;
        case 2:
          message.Row = reader.int32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWidgetPosition {
    return {
      Column: isSet(object.column) ? Number(object.column) : 0,
      Row: isSet(object.row) ? Number(object.row) : 0,
    };
  },

  toJSON(message: TWidgetPosition): unknown {
    const obj: any = {};
    message.Column !== undefined && (obj.column = Math.round(message.Column));
    message.Row !== undefined && (obj.row = Math.round(message.Row));
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
