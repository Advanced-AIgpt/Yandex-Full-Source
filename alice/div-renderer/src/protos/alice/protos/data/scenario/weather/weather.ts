/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";

export const protobufPackage = "NAlice.NData";

export interface TWeatherCondition {
  Title: string;
  FeelsLike: number;
  Cloudness: number;
  PrecStrength: number;
  PrecType: number;
}

export interface TWeatherLocation {
  City: string;
  CityPrepcase: string;
}

export interface TDaylightHours {
  Sunrise: string;
  Sunset: string;
}

export interface TWeatherTeaserData {
  HourItems: TWeatherHourItem[];
  Date: string;
  GeoLocation?: TWeatherLocation;
  Tz: string;
  UserTime: string;
  Sunrise: string;
  Sunset: string;
  Temperature: number;
  Icon: string;
  Condition?: TWeatherCondition;
  IconType: string;
  UserDate: string;
}

export interface TWeatherMainScreenData {
  Date: string;
  GeoLocation?: TWeatherLocation;
  Tz: string;
  UserTime: string;
  Sunrise: string;
  Sunset: string;
  Temperature: number;
  Icon: string;
  Condition?: TWeatherCondition;
  IconType: string;
  UserDate: string;
}

export interface TWeatherHourItem {
  Hour: number;
  Timestamp: number;
  Temperature: number;
  Icon: string;
  IconType: string;
  PrecStrength: number;
  PrecType: number;
}

export interface TWeatherDayHoursData {
  HourItems: TWeatherHourItem[];
  Date: string;
  GeoLocation?: TWeatherLocation;
  DayPartType: string;
  Tz: string;
  UserTime: string;
  Sunrise: string;
  Sunset: string;
  Temperature: number;
  Icon: string;
  Condition?: TWeatherCondition;
  TodayDaylight?: TDaylightHours;
  IconType: string;
  UserDate: string;
}

export interface TWeatherDayPartData {
  Date: string;
  GeoLocation?: TWeatherLocation;
  DayPartType: string;
  Tz: string;
  UserTime: string;
  Sunrise: string;
  Sunset: string;
  Temperature: number;
  Icon: string;
  Condition?: TWeatherCondition;
  TodayDaylight?: TDaylightHours;
  IconType: string;
  UserDate: string;
}

export interface TWeatherDayPartItem {
  DayPartType: string;
  Temperature: number;
  Icon: string;
  Condition?: TWeatherCondition;
  IconType: string;
}

export interface TWeatherDayData {
  DayPartItems: TWeatherDayPartItem[];
  Date: string;
  GeoLocation?: TWeatherLocation;
  Tz: string;
  UserTime: string;
  Sunrise: string;
  Sunset: string;
  Temperature: number;
  Icon: string;
  Condition?: TWeatherCondition;
  TodayDaylight?: TDaylightHours;
  IconType: string;
  UserDate: string;
}

export interface TWeatherDayItem {
  Date: string;
  Tz: string;
  WeekDay: number;
  DayTemp: number;
  NightTemp: number;
  Icon: string;
  Condition?: TWeatherCondition;
  Url: string;
  IconType: string;
}

export interface TWeatherDaysRangeData {
  DayItems: TWeatherDayItem[];
  GeoLocation?: TWeatherLocation;
  Tz: string;
  UserTime: string;
  TodayDaylight?: TDaylightHours;
  UserDate: string;
}

function createBaseTWeatherCondition(): TWeatherCondition {
  return {
    Title: "",
    FeelsLike: 0,
    Cloudness: 0,
    PrecStrength: 0,
    PrecType: 0,
  };
}

export const TWeatherCondition = {
  encode(message: TWeatherCondition, writer: Writer = Writer.create()): Writer {
    if (message.Title !== "") {
      writer.uint32(10).string(message.Title);
    }
    if (message.FeelsLike !== 0) {
      writer.uint32(17).double(message.FeelsLike);
    }
    if (message.Cloudness !== 0) {
      writer.uint32(25).double(message.Cloudness);
    }
    if (message.PrecStrength !== 0) {
      writer.uint32(33).double(message.PrecStrength);
    }
    if (message.PrecType !== 0) {
      writer.uint32(40).uint32(message.PrecType);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherCondition {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherCondition();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Title = reader.string();
          break;
        case 2:
          message.FeelsLike = reader.double();
          break;
        case 3:
          message.Cloudness = reader.double();
          break;
        case 4:
          message.PrecStrength = reader.double();
          break;
        case 5:
          message.PrecType = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherCondition {
    return {
      Title: isSet(object.title) ? String(object.title) : "",
      FeelsLike: isSet(object.feels_like) ? Number(object.feels_like) : 0,
      Cloudness: isSet(object.cloudness) ? Number(object.cloudness) : 0,
      PrecStrength: isSet(object.prec_strength)
        ? Number(object.prec_strength)
        : 0,
      PrecType: isSet(object.prec_type) ? Number(object.prec_type) : 0,
    };
  },

  toJSON(message: TWeatherCondition): unknown {
    const obj: any = {};
    message.Title !== undefined && (obj.title = message.Title);
    message.FeelsLike !== undefined && (obj.feels_like = message.FeelsLike);
    message.Cloudness !== undefined && (obj.cloudness = message.Cloudness);
    message.PrecStrength !== undefined &&
      (obj.prec_strength = message.PrecStrength);
    message.PrecType !== undefined &&
      (obj.prec_type = Math.round(message.PrecType));
    return obj;
  },
};

function createBaseTWeatherLocation(): TWeatherLocation {
  return { City: "", CityPrepcase: "" };
}

export const TWeatherLocation = {
  encode(message: TWeatherLocation, writer: Writer = Writer.create()): Writer {
    if (message.City !== "") {
      writer.uint32(10).string(message.City);
    }
    if (message.CityPrepcase !== "") {
      writer.uint32(18).string(message.CityPrepcase);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherLocation {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherLocation();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.City = reader.string();
          break;
        case 2:
          message.CityPrepcase = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherLocation {
    return {
      City: isSet(object.city) ? String(object.city) : "",
      CityPrepcase: isSet(object.city_prepcase)
        ? String(object.city_prepcase)
        : "",
    };
  },

  toJSON(message: TWeatherLocation): unknown {
    const obj: any = {};
    message.City !== undefined && (obj.city = message.City);
    message.CityPrepcase !== undefined &&
      (obj.city_prepcase = message.CityPrepcase);
    return obj;
  },
};

function createBaseTDaylightHours(): TDaylightHours {
  return { Sunrise: "", Sunset: "" };
}

export const TDaylightHours = {
  encode(message: TDaylightHours, writer: Writer = Writer.create()): Writer {
    if (message.Sunrise !== "") {
      writer.uint32(10).string(message.Sunrise);
    }
    if (message.Sunset !== "") {
      writer.uint32(18).string(message.Sunset);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDaylightHours {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDaylightHours();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Sunrise = reader.string();
          break;
        case 2:
          message.Sunset = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDaylightHours {
    return {
      Sunrise: isSet(object.sunrise) ? String(object.sunrise) : "",
      Sunset: isSet(object.sunset) ? String(object.sunset) : "",
    };
  },

  toJSON(message: TDaylightHours): unknown {
    const obj: any = {};
    message.Sunrise !== undefined && (obj.sunrise = message.Sunrise);
    message.Sunset !== undefined && (obj.sunset = message.Sunset);
    return obj;
  },
};

function createBaseTWeatherTeaserData(): TWeatherTeaserData {
  return {
    HourItems: [],
    Date: "",
    GeoLocation: undefined,
    Tz: "",
    UserTime: "",
    Sunrise: "",
    Sunset: "",
    Temperature: 0,
    Icon: "",
    Condition: undefined,
    IconType: "",
    UserDate: "",
  };
}

export const TWeatherTeaserData = {
  encode(
    message: TWeatherTeaserData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.HourItems) {
      TWeatherHourItem.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    if (message.Date !== "") {
      writer.uint32(18).string(message.Date);
    }
    if (message.GeoLocation !== undefined) {
      TWeatherLocation.encode(
        message.GeoLocation,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Tz !== "") {
      writer.uint32(34).string(message.Tz);
    }
    if (message.UserTime !== "") {
      writer.uint32(42).string(message.UserTime);
    }
    if (message.Sunrise !== "") {
      writer.uint32(50).string(message.Sunrise);
    }
    if (message.Sunset !== "") {
      writer.uint32(58).string(message.Sunset);
    }
    if (message.Temperature !== 0) {
      writer.uint32(65).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(74).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(82).fork()
      ).ldelim();
    }
    if (message.IconType !== "") {
      writer.uint32(90).string(message.IconType);
    }
    if (message.UserDate !== "") {
      writer.uint32(98).string(message.UserDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherTeaserData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherTeaserData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.HourItems.push(
            TWeatherHourItem.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.Date = reader.string();
          break;
        case 3:
          message.GeoLocation = TWeatherLocation.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.Tz = reader.string();
          break;
        case 5:
          message.UserTime = reader.string();
          break;
        case 6:
          message.Sunrise = reader.string();
          break;
        case 7:
          message.Sunset = reader.string();
          break;
        case 8:
          message.Temperature = reader.double();
          break;
        case 9:
          message.Icon = reader.string();
          break;
        case 10:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 11:
          message.IconType = reader.string();
          break;
        case 12:
          message.UserDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherTeaserData {
    return {
      HourItems: Array.isArray(object?.hour_items)
        ? object.hour_items.map((e: any) => TWeatherHourItem.fromJSON(e))
        : [],
      Date: isSet(object.date) ? String(object.date) : "",
      GeoLocation: isSet(object.geo_location)
        ? TWeatherLocation.fromJSON(object.geo_location)
        : undefined,
      Tz: isSet(object.tz) ? String(object.tz) : "",
      UserTime: isSet(object.user_time) ? String(object.user_time) : "",
      Sunrise: isSet(object.sunrise) ? String(object.sunrise) : "",
      Sunset: isSet(object.sunset) ? String(object.sunset) : "",
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
      UserDate: isSet(object.user_date) ? String(object.user_date) : "",
    };
  },

  toJSON(message: TWeatherTeaserData): unknown {
    const obj: any = {};
    if (message.HourItems) {
      obj.hour_items = message.HourItems.map((e) =>
        e ? TWeatherHourItem.toJSON(e) : undefined
      );
    } else {
      obj.hour_items = [];
    }
    message.Date !== undefined && (obj.date = message.Date);
    message.GeoLocation !== undefined &&
      (obj.geo_location = message.GeoLocation
        ? TWeatherLocation.toJSON(message.GeoLocation)
        : undefined);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.UserTime !== undefined && (obj.user_time = message.UserTime);
    message.Sunrise !== undefined && (obj.sunrise = message.Sunrise);
    message.Sunset !== undefined && (obj.sunset = message.Sunset);
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    message.UserDate !== undefined && (obj.user_date = message.UserDate);
    return obj;
  },
};

function createBaseTWeatherMainScreenData(): TWeatherMainScreenData {
  return {
    Date: "",
    GeoLocation: undefined,
    Tz: "",
    UserTime: "",
    Sunrise: "",
    Sunset: "",
    Temperature: 0,
    Icon: "",
    Condition: undefined,
    IconType: "",
    UserDate: "",
  };
}

export const TWeatherMainScreenData = {
  encode(
    message: TWeatherMainScreenData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Date !== "") {
      writer.uint32(10).string(message.Date);
    }
    if (message.GeoLocation !== undefined) {
      TWeatherLocation.encode(
        message.GeoLocation,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Tz !== "") {
      writer.uint32(26).string(message.Tz);
    }
    if (message.UserTime !== "") {
      writer.uint32(34).string(message.UserTime);
    }
    if (message.Sunrise !== "") {
      writer.uint32(42).string(message.Sunrise);
    }
    if (message.Sunset !== "") {
      writer.uint32(50).string(message.Sunset);
    }
    if (message.Temperature !== 0) {
      writer.uint32(57).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(66).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.IconType !== "") {
      writer.uint32(82).string(message.IconType);
    }
    if (message.UserDate !== "") {
      writer.uint32(90).string(message.UserDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherMainScreenData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherMainScreenData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Date = reader.string();
          break;
        case 2:
          message.GeoLocation = TWeatherLocation.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.Tz = reader.string();
          break;
        case 4:
          message.UserTime = reader.string();
          break;
        case 5:
          message.Sunrise = reader.string();
          break;
        case 6:
          message.Sunset = reader.string();
          break;
        case 7:
          message.Temperature = reader.double();
          break;
        case 8:
          message.Icon = reader.string();
          break;
        case 9:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 10:
          message.IconType = reader.string();
          break;
        case 11:
          message.UserDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherMainScreenData {
    return {
      Date: isSet(object.date) ? String(object.date) : "",
      GeoLocation: isSet(object.geo_location)
        ? TWeatherLocation.fromJSON(object.geo_location)
        : undefined,
      Tz: isSet(object.tz) ? String(object.tz) : "",
      UserTime: isSet(object.user_time) ? String(object.user_time) : "",
      Sunrise: isSet(object.sunrise) ? String(object.sunrise) : "",
      Sunset: isSet(object.sunset) ? String(object.sunset) : "",
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
      UserDate: isSet(object.user_date) ? String(object.user_date) : "",
    };
  },

  toJSON(message: TWeatherMainScreenData): unknown {
    const obj: any = {};
    message.Date !== undefined && (obj.date = message.Date);
    message.GeoLocation !== undefined &&
      (obj.geo_location = message.GeoLocation
        ? TWeatherLocation.toJSON(message.GeoLocation)
        : undefined);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.UserTime !== undefined && (obj.user_time = message.UserTime);
    message.Sunrise !== undefined && (obj.sunrise = message.Sunrise);
    message.Sunset !== undefined && (obj.sunset = message.Sunset);
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    message.UserDate !== undefined && (obj.user_date = message.UserDate);
    return obj;
  },
};

function createBaseTWeatherHourItem(): TWeatherHourItem {
  return {
    Hour: 0,
    Timestamp: 0,
    Temperature: 0,
    Icon: "",
    IconType: "",
    PrecStrength: 0,
    PrecType: 0,
  };
}

export const TWeatherHourItem = {
  encode(message: TWeatherHourItem, writer: Writer = Writer.create()): Writer {
    if (message.Hour !== 0) {
      writer.uint32(8).uint32(message.Hour);
    }
    if (message.Timestamp !== 0) {
      writer.uint32(16).uint64(message.Timestamp);
    }
    if (message.Temperature !== 0) {
      writer.uint32(25).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(34).string(message.Icon);
    }
    if (message.IconType !== "") {
      writer.uint32(42).string(message.IconType);
    }
    if (message.PrecStrength !== 0) {
      writer.uint32(49).double(message.PrecStrength);
    }
    if (message.PrecType !== 0) {
      writer.uint32(56).uint32(message.PrecType);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherHourItem {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherHourItem();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Hour = reader.uint32();
          break;
        case 2:
          message.Timestamp = longToNumber(reader.uint64() as Long);
          break;
        case 3:
          message.Temperature = reader.double();
          break;
        case 4:
          message.Icon = reader.string();
          break;
        case 5:
          message.IconType = reader.string();
          break;
        case 6:
          message.PrecStrength = reader.double();
          break;
        case 7:
          message.PrecType = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherHourItem {
    return {
      Hour: isSet(object.hour) ? Number(object.hour) : 0,
      Timestamp: isSet(object.timestamp) ? Number(object.timestamp) : 0,
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
      PrecStrength: isSet(object.prec_strength)
        ? Number(object.prec_strength)
        : 0,
      PrecType: isSet(object.prec_type) ? Number(object.prec_type) : 0,
    };
  },

  toJSON(message: TWeatherHourItem): unknown {
    const obj: any = {};
    message.Hour !== undefined && (obj.hour = Math.round(message.Hour));
    message.Timestamp !== undefined &&
      (obj.timestamp = Math.round(message.Timestamp));
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    message.PrecStrength !== undefined &&
      (obj.prec_strength = message.PrecStrength);
    message.PrecType !== undefined &&
      (obj.prec_type = Math.round(message.PrecType));
    return obj;
  },
};

function createBaseTWeatherDayHoursData(): TWeatherDayHoursData {
  return {
    HourItems: [],
    Date: "",
    GeoLocation: undefined,
    DayPartType: "",
    Tz: "",
    UserTime: "",
    Sunrise: "",
    Sunset: "",
    Temperature: 0,
    Icon: "",
    Condition: undefined,
    TodayDaylight: undefined,
    IconType: "",
    UserDate: "",
  };
}

export const TWeatherDayHoursData = {
  encode(
    message: TWeatherDayHoursData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.HourItems) {
      TWeatherHourItem.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    if (message.Date !== "") {
      writer.uint32(18).string(message.Date);
    }
    if (message.GeoLocation !== undefined) {
      TWeatherLocation.encode(
        message.GeoLocation,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.DayPartType !== "") {
      writer.uint32(34).string(message.DayPartType);
    }
    if (message.Tz !== "") {
      writer.uint32(42).string(message.Tz);
    }
    if (message.UserTime !== "") {
      writer.uint32(50).string(message.UserTime);
    }
    if (message.Sunrise !== "") {
      writer.uint32(58).string(message.Sunrise);
    }
    if (message.Sunset !== "") {
      writer.uint32(66).string(message.Sunset);
    }
    if (message.Temperature !== 0) {
      writer.uint32(73).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(82).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(90).fork()
      ).ldelim();
    }
    if (message.TodayDaylight !== undefined) {
      TDaylightHours.encode(
        message.TodayDaylight,
        writer.uint32(98).fork()
      ).ldelim();
    }
    if (message.IconType !== "") {
      writer.uint32(106).string(message.IconType);
    }
    if (message.UserDate !== "") {
      writer.uint32(114).string(message.UserDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherDayHoursData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherDayHoursData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.HourItems.push(
            TWeatherHourItem.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.Date = reader.string();
          break;
        case 3:
          message.GeoLocation = TWeatherLocation.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.DayPartType = reader.string();
          break;
        case 5:
          message.Tz = reader.string();
          break;
        case 6:
          message.UserTime = reader.string();
          break;
        case 7:
          message.Sunrise = reader.string();
          break;
        case 8:
          message.Sunset = reader.string();
          break;
        case 9:
          message.Temperature = reader.double();
          break;
        case 10:
          message.Icon = reader.string();
          break;
        case 11:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 12:
          message.TodayDaylight = TDaylightHours.decode(
            reader,
            reader.uint32()
          );
          break;
        case 13:
          message.IconType = reader.string();
          break;
        case 14:
          message.UserDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherDayHoursData {
    return {
      HourItems: Array.isArray(object?.hour_items)
        ? object.hour_items.map((e: any) => TWeatherHourItem.fromJSON(e))
        : [],
      Date: isSet(object.date) ? String(object.date) : "",
      GeoLocation: isSet(object.geo_location)
        ? TWeatherLocation.fromJSON(object.geo_location)
        : undefined,
      DayPartType: isSet(object.day_part_type)
        ? String(object.day_part_type)
        : "",
      Tz: isSet(object.tz) ? String(object.tz) : "",
      UserTime: isSet(object.user_time) ? String(object.user_time) : "",
      Sunrise: isSet(object.sunrise) ? String(object.sunrise) : "",
      Sunset: isSet(object.sunset) ? String(object.sunset) : "",
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      TodayDaylight: isSet(object.today_dayligth)
        ? TDaylightHours.fromJSON(object.today_dayligth)
        : undefined,
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
      UserDate: isSet(object.user_date) ? String(object.user_date) : "",
    };
  },

  toJSON(message: TWeatherDayHoursData): unknown {
    const obj: any = {};
    if (message.HourItems) {
      obj.hour_items = message.HourItems.map((e) =>
        e ? TWeatherHourItem.toJSON(e) : undefined
      );
    } else {
      obj.hour_items = [];
    }
    message.Date !== undefined && (obj.date = message.Date);
    message.GeoLocation !== undefined &&
      (obj.geo_location = message.GeoLocation
        ? TWeatherLocation.toJSON(message.GeoLocation)
        : undefined);
    message.DayPartType !== undefined &&
      (obj.day_part_type = message.DayPartType);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.UserTime !== undefined && (obj.user_time = message.UserTime);
    message.Sunrise !== undefined && (obj.sunrise = message.Sunrise);
    message.Sunset !== undefined && (obj.sunset = message.Sunset);
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.TodayDaylight !== undefined &&
      (obj.today_dayligth = message.TodayDaylight
        ? TDaylightHours.toJSON(message.TodayDaylight)
        : undefined);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    message.UserDate !== undefined && (obj.user_date = message.UserDate);
    return obj;
  },
};

function createBaseTWeatherDayPartData(): TWeatherDayPartData {
  return {
    Date: "",
    GeoLocation: undefined,
    DayPartType: "",
    Tz: "",
    UserTime: "",
    Sunrise: "",
    Sunset: "",
    Temperature: 0,
    Icon: "",
    Condition: undefined,
    TodayDaylight: undefined,
    IconType: "",
    UserDate: "",
  };
}

export const TWeatherDayPartData = {
  encode(
    message: TWeatherDayPartData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Date !== "") {
      writer.uint32(10).string(message.Date);
    }
    if (message.GeoLocation !== undefined) {
      TWeatherLocation.encode(
        message.GeoLocation,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.DayPartType !== "") {
      writer.uint32(26).string(message.DayPartType);
    }
    if (message.Tz !== "") {
      writer.uint32(34).string(message.Tz);
    }
    if (message.UserTime !== "") {
      writer.uint32(42).string(message.UserTime);
    }
    if (message.Sunrise !== "") {
      writer.uint32(50).string(message.Sunrise);
    }
    if (message.Sunset !== "") {
      writer.uint32(58).string(message.Sunset);
    }
    if (message.Temperature !== 0) {
      writer.uint32(65).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(74).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(82).fork()
      ).ldelim();
    }
    if (message.TodayDaylight !== undefined) {
      TDaylightHours.encode(
        message.TodayDaylight,
        writer.uint32(90).fork()
      ).ldelim();
    }
    if (message.IconType !== "") {
      writer.uint32(98).string(message.IconType);
    }
    if (message.UserDate !== "") {
      writer.uint32(106).string(message.UserDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherDayPartData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherDayPartData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Date = reader.string();
          break;
        case 2:
          message.GeoLocation = TWeatherLocation.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.DayPartType = reader.string();
          break;
        case 4:
          message.Tz = reader.string();
          break;
        case 5:
          message.UserTime = reader.string();
          break;
        case 6:
          message.Sunrise = reader.string();
          break;
        case 7:
          message.Sunset = reader.string();
          break;
        case 8:
          message.Temperature = reader.double();
          break;
        case 9:
          message.Icon = reader.string();
          break;
        case 10:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 11:
          message.TodayDaylight = TDaylightHours.decode(
            reader,
            reader.uint32()
          );
          break;
        case 12:
          message.IconType = reader.string();
          break;
        case 13:
          message.UserDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherDayPartData {
    return {
      Date: isSet(object.date) ? String(object.date) : "",
      GeoLocation: isSet(object.geo_location)
        ? TWeatherLocation.fromJSON(object.geo_location)
        : undefined,
      DayPartType: isSet(object.day_part_type)
        ? String(object.day_part_type)
        : "",
      Tz: isSet(object.tz) ? String(object.tz) : "",
      UserTime: isSet(object.user_time) ? String(object.user_time) : "",
      Sunrise: isSet(object.sunrise) ? String(object.sunrise) : "",
      Sunset: isSet(object.sunset) ? String(object.sunset) : "",
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      TodayDaylight: isSet(object.today_dayligth)
        ? TDaylightHours.fromJSON(object.today_dayligth)
        : undefined,
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
      UserDate: isSet(object.user_date) ? String(object.user_date) : "",
    };
  },

  toJSON(message: TWeatherDayPartData): unknown {
    const obj: any = {};
    message.Date !== undefined && (obj.date = message.Date);
    message.GeoLocation !== undefined &&
      (obj.geo_location = message.GeoLocation
        ? TWeatherLocation.toJSON(message.GeoLocation)
        : undefined);
    message.DayPartType !== undefined &&
      (obj.day_part_type = message.DayPartType);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.UserTime !== undefined && (obj.user_time = message.UserTime);
    message.Sunrise !== undefined && (obj.sunrise = message.Sunrise);
    message.Sunset !== undefined && (obj.sunset = message.Sunset);
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.TodayDaylight !== undefined &&
      (obj.today_dayligth = message.TodayDaylight
        ? TDaylightHours.toJSON(message.TodayDaylight)
        : undefined);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    message.UserDate !== undefined && (obj.user_date = message.UserDate);
    return obj;
  },
};

function createBaseTWeatherDayPartItem(): TWeatherDayPartItem {
  return {
    DayPartType: "",
    Temperature: 0,
    Icon: "",
    Condition: undefined,
    IconType: "",
  };
}

export const TWeatherDayPartItem = {
  encode(
    message: TWeatherDayPartItem,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DayPartType !== "") {
      writer.uint32(10).string(message.DayPartType);
    }
    if (message.Temperature !== 0) {
      writer.uint32(17).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(26).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.IconType !== "") {
      writer.uint32(42).string(message.IconType);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherDayPartItem {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherDayPartItem();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DayPartType = reader.string();
          break;
        case 2:
          message.Temperature = reader.double();
          break;
        case 3:
          message.Icon = reader.string();
          break;
        case 4:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 5:
          message.IconType = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherDayPartItem {
    return {
      DayPartType: isSet(object.day_part_type)
        ? String(object.day_part_type)
        : "",
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
    };
  },

  toJSON(message: TWeatherDayPartItem): unknown {
    const obj: any = {};
    message.DayPartType !== undefined &&
      (obj.day_part_type = message.DayPartType);
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    return obj;
  },
};

function createBaseTWeatherDayData(): TWeatherDayData {
  return {
    DayPartItems: [],
    Date: "",
    GeoLocation: undefined,
    Tz: "",
    UserTime: "",
    Sunrise: "",
    Sunset: "",
    Temperature: 0,
    Icon: "",
    Condition: undefined,
    TodayDaylight: undefined,
    IconType: "",
    UserDate: "",
  };
}

export const TWeatherDayData = {
  encode(message: TWeatherDayData, writer: Writer = Writer.create()): Writer {
    for (const v of message.DayPartItems) {
      TWeatherDayPartItem.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    if (message.Date !== "") {
      writer.uint32(18).string(message.Date);
    }
    if (message.GeoLocation !== undefined) {
      TWeatherLocation.encode(
        message.GeoLocation,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Tz !== "") {
      writer.uint32(34).string(message.Tz);
    }
    if (message.UserTime !== "") {
      writer.uint32(42).string(message.UserTime);
    }
    if (message.Sunrise !== "") {
      writer.uint32(50).string(message.Sunrise);
    }
    if (message.Sunset !== "") {
      writer.uint32(58).string(message.Sunset);
    }
    if (message.Temperature !== 0) {
      writer.uint32(65).double(message.Temperature);
    }
    if (message.Icon !== "") {
      writer.uint32(74).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(82).fork()
      ).ldelim();
    }
    if (message.TodayDaylight !== undefined) {
      TDaylightHours.encode(
        message.TodayDaylight,
        writer.uint32(90).fork()
      ).ldelim();
    }
    if (message.IconType !== "") {
      writer.uint32(98).string(message.IconType);
    }
    if (message.UserDate !== "") {
      writer.uint32(106).string(message.UserDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherDayData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherDayData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DayPartItems.push(
            TWeatherDayPartItem.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.Date = reader.string();
          break;
        case 3:
          message.GeoLocation = TWeatherLocation.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.Tz = reader.string();
          break;
        case 5:
          message.UserTime = reader.string();
          break;
        case 6:
          message.Sunrise = reader.string();
          break;
        case 7:
          message.Sunset = reader.string();
          break;
        case 8:
          message.Temperature = reader.double();
          break;
        case 9:
          message.Icon = reader.string();
          break;
        case 10:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 11:
          message.TodayDaylight = TDaylightHours.decode(
            reader,
            reader.uint32()
          );
          break;
        case 12:
          message.IconType = reader.string();
          break;
        case 13:
          message.UserDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherDayData {
    return {
      DayPartItems: Array.isArray(object?.day_part_items)
        ? object.day_part_items.map((e: any) => TWeatherDayPartItem.fromJSON(e))
        : [],
      Date: isSet(object.date) ? String(object.date) : "",
      GeoLocation: isSet(object.geo_location)
        ? TWeatherLocation.fromJSON(object.geo_location)
        : undefined,
      Tz: isSet(object.tz) ? String(object.tz) : "",
      UserTime: isSet(object.user_time) ? String(object.user_time) : "",
      Sunrise: isSet(object.sunrise) ? String(object.sunrise) : "",
      Sunset: isSet(object.sunset) ? String(object.sunset) : "",
      Temperature: isSet(object.temperature) ? Number(object.temperature) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      TodayDaylight: isSet(object.today_dayligth)
        ? TDaylightHours.fromJSON(object.today_dayligth)
        : undefined,
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
      UserDate: isSet(object.user_date) ? String(object.user_date) : "",
    };
  },

  toJSON(message: TWeatherDayData): unknown {
    const obj: any = {};
    if (message.DayPartItems) {
      obj.day_part_items = message.DayPartItems.map((e) =>
        e ? TWeatherDayPartItem.toJSON(e) : undefined
      );
    } else {
      obj.day_part_items = [];
    }
    message.Date !== undefined && (obj.date = message.Date);
    message.GeoLocation !== undefined &&
      (obj.geo_location = message.GeoLocation
        ? TWeatherLocation.toJSON(message.GeoLocation)
        : undefined);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.UserTime !== undefined && (obj.user_time = message.UserTime);
    message.Sunrise !== undefined && (obj.sunrise = message.Sunrise);
    message.Sunset !== undefined && (obj.sunset = message.Sunset);
    message.Temperature !== undefined &&
      (obj.temperature = message.Temperature);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.TodayDaylight !== undefined &&
      (obj.today_dayligth = message.TodayDaylight
        ? TDaylightHours.toJSON(message.TodayDaylight)
        : undefined);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    message.UserDate !== undefined && (obj.user_date = message.UserDate);
    return obj;
  },
};

function createBaseTWeatherDayItem(): TWeatherDayItem {
  return {
    Date: "",
    Tz: "",
    WeekDay: 0,
    DayTemp: 0,
    NightTemp: 0,
    Icon: "",
    Condition: undefined,
    Url: "",
    IconType: "",
  };
}

export const TWeatherDayItem = {
  encode(message: TWeatherDayItem, writer: Writer = Writer.create()): Writer {
    if (message.Date !== "") {
      writer.uint32(10).string(message.Date);
    }
    if (message.Tz !== "") {
      writer.uint32(18).string(message.Tz);
    }
    if (message.WeekDay !== 0) {
      writer.uint32(24).uint32(message.WeekDay);
    }
    if (message.DayTemp !== 0) {
      writer.uint32(33).double(message.DayTemp);
    }
    if (message.NightTemp !== 0) {
      writer.uint32(41).double(message.NightTemp);
    }
    if (message.Icon !== "") {
      writer.uint32(50).string(message.Icon);
    }
    if (message.Condition !== undefined) {
      TWeatherCondition.encode(
        message.Condition,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.Url !== "") {
      writer.uint32(66).string(message.Url);
    }
    if (message.IconType !== "") {
      writer.uint32(74).string(message.IconType);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherDayItem {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherDayItem();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Date = reader.string();
          break;
        case 2:
          message.Tz = reader.string();
          break;
        case 3:
          message.WeekDay = reader.uint32();
          break;
        case 4:
          message.DayTemp = reader.double();
          break;
        case 5:
          message.NightTemp = reader.double();
          break;
        case 6:
          message.Icon = reader.string();
          break;
        case 7:
          message.Condition = TWeatherCondition.decode(reader, reader.uint32());
          break;
        case 8:
          message.Url = reader.string();
          break;
        case 9:
          message.IconType = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherDayItem {
    return {
      Date: isSet(object.date) ? String(object.date) : "",
      Tz: isSet(object.tz) ? String(object.tz) : "",
      WeekDay: isSet(object.week_day) ? Number(object.week_day) : 0,
      DayTemp: isSet(object.day_temp) ? Number(object.day_temp) : 0,
      NightTemp: isSet(object.night_temp) ? Number(object.night_temp) : 0,
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Condition: isSet(object.condition)
        ? TWeatherCondition.fromJSON(object.condition)
        : undefined,
      Url: isSet(object.url) ? String(object.url) : "",
      IconType: isSet(object.icon_type) ? String(object.icon_type) : "",
    };
  },

  toJSON(message: TWeatherDayItem): unknown {
    const obj: any = {};
    message.Date !== undefined && (obj.date = message.Date);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.WeekDay !== undefined &&
      (obj.week_day = Math.round(message.WeekDay));
    message.DayTemp !== undefined && (obj.day_temp = message.DayTemp);
    message.NightTemp !== undefined && (obj.night_temp = message.NightTemp);
    message.Icon !== undefined && (obj.icon = message.Icon);
    message.Condition !== undefined &&
      (obj.condition = message.Condition
        ? TWeatherCondition.toJSON(message.Condition)
        : undefined);
    message.Url !== undefined && (obj.url = message.Url);
    message.IconType !== undefined && (obj.icon_type = message.IconType);
    return obj;
  },
};

function createBaseTWeatherDaysRangeData(): TWeatherDaysRangeData {
  return {
    DayItems: [],
    GeoLocation: undefined,
    Tz: "",
    UserTime: "",
    TodayDaylight: undefined,
    UserDate: "",
  };
}

export const TWeatherDaysRangeData = {
  encode(
    message: TWeatherDaysRangeData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.DayItems) {
      TWeatherDayItem.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    if (message.GeoLocation !== undefined) {
      TWeatherLocation.encode(
        message.GeoLocation,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Tz !== "") {
      writer.uint32(26).string(message.Tz);
    }
    if (message.UserTime !== "") {
      writer.uint32(34).string(message.UserTime);
    }
    if (message.TodayDaylight !== undefined) {
      TDaylightHours.encode(
        message.TodayDaylight,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.UserDate !== "") {
      writer.uint32(50).string(message.UserDate);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherDaysRangeData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherDaysRangeData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DayItems.push(
            TWeatherDayItem.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.GeoLocation = TWeatherLocation.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.Tz = reader.string();
          break;
        case 4:
          message.UserTime = reader.string();
          break;
        case 5:
          message.TodayDaylight = TDaylightHours.decode(
            reader,
            reader.uint32()
          );
          break;
        case 6:
          message.UserDate = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherDaysRangeData {
    return {
      DayItems: Array.isArray(object?.day_items)
        ? object.day_items.map((e: any) => TWeatherDayItem.fromJSON(e))
        : [],
      GeoLocation: isSet(object.geo_location)
        ? TWeatherLocation.fromJSON(object.geo_location)
        : undefined,
      Tz: isSet(object.tz) ? String(object.tz) : "",
      UserTime: isSet(object.user_time) ? String(object.user_time) : "",
      TodayDaylight: isSet(object.today_dayligth)
        ? TDaylightHours.fromJSON(object.today_dayligth)
        : undefined,
      UserDate: isSet(object.user_date) ? String(object.user_date) : "",
    };
  },

  toJSON(message: TWeatherDaysRangeData): unknown {
    const obj: any = {};
    if (message.DayItems) {
      obj.day_items = message.DayItems.map((e) =>
        e ? TWeatherDayItem.toJSON(e) : undefined
      );
    } else {
      obj.day_items = [];
    }
    message.GeoLocation !== undefined &&
      (obj.geo_location = message.GeoLocation
        ? TWeatherLocation.toJSON(message.GeoLocation)
        : undefined);
    message.Tz !== undefined && (obj.tz = message.Tz);
    message.UserTime !== undefined && (obj.user_time = message.UserTime);
    message.TodayDaylight !== undefined &&
      (obj.today_dayligth = message.TodayDaylight
        ? TDaylightHours.toJSON(message.TodayDaylight)
        : undefined);
    message.UserDate !== undefined && (obj.user_date = message.UserDate);
    return obj;
  },
};

declare var self: any | undefined;
declare var window: any | undefined;
declare var global: any | undefined;
var globalThis: any = (() => {
  if (typeof globalThis !== "undefined") return globalThis;
  if (typeof self !== "undefined") return self;
  if (typeof window !== "undefined") return window;
  if (typeof global !== "undefined") return global;
  throw "Unable to locate global object";
})();

function longToNumber(long: Long): number {
  if (long.gt(Number.MAX_SAFE_INTEGER)) {
    throw new globalThis.Error("Value is larger than Number.MAX_SAFE_INTEGER");
  }
  return long.toNumber();
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
