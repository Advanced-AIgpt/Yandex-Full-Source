/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import { TNewsTeaserData } from "../../../../../../alice/protos/data/scenario/news/news";
import { TScreenSaverData } from "../../../../../../alice/protos/data/scenario/photoframe/screen_saver";
import { TAfishaTeaserData } from "../../../../../../alice/protos/data/scenario/afisha/afisha";
import { TWeatherTeaserData } from "../../../../../../alice/protos/data/scenario/weather/weather";
import { TDialogovoSkillTeaserData } from "../../../../../../alice/protos/data/scenario/dialogovo/skill";

export const protobufPackage = "NAlice.NData";

export interface TCentaurTeaserConfigData {
  TeaserType: string;
  TeaserId: string;
}

export interface TTeaserSettingsData {
  TeaserSettings: TTeaserSettingsData_TeaserSetting[];
}

export interface TTeaserSettingsData_TeaserSetting {
  TeaserConfigData?: TCentaurTeaserConfigData;
  IsChosen: boolean;
}

export interface TTeaserPreviewScenarioData {
  NewsTeaserData?: TNewsTeaserData | undefined;
  ScreenSaverData?: TScreenSaverData | undefined;
  AfishaTeaserData?: TAfishaTeaserData | undefined;
  WeatherTeaserData?: TWeatherTeaserData | undefined;
  DialogovoTeaserCardData?: TDialogovoSkillTeaserData | undefined;
}

export interface TTeaserSettingsWithContentData {
  TeaserSettingsWithPreview: TTeaserSettingsWithContentData_TeaserSettingWithPreview[];
}

export interface TTeaserSettingsWithContentData_TeaserSettingWithPreview {
  TeaserConfigData?: TCentaurTeaserConfigData;
  IsChosen: boolean;
  TeaserName: string;
  TeaserPreviewScenarioData?: TTeaserPreviewScenarioData;
}

export interface TTeasersPreviewData {
  TeaserPreviews: TTeasersPreviewData_TTeaserPreview[];
}

export interface TTeasersPreviewData_TTeaserPreview {
  TeaserConfigData?: TCentaurTeaserConfigData;
  TeaserName: string;
  TeaserPreviewScenarioData?: TTeaserPreviewScenarioData;
}

function createBaseTCentaurTeaserConfigData(): TCentaurTeaserConfigData {
  return { TeaserType: "", TeaserId: "" };
}

export const TCentaurTeaserConfigData = {
  encode(
    message: TCentaurTeaserConfigData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TeaserType !== "") {
      writer.uint32(10).string(message.TeaserType);
    }
    if (message.TeaserId !== "") {
      writer.uint32(18).string(message.TeaserId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurTeaserConfigData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurTeaserConfigData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserType = reader.string();
          break;
        case 2:
          message.TeaserId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCentaurTeaserConfigData {
    return {
      TeaserType: isSet(object.teaser_type) ? String(object.teaser_type) : "",
      TeaserId: isSet(object.teaser_id) ? String(object.teaser_id) : "",
    };
  },

  toJSON(message: TCentaurTeaserConfigData): unknown {
    const obj: any = {};
    message.TeaserType !== undefined && (obj.teaser_type = message.TeaserType);
    message.TeaserId !== undefined && (obj.teaser_id = message.TeaserId);
    return obj;
  },
};

function createBaseTTeaserSettingsData(): TTeaserSettingsData {
  return { TeaserSettings: [] };
}

export const TTeaserSettingsData = {
  encode(
    message: TTeaserSettingsData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.TeaserSettings) {
      TTeaserSettingsData_TeaserSetting.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTeaserSettingsData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTeaserSettingsData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserSettings.push(
            TTeaserSettingsData_TeaserSetting.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTeaserSettingsData {
    return {
      TeaserSettings: Array.isArray(object?.teaser_settings)
        ? object.teaser_settings.map((e: any) =>
            TTeaserSettingsData_TeaserSetting.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TTeaserSettingsData): unknown {
    const obj: any = {};
    if (message.TeaserSettings) {
      obj.teaser_settings = message.TeaserSettings.map((e) =>
        e ? TTeaserSettingsData_TeaserSetting.toJSON(e) : undefined
      );
    } else {
      obj.teaser_settings = [];
    }
    return obj;
  },
};

function createBaseTTeaserSettingsData_TeaserSetting(): TTeaserSettingsData_TeaserSetting {
  return { TeaserConfigData: undefined, IsChosen: false };
}

export const TTeaserSettingsData_TeaserSetting = {
  encode(
    message: TTeaserSettingsData_TeaserSetting,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TeaserConfigData !== undefined) {
      TCentaurTeaserConfigData.encode(
        message.TeaserConfigData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.IsChosen === true) {
      writer.uint32(24).bool(message.IsChosen);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTeaserSettingsData_TeaserSetting {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTeaserSettingsData_TeaserSetting();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserConfigData = TCentaurTeaserConfigData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.IsChosen = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTeaserSettingsData_TeaserSetting {
    return {
      TeaserConfigData: isSet(object.teaser_config_data)
        ? TCentaurTeaserConfigData.fromJSON(object.teaser_config_data)
        : undefined,
      IsChosen: isSet(object.is_chosen) ? Boolean(object.is_chosen) : false,
    };
  },

  toJSON(message: TTeaserSettingsData_TeaserSetting): unknown {
    const obj: any = {};
    message.TeaserConfigData !== undefined &&
      (obj.teaser_config_data = message.TeaserConfigData
        ? TCentaurTeaserConfigData.toJSON(message.TeaserConfigData)
        : undefined);
    message.IsChosen !== undefined && (obj.is_chosen = message.IsChosen);
    return obj;
  },
};

function createBaseTTeaserPreviewScenarioData(): TTeaserPreviewScenarioData {
  return {
    NewsTeaserData: undefined,
    ScreenSaverData: undefined,
    AfishaTeaserData: undefined,
    WeatherTeaserData: undefined,
    DialogovoTeaserCardData: undefined,
  };
}

export const TTeaserPreviewScenarioData = {
  encode(
    message: TTeaserPreviewScenarioData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.NewsTeaserData !== undefined) {
      TNewsTeaserData.encode(
        message.NewsTeaserData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.ScreenSaverData !== undefined) {
      TScreenSaverData.encode(
        message.ScreenSaverData,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.AfishaTeaserData !== undefined) {
      TAfishaTeaserData.encode(
        message.AfishaTeaserData,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.WeatherTeaserData !== undefined) {
      TWeatherTeaserData.encode(
        message.WeatherTeaserData,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.DialogovoTeaserCardData !== undefined) {
      TDialogovoSkillTeaserData.encode(
        message.DialogovoTeaserCardData,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTeaserPreviewScenarioData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTeaserPreviewScenarioData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsTeaserData = TNewsTeaserData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.ScreenSaverData = TScreenSaverData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.AfishaTeaserData = TAfishaTeaserData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.WeatherTeaserData = TWeatherTeaserData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.DialogovoTeaserCardData = TDialogovoSkillTeaserData.decode(
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

  fromJSON(object: any): TTeaserPreviewScenarioData {
    return {
      NewsTeaserData: isSet(object.news_teaser_data)
        ? TNewsTeaserData.fromJSON(object.news_teaser_data)
        : undefined,
      ScreenSaverData: isSet(object.screen_saver_data)
        ? TScreenSaverData.fromJSON(object.screen_saver_data)
        : undefined,
      AfishaTeaserData: isSet(object.afisha_teaser_data)
        ? TAfishaTeaserData.fromJSON(object.afisha_teaser_data)
        : undefined,
      WeatherTeaserData: isSet(object.weather_teaser_data)
        ? TWeatherTeaserData.fromJSON(object.weather_teaser_data)
        : undefined,
      DialogovoTeaserCardData: isSet(object.dialogovo_teaser_card_data)
        ? TDialogovoSkillTeaserData.fromJSON(object.dialogovo_teaser_card_data)
        : undefined,
    };
  },

  toJSON(message: TTeaserPreviewScenarioData): unknown {
    const obj: any = {};
    message.NewsTeaserData !== undefined &&
      (obj.news_teaser_data = message.NewsTeaserData
        ? TNewsTeaserData.toJSON(message.NewsTeaserData)
        : undefined);
    message.ScreenSaverData !== undefined &&
      (obj.screen_saver_data = message.ScreenSaverData
        ? TScreenSaverData.toJSON(message.ScreenSaverData)
        : undefined);
    message.AfishaTeaserData !== undefined &&
      (obj.afisha_teaser_data = message.AfishaTeaserData
        ? TAfishaTeaserData.toJSON(message.AfishaTeaserData)
        : undefined);
    message.WeatherTeaserData !== undefined &&
      (obj.weather_teaser_data = message.WeatherTeaserData
        ? TWeatherTeaserData.toJSON(message.WeatherTeaserData)
        : undefined);
    message.DialogovoTeaserCardData !== undefined &&
      (obj.dialogovo_teaser_card_data = message.DialogovoTeaserCardData
        ? TDialogovoSkillTeaserData.toJSON(message.DialogovoTeaserCardData)
        : undefined);
    return obj;
  },
};

function createBaseTTeaserSettingsWithContentData(): TTeaserSettingsWithContentData {
  return { TeaserSettingsWithPreview: [] };
}

export const TTeaserSettingsWithContentData = {
  encode(
    message: TTeaserSettingsWithContentData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.TeaserSettingsWithPreview) {
      TTeaserSettingsWithContentData_TeaserSettingWithPreview.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTeaserSettingsWithContentData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTeaserSettingsWithContentData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserSettingsWithPreview.push(
            TTeaserSettingsWithContentData_TeaserSettingWithPreview.decode(
              reader,
              reader.uint32()
            )
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTeaserSettingsWithContentData {
    return {
      TeaserSettingsWithPreview: Array.isArray(
        object?.teaser_settings_with_preview
      )
        ? object.teaser_settings_with_preview.map((e: any) =>
            TTeaserSettingsWithContentData_TeaserSettingWithPreview.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TTeaserSettingsWithContentData): unknown {
    const obj: any = {};
    if (message.TeaserSettingsWithPreview) {
      obj.teaser_settings_with_preview = message.TeaserSettingsWithPreview.map(
        (e) =>
          e
            ? TTeaserSettingsWithContentData_TeaserSettingWithPreview.toJSON(e)
            : undefined
      );
    } else {
      obj.teaser_settings_with_preview = [];
    }
    return obj;
  },
};

function createBaseTTeaserSettingsWithContentData_TeaserSettingWithPreview(): TTeaserSettingsWithContentData_TeaserSettingWithPreview {
  return {
    TeaserConfigData: undefined,
    IsChosen: false,
    TeaserName: "",
    TeaserPreviewScenarioData: undefined,
  };
}

export const TTeaserSettingsWithContentData_TeaserSettingWithPreview = {
  encode(
    message: TTeaserSettingsWithContentData_TeaserSettingWithPreview,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TeaserConfigData !== undefined) {
      TCentaurTeaserConfigData.encode(
        message.TeaserConfigData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.IsChosen === true) {
      writer.uint32(16).bool(message.IsChosen);
    }
    if (message.TeaserName !== "") {
      writer.uint32(26).string(message.TeaserName);
    }
    if (message.TeaserPreviewScenarioData !== undefined) {
      TTeaserPreviewScenarioData.encode(
        message.TeaserPreviewScenarioData,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTeaserSettingsWithContentData_TeaserSettingWithPreview {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTTeaserSettingsWithContentData_TeaserSettingWithPreview();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserConfigData = TCentaurTeaserConfigData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.IsChosen = reader.bool();
          break;
        case 3:
          message.TeaserName = reader.string();
          break;
        case 4:
          message.TeaserPreviewScenarioData = TTeaserPreviewScenarioData.decode(
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

  fromJSON(
    object: any
  ): TTeaserSettingsWithContentData_TeaserSettingWithPreview {
    return {
      TeaserConfigData: isSet(object.teaser_config_data)
        ? TCentaurTeaserConfigData.fromJSON(object.teaser_config_data)
        : undefined,
      IsChosen: isSet(object.is_chosen) ? Boolean(object.is_chosen) : false,
      TeaserName: isSet(object.teaser_name) ? String(object.teaser_name) : "",
      TeaserPreviewScenarioData: isSet(object.teaser_preview_scenario_data)
        ? TTeaserPreviewScenarioData.fromJSON(
            object.teaser_preview_scenario_data
          )
        : undefined,
    };
  },

  toJSON(
    message: TTeaserSettingsWithContentData_TeaserSettingWithPreview
  ): unknown {
    const obj: any = {};
    message.TeaserConfigData !== undefined &&
      (obj.teaser_config_data = message.TeaserConfigData
        ? TCentaurTeaserConfigData.toJSON(message.TeaserConfigData)
        : undefined);
    message.IsChosen !== undefined && (obj.is_chosen = message.IsChosen);
    message.TeaserName !== undefined && (obj.teaser_name = message.TeaserName);
    message.TeaserPreviewScenarioData !== undefined &&
      (obj.teaser_preview_scenario_data = message.TeaserPreviewScenarioData
        ? TTeaserPreviewScenarioData.toJSON(message.TeaserPreviewScenarioData)
        : undefined);
    return obj;
  },
};

function createBaseTTeasersPreviewData(): TTeasersPreviewData {
  return { TeaserPreviews: [] };
}

export const TTeasersPreviewData = {
  encode(
    message: TTeasersPreviewData,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.TeaserPreviews) {
      TTeasersPreviewData_TTeaserPreview.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTeasersPreviewData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTeasersPreviewData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserPreviews.push(
            TTeasersPreviewData_TTeaserPreview.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTeasersPreviewData {
    return {
      TeaserPreviews: Array.isArray(object?.teaser_previews)
        ? object.teaser_previews.map((e: any) =>
            TTeasersPreviewData_TTeaserPreview.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TTeasersPreviewData): unknown {
    const obj: any = {};
    if (message.TeaserPreviews) {
      obj.teaser_previews = message.TeaserPreviews.map((e) =>
        e ? TTeasersPreviewData_TTeaserPreview.toJSON(e) : undefined
      );
    } else {
      obj.teaser_previews = [];
    }
    return obj;
  },
};

function createBaseTTeasersPreviewData_TTeaserPreview(): TTeasersPreviewData_TTeaserPreview {
  return {
    TeaserConfigData: undefined,
    TeaserName: "",
    TeaserPreviewScenarioData: undefined,
  };
}

export const TTeasersPreviewData_TTeaserPreview = {
  encode(
    message: TTeasersPreviewData_TTeaserPreview,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TeaserConfigData !== undefined) {
      TCentaurTeaserConfigData.encode(
        message.TeaserConfigData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.TeaserName !== "") {
      writer.uint32(18).string(message.TeaserName);
    }
    if (message.TeaserPreviewScenarioData !== undefined) {
      TTeaserPreviewScenarioData.encode(
        message.TeaserPreviewScenarioData,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTeasersPreviewData_TTeaserPreview {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTeasersPreviewData_TTeaserPreview();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserConfigData = TCentaurTeaserConfigData.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.TeaserName = reader.string();
          break;
        case 3:
          message.TeaserPreviewScenarioData = TTeaserPreviewScenarioData.decode(
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

  fromJSON(object: any): TTeasersPreviewData_TTeaserPreview {
    return {
      TeaserConfigData: isSet(object.teaser_config_data)
        ? TCentaurTeaserConfigData.fromJSON(object.teaser_config_data)
        : undefined,
      TeaserName: isSet(object.teaser_name) ? String(object.teaser_name) : "",
      TeaserPreviewScenarioData: isSet(object.teaser_preview_scenario_data)
        ? TTeaserPreviewScenarioData.fromJSON(
            object.teaser_preview_scenario_data
          )
        : undefined,
    };
  },

  toJSON(message: TTeasersPreviewData_TTeaserPreview): unknown {
    const obj: any = {};
    message.TeaserConfigData !== undefined &&
      (obj.teaser_config_data = message.TeaserConfigData
        ? TCentaurTeaserConfigData.toJSON(message.TeaserConfigData)
        : undefined);
    message.TeaserName !== undefined && (obj.teaser_name = message.TeaserName);
    message.TeaserPreviewScenarioData !== undefined &&
      (obj.teaser_preview_scenario_data = message.TeaserPreviewScenarioData
        ? TTeaserPreviewScenarioData.toJSON(message.TeaserPreviewScenarioData)
        : undefined);
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
