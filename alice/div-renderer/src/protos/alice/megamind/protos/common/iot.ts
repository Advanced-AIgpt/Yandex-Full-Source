/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import {
  EUserDeviceType,
  TUserSharingInfo,
  eUserDeviceTypeFromJSON,
  eUserDeviceTypeToJSON,
} from "../../../../alice/protos/data/device/info";
import {
  TIotDiscoveryCapability_TNetworks,
  TIotDiscoveryCapability_TProtocol,
  tIotDiscoveryCapability_TProtocolFromJSON,
  tIotDiscoveryCapability_TProtocolToJSON,
} from "../../../../alice/protos/endpoint/capability";
import { TUserRoom } from "../../../../alice/protos/data/location/room";
import { TUserGroup } from "../../../../alice/protos/data/location/group";
import { TEndpoint } from "../../../../alice/protos/endpoint/endpoint";

export const protobufPackage = "NAlice";

export interface TIoTUserInfo {
  Rooms: TUserRoom[];
  Groups: TUserGroup[];
  Devices: TIoTUserInfo_TDevice[];
  Scenarios: TIoTUserInfo_TScenario[];
  Colors: TIoTUserInfo_TColor[];
  Households: TIoTUserInfo_THousehold[];
  Stereopairs: TIoTUserInfo_TStereopair[];
  CurrentHouseholdId: string;
  RawUserInfo: string;
}

export interface TIoTUserInfo_TCapability {
  Type: TIoTUserInfo_TCapability_ECapabilityType;
  Retrievable: boolean;
  Reportable: boolean;
  LastUpdated: number;
  AnalyticsType: string;
  AnalyticsName: string;
  OnOffCapabilityParameters?:
    | TIoTUserInfo_TCapability_TOnOffCapabilityParameters
    | undefined;
  ColorSettingCapabilityParameters?:
    | TIoTUserInfo_TCapability_TColorSettingCapabilityParameters
    | undefined;
  ModeCapabilityParameters?:
    | TIoTUserInfo_TCapability_TModeCapabilityParameters
    | undefined;
  RangeCapabilityParameters?:
    | TIoTUserInfo_TCapability_TRangeCapabilityParameters
    | undefined;
  ToggleCapabilityParameters?:
    | TIoTUserInfo_TCapability_TToggleCapabilityParameters
    | undefined;
  CustomButtonCapabilityParameters?:
    | TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters
    | undefined;
  QuasarServerActionCapabilityParameters?:
    | TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters
    | undefined;
  QuasarCapabilityParameters?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityParameters
    | undefined;
  VideoStreamCapabilityParameters?:
    | TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters
    | undefined;
  OnOffCapabilityState?:
    | TIoTUserInfo_TCapability_TOnOffCapabilityState
    | undefined;
  ColorSettingCapabilityState?:
    | TIoTUserInfo_TCapability_TColorSettingCapabilityState
    | undefined;
  ModeCapabilityState?:
    | TIoTUserInfo_TCapability_TModeCapabilityState
    | undefined;
  RangeCapabilityState?:
    | TIoTUserInfo_TCapability_TRangeCapabilityState
    | undefined;
  ToggleCapabilityState?:
    | TIoTUserInfo_TCapability_TToggleCapabilityState
    | undefined;
  CustomButtonCapabilityState?:
    | TIoTUserInfo_TCapability_TCustomButtonCapabilityState
    | undefined;
  QuasarServerActionCapabilityState?:
    | TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState
    | undefined;
  QuasarCapabilityState?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState
    | undefined;
  VideoStreamCapabilityState?:
    | TIoTUserInfo_TCapability_TVideoStreamCapabilityState
    | undefined;
}

export enum TIoTUserInfo_TCapability_ECapabilityType {
  UnknownCapabilityType = 0,
  OnOffCapabilityType = 1,
  ColorSettingCapabilityType = 2,
  ModeCapabilityType = 3,
  RangeCapabilityType = 4,
  ToggleCapabilityType = 5,
  CustomButtonCapabilityType = 6,
  QuasarServerActionCapabilityType = 7,
  QuasarCapabilityType = 8,
  VideoStreamCapabilityType = 9,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TCapability_ECapabilityTypeFromJSON(
  object: any
): TIoTUserInfo_TCapability_ECapabilityType {
  switch (object) {
    case 0:
    case "UnknownCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.UnknownCapabilityType;
    case 1:
    case "OnOffCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.OnOffCapabilityType;
    case 2:
    case "ColorSettingCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.ColorSettingCapabilityType;
    case 3:
    case "ModeCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.ModeCapabilityType;
    case 4:
    case "RangeCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.RangeCapabilityType;
    case 5:
    case "ToggleCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.ToggleCapabilityType;
    case 6:
    case "CustomButtonCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.CustomButtonCapabilityType;
    case 7:
    case "QuasarServerActionCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.QuasarServerActionCapabilityType;
    case 8:
    case "QuasarCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.QuasarCapabilityType;
    case 9:
    case "VideoStreamCapabilityType":
      return TIoTUserInfo_TCapability_ECapabilityType.VideoStreamCapabilityType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TCapability_ECapabilityType.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TCapability_ECapabilityTypeToJSON(
  object: TIoTUserInfo_TCapability_ECapabilityType
): string {
  switch (object) {
    case TIoTUserInfo_TCapability_ECapabilityType.UnknownCapabilityType:
      return "UnknownCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.OnOffCapabilityType:
      return "OnOffCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.ColorSettingCapabilityType:
      return "ColorSettingCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.ModeCapabilityType:
      return "ModeCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.RangeCapabilityType:
      return "RangeCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.ToggleCapabilityType:
      return "ToggleCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.CustomButtonCapabilityType:
      return "CustomButtonCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.QuasarServerActionCapabilityType:
      return "QuasarServerActionCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.QuasarCapabilityType:
      return "QuasarCapabilityType";
    case TIoTUserInfo_TCapability_ECapabilityType.VideoStreamCapabilityType:
      return "VideoStreamCapabilityType";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TCapability_TOnOffCapabilityParameters {
  Split: boolean;
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityParameters {
  ColorModel?: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel;
  TemperatureK?: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters;
  ColorSceneParameters?: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters;
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel {
  Type: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType;
  AnalyticsName: string;
}

export enum TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType {
  UnknownColorModel = 0,
  HsvColorModel = 1,
  RgbColorModel = 2,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelTypeFromJSON(
  object: any
): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType {
  switch (object) {
    case 0:
    case "UnknownColorModel":
      return TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.UnknownColorModel;
    case 1:
    case "HsvColorModel":
      return TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.HsvColorModel;
    case 2:
    case "RgbColorModel":
      return TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.RgbColorModel;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelTypeToJSON(
  object: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType
): string {
  switch (object) {
    case TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.UnknownColorModel:
      return "UnknownColorModel";
    case TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.HsvColorModel:
      return "HsvColorModel";
    case TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType.RgbColorModel:
      return "RgbColorModel";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters {
  Min: number;
  Max: number;
  AnalyticsName: string;
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene {
  ID: string;
  Name: string;
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters {
  Scenes: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene[];
  AnalyticsName: string;
}

export interface TIoTUserInfo_TCapability_TModeCapabilityParameters {
  Instance: string;
  Modes: TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode[];
}

export interface TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode {
  Name: string;
  Value: string;
}

export interface TIoTUserInfo_TCapability_TRangeCapabilityParameters {
  Instance: string;
  Unit: string;
  RandomAccess: boolean;
  Looped: boolean;
  Range?: TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange;
}

export interface TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange {
  Min: number;
  Max: number;
  Precision: number;
}

export interface TIoTUserInfo_TCapability_TToggleCapabilityParameters {
  Instance: string;
}

export interface TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters {
  Instance: string;
  InstanceNames: string[];
}

export interface TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters {
  Instance: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityParameters {
  Instance: string;
}

export interface TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters {
  Protocols: string[];
}

export interface TIoTUserInfo_TCapability_TOnOffCapabilityState {
  Instance: string;
  Value: boolean;
  Relative?: TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative;
}

export interface TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative {
  IsRelative: boolean;
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityState {
  Instance: string;
  TemperatureK: number | undefined;
  RGB: number | undefined;
  HSV?: TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV | undefined;
  ColorSceneID: string | undefined;
}

export interface TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV {
  H: number;
  S: number;
  V: number;
}

export interface TIoTUserInfo_TCapability_TRangeCapabilityState {
  Instance: string;
  Value: number;
  Relative?: TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative;
}

export interface TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative {
  IsRelative: boolean;
}

export interface TIoTUserInfo_TCapability_TModeCapabilityState {
  Instance: string;
  Value: string;
}

export interface TIoTUserInfo_TCapability_TToggleCapabilityState {
  Instance: string;
  Value: boolean;
}

export interface TIoTUserInfo_TCapability_TCustomButtonCapabilityState {
  Instance: string;
  Value: boolean;
}

export interface TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState {
  Instance: string;
  Value: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState {
  Instance: string;
  MusicPlayValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue
    | undefined;
  NewsValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue
    | undefined;
  SoundPlayValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue
    | undefined;
  StopEverythingValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue
    | undefined;
  VolumeValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue
    | undefined;
  WeatherValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue
    | undefined;
  TtsValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue
    | undefined;
  AliceShowValue?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue
    | undefined;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue {
  Object?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject
    | undefined;
  SearchText: string | undefined;
  PlayInBackground: boolean;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject {
  Id: string;
  Type: string;
  Name: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue {
  Topic: string;
  Provider: string;
  PlayInBackground: boolean;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue {
  Sound: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue {}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue {
  Value: number;
  Relative: boolean;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue {
  Where?: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation;
  Household?: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation {
  Longitude: number;
  Latitude: number;
  Address: string;
  ShortAddress: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo {
  Id: string;
  Name: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue {
  Text: string;
}

export interface TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue {}

export interface TIoTUserInfo_TCapability_TVideoStreamCapabilityState {
  Instance: string;
  Value?: TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue;
}

export interface TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue {
  Protocol: string;
  StreamURL: string;
  /** unix timestamp in seconds */
  ExpirationTime: number;
  Protocols: string[];
}

export interface TIoTUserInfo_TProperty {
  Type: TIoTUserInfo_TProperty_EPropertyType;
  Retrievable: boolean;
  Reportable: boolean;
  /** timestamp of the last value update */
  StateChangedAt: number;
  /** timestamp of any state update */
  LastUpdated: number;
  /**
   * timestamp of particular state update, it's actual only for event property now and changes only when
   * some particular event occur, i.e. only "opened" event updates this field for open/close sensor
   */
  LastActivated: number;
  AnalyticsType: string;
  AnalyticsName: string;
  FloatPropertyParameters?:
    | TIoTUserInfo_TProperty_TFloatPropertyParameters
    | undefined;
  BoolPropertyParameters?:
    | TIoTUserInfo_TProperty_TBoolPropertyParameters
    | undefined;
  EventPropertyParameters?:
    | TIoTUserInfo_TProperty_TEventPropertyParameters
    | undefined;
  BoolPropertyState?: TIoTUserInfo_TProperty_TBoolPropertyState | undefined;
  FloatPropertyState?: TIoTUserInfo_TProperty_TFloatPropertyState | undefined;
  EventPropertyState?: TIoTUserInfo_TProperty_TEventPropertyState | undefined;
}

export enum TIoTUserInfo_TProperty_EPropertyType {
  UnknownPropertyType = 0,
  FloatPropertyType = 1,
  BoolPropertyType = 2,
  EventPropertyType = 3,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TProperty_EPropertyTypeFromJSON(
  object: any
): TIoTUserInfo_TProperty_EPropertyType {
  switch (object) {
    case 0:
    case "UnknownPropertyType":
      return TIoTUserInfo_TProperty_EPropertyType.UnknownPropertyType;
    case 1:
    case "FloatPropertyType":
      return TIoTUserInfo_TProperty_EPropertyType.FloatPropertyType;
    case 2:
    case "BoolPropertyType":
      return TIoTUserInfo_TProperty_EPropertyType.BoolPropertyType;
    case 3:
    case "EventPropertyType":
      return TIoTUserInfo_TProperty_EPropertyType.EventPropertyType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TProperty_EPropertyType.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TProperty_EPropertyTypeToJSON(
  object: TIoTUserInfo_TProperty_EPropertyType
): string {
  switch (object) {
    case TIoTUserInfo_TProperty_EPropertyType.UnknownPropertyType:
      return "UnknownPropertyType";
    case TIoTUserInfo_TProperty_EPropertyType.FloatPropertyType:
      return "FloatPropertyType";
    case TIoTUserInfo_TProperty_EPropertyType.BoolPropertyType:
      return "BoolPropertyType";
    case TIoTUserInfo_TProperty_EPropertyType.EventPropertyType:
      return "EventPropertyType";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TProperty_TFloatPropertyParameters {
  Instance: string;
  Unit: string;
}

export interface TIoTUserInfo_TProperty_TFloatPropertyState {
  Instance: string;
  Value: number;
}

export interface TIoTUserInfo_TProperty_TBoolPropertyParameters {
  Instance: string;
}

export interface TIoTUserInfo_TProperty_TBoolPropertyState {
  Instance: string;
  Value: boolean;
}

export interface TIoTUserInfo_TProperty_TEventPropertyParameters {
  Instance: string;
  Events: TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent[];
}

export interface TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent {
  Name: string;
  Value: string;
}

export interface TIoTUserInfo_TProperty_TEventPropertyState {
  Instance: string;
  Value: string;
}

export interface TIoTUserInfo_TDevice {
  Id: string;
  Name: string;
  Aliases: string[];
  Type: EUserDeviceType;
  OriginalType: EUserDeviceType;
  ExternalId: string;
  ExternalName: string;
  SkillId: string;
  RoomId: string;
  GroupIds: string[];
  Capabilities: TIoTUserInfo_TCapability[];
  Properties: TIoTUserInfo_TProperty[];
  DeviceInfo?: TIoTUserInfo_TDevice_TDeviceInfo;
  QuasarInfo?: TIoTUserInfo_TDevice_TQuasarInfo;
  CustomData: Uint8Array;
  Updated: number;
  Created: number;
  HouseholdId: string;
  Status: TIoTUserInfo_TDevice_EDeviceState;
  StatusUpdated: number;
  AnalyticsType: string;
  AnalyticsName: string;
  IconURL: string;
  Favorite: boolean;
  InternalConfig?: TIoTUserInfo_TDevice_TDeviceConfig;
  SharingInfo?: TUserSharingInfo;
}

export enum TIoTUserInfo_TDevice_EDeviceState {
  UnknownDeviceState = 0,
  OnlineDeviceState = 1,
  OfflineDeviceState = 2,
  NotFoundDeviceState = 3,
  SplitState = 4,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TDevice_EDeviceStateFromJSON(
  object: any
): TIoTUserInfo_TDevice_EDeviceState {
  switch (object) {
    case 0:
    case "UnknownDeviceState":
      return TIoTUserInfo_TDevice_EDeviceState.UnknownDeviceState;
    case 1:
    case "OnlineDeviceState":
      return TIoTUserInfo_TDevice_EDeviceState.OnlineDeviceState;
    case 2:
    case "OfflineDeviceState":
      return TIoTUserInfo_TDevice_EDeviceState.OfflineDeviceState;
    case 3:
    case "NotFoundDeviceState":
      return TIoTUserInfo_TDevice_EDeviceState.NotFoundDeviceState;
    case 4:
    case "SplitState":
      return TIoTUserInfo_TDevice_EDeviceState.SplitState;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TDevice_EDeviceState.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TDevice_EDeviceStateToJSON(
  object: TIoTUserInfo_TDevice_EDeviceState
): string {
  switch (object) {
    case TIoTUserInfo_TDevice_EDeviceState.UnknownDeviceState:
      return "UnknownDeviceState";
    case TIoTUserInfo_TDevice_EDeviceState.OnlineDeviceState:
      return "OnlineDeviceState";
    case TIoTUserInfo_TDevice_EDeviceState.OfflineDeviceState:
      return "OfflineDeviceState";
    case TIoTUserInfo_TDevice_EDeviceState.NotFoundDeviceState:
      return "NotFoundDeviceState";
    case TIoTUserInfo_TDevice_EDeviceState.SplitState:
      return "SplitState";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TDevice_TDeviceInfo {
  Manufacturer: string;
  Model: string;
  HwVersion: string;
  SwVersion: string;
}

export interface TIoTUserInfo_TDevice_TQuasarInfo {
  DeviceId: string;
  Platform: string;
}

export interface TIoTUserInfo_TDevice_TDeviceConfig {
  Tandem?: TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig;
  SpeakerYandexIOConfig?: TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig;
}

export interface TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig {
  Partner?: TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner;
}

export interface TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner {
  Id: string;
}

export interface TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig {
  Networks?: TIotDiscoveryCapability_TNetworks;
}

export interface TIoTUserInfo_THousehold {
  Id: string;
  Name: string;
  Longitude: number;
  Latitude: number;
  Address: string;
  SharingInfo?: TUserSharingInfo;
}

export interface TIoTUserInfo_TColor {
  Id: string;
  Name: string;
}

export interface TIoTUserInfo_TScenario {
  Id: string;
  Name: string;
  Icon: string;
  Devices: TIoTUserInfo_TScenario_TDevice[];
  RequestedSpeakerCapabilities: TIoTUserInfo_TScenario_TCapability[];
  Triggers: TIoTUserInfo_TScenario_TTrigger[];
  IsActive: boolean;
  Steps: TIoTUserInfo_TScenario_TStep[];
  PushOnInvoke: boolean;
}

export interface TIoTUserInfo_TScenario_TCapability {
  Type: TIoTUserInfo_TCapability_ECapabilityType;
  OnOffCapabilityState?:
    | TIoTUserInfo_TCapability_TOnOffCapabilityState
    | undefined;
  ColorSettingCapabilityState?:
    | TIoTUserInfo_TCapability_TColorSettingCapabilityState
    | undefined;
  ModeCapabilityState?:
    | TIoTUserInfo_TCapability_TModeCapabilityState
    | undefined;
  RangeCapabilityState?:
    | TIoTUserInfo_TCapability_TRangeCapabilityState
    | undefined;
  ToggleCapabilityState?:
    | TIoTUserInfo_TCapability_TToggleCapabilityState
    | undefined;
  CustomButtonCapabilityState?:
    | TIoTUserInfo_TCapability_TCustomButtonCapabilityState
    | undefined;
  QuasarServerActionCapabilityState?:
    | TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState
    | undefined;
  QuasarCapabilityState?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState
    | undefined;
  VideoStreamCapabilityState?:
    | TIoTUserInfo_TCapability_TVideoStreamCapabilityState
    | undefined;
}

export interface TIoTUserInfo_TScenario_TDevice {
  Id: string;
  Capabilities: TIoTUserInfo_TScenario_TCapability[];
}

export interface TIoTUserInfo_TScenario_TLaunchDevice {
  Id: string;
  Name: string;
  Type: EUserDeviceType;
  Capabilities: TIoTUserInfo_TCapability[];
  CustomData: Uint8Array;
  SkillID: string;
}

export interface TIoTUserInfo_TScenario_TTrigger {
  Type: TIoTUserInfo_TScenario_TTrigger_ETriggerType;
  VoiceTriggerPhrase: string | undefined;
  TimerTriggerTimestamp: number | undefined;
  Timetable?: TIoTUserInfo_TScenario_TTrigger_TTimetable | undefined;
  DeviceProperty?: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty | undefined;
}

export enum TIoTUserInfo_TScenario_TTrigger_ETriggerType {
  UnknownScenarioTriggerType = 0,
  VoiceScenarioTriggerType = 1,
  TimerScenarioTriggerType = 2,
  TimetableScenarioTriggerType = 3,
  DevicePropertyScenarioTriggerType = 4,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TScenario_TTrigger_ETriggerTypeFromJSON(
  object: any
): TIoTUserInfo_TScenario_TTrigger_ETriggerType {
  switch (object) {
    case 0:
    case "UnknownScenarioTriggerType":
      return TIoTUserInfo_TScenario_TTrigger_ETriggerType.UnknownScenarioTriggerType;
    case 1:
    case "VoiceScenarioTriggerType":
      return TIoTUserInfo_TScenario_TTrigger_ETriggerType.VoiceScenarioTriggerType;
    case 2:
    case "TimerScenarioTriggerType":
      return TIoTUserInfo_TScenario_TTrigger_ETriggerType.TimerScenarioTriggerType;
    case 3:
    case "TimetableScenarioTriggerType":
      return TIoTUserInfo_TScenario_TTrigger_ETriggerType.TimetableScenarioTriggerType;
    case 4:
    case "DevicePropertyScenarioTriggerType":
      return TIoTUserInfo_TScenario_TTrigger_ETriggerType.DevicePropertyScenarioTriggerType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TScenario_TTrigger_ETriggerType.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TScenario_TTrigger_ETriggerTypeToJSON(
  object: TIoTUserInfo_TScenario_TTrigger_ETriggerType
): string {
  switch (object) {
    case TIoTUserInfo_TScenario_TTrigger_ETriggerType.UnknownScenarioTriggerType:
      return "UnknownScenarioTriggerType";
    case TIoTUserInfo_TScenario_TTrigger_ETriggerType.VoiceScenarioTriggerType:
      return "VoiceScenarioTriggerType";
    case TIoTUserInfo_TScenario_TTrigger_ETriggerType.TimerScenarioTriggerType:
      return "TimerScenarioTriggerType";
    case TIoTUserInfo_TScenario_TTrigger_ETriggerType.TimetableScenarioTriggerType:
      return "TimetableScenarioTriggerType";
    case TIoTUserInfo_TScenario_TTrigger_ETriggerType.DevicePropertyScenarioTriggerType:
      return "DevicePropertyScenarioTriggerType";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TScenario_TTrigger_TTimetable {
  TimeOffset: number;
  Weekdays: number[];
}

export interface TIoTUserInfo_TScenario_TTrigger_TDeviceProperty {
  DeviceID: string;
  PropertyType: string;
  Instance: string;
  ConditionType: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType;
  EventPropertyCondition?:
    | TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition
    | undefined;
  FloatPropertyCondition?:
    | TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition
    | undefined;
}

export enum TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType {
  UndefinedPropertyConditionType = 0,
  FloatPropertyConditionType = 1,
  EventPropertyConditionType = 2,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionTypeFromJSON(
  object: any
): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType {
  switch (object) {
    case 0:
    case "UndefinedPropertyConditionType":
      return TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.UndefinedPropertyConditionType;
    case 1:
    case "FloatPropertyConditionType":
      return TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.FloatPropertyConditionType;
    case 2:
    case "EventPropertyConditionType":
      return TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.EventPropertyConditionType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionTypeToJSON(
  object: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType
): string {
  switch (object) {
    case TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.UndefinedPropertyConditionType:
      return "UndefinedPropertyConditionType";
    case TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.FloatPropertyConditionType:
      return "FloatPropertyConditionType";
    case TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionType.EventPropertyConditionType:
      return "EventPropertyConditionType";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition {
  Values: string[];
}

export interface TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition {
  LowerBound?: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound;
  UpperBound?: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound;
}

export interface TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound {
  Value: number;
}

export interface TIoTUserInfo_TScenario_TStep {
  Type: TIoTUserInfo_TScenario_TStep_EStepType;
  ScenarioStepActionsParameters?:
    | TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters
    | undefined;
  ScenarioStepDelayParameters?:
    | TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters
    | undefined;
}

export enum TIoTUserInfo_TScenario_TStep_EStepType {
  UnknownScenarioStepType = 0,
  ActionsScenarioStepType = 1,
  DelayScenarioStepType = 2,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TScenario_TStep_EStepTypeFromJSON(
  object: any
): TIoTUserInfo_TScenario_TStep_EStepType {
  switch (object) {
    case 0:
    case "UnknownScenarioStepType":
      return TIoTUserInfo_TScenario_TStep_EStepType.UnknownScenarioStepType;
    case 1:
    case "ActionsScenarioStepType":
      return TIoTUserInfo_TScenario_TStep_EStepType.ActionsScenarioStepType;
    case 2:
    case "DelayScenarioStepType":
      return TIoTUserInfo_TScenario_TStep_EStepType.DelayScenarioStepType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TScenario_TStep_EStepType.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TScenario_TStep_EStepTypeToJSON(
  object: TIoTUserInfo_TScenario_TStep_EStepType
): string {
  switch (object) {
    case TIoTUserInfo_TScenario_TStep_EStepType.UnknownScenarioStepType:
      return "UnknownScenarioStepType";
    case TIoTUserInfo_TScenario_TStep_EStepType.ActionsScenarioStepType:
      return "ActionsScenarioStepType";
    case TIoTUserInfo_TScenario_TStep_EStepType.DelayScenarioStepType:
      return "DelayScenarioStepType";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters {
  Devices: TIoTUserInfo_TScenario_TLaunchDevice[];
  RequestedSpeakerCapabilities: TIoTUserInfo_TScenario_TCapability[];
}

export interface TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters {
  Delay: number;
}

export interface TIoTUserInfo_TStereopair {
  Id: string;
  Name: string;
  Devices: TIoTUserInfo_TStereopair_TStereopairDevice[];
}

export enum TIoTUserInfo_TStereopair_EStereopairRole {
  UnknownStereopairRole = 0,
  LeaderStereopairRole = 1,
  FollowerStereopairRole = 2,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TStereopair_EStereopairRoleFromJSON(
  object: any
): TIoTUserInfo_TStereopair_EStereopairRole {
  switch (object) {
    case 0:
    case "UnknownStereopairRole":
      return TIoTUserInfo_TStereopair_EStereopairRole.UnknownStereopairRole;
    case 1:
    case "LeaderStereopairRole":
      return TIoTUserInfo_TStereopair_EStereopairRole.LeaderStereopairRole;
    case 2:
    case "FollowerStereopairRole":
      return TIoTUserInfo_TStereopair_EStereopairRole.FollowerStereopairRole;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TStereopair_EStereopairRole.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TStereopair_EStereopairRoleToJSON(
  object: TIoTUserInfo_TStereopair_EStereopairRole
): string {
  switch (object) {
    case TIoTUserInfo_TStereopair_EStereopairRole.UnknownStereopairRole:
      return "UnknownStereopairRole";
    case TIoTUserInfo_TStereopair_EStereopairRole.LeaderStereopairRole:
      return "LeaderStereopairRole";
    case TIoTUserInfo_TStereopair_EStereopairRole.FollowerStereopairRole:
      return "FollowerStereopairRole";
    default:
      return "UNKNOWN";
  }
}

export enum TIoTUserInfo_TStereopair_EStereopairChannel {
  UnknownStereopairChannel = 0,
  LeftStereopairChannel = 1,
  RightStereopairChannel = 2,
  UNRECOGNIZED = -1,
}

export function tIoTUserInfo_TStereopair_EStereopairChannelFromJSON(
  object: any
): TIoTUserInfo_TStereopair_EStereopairChannel {
  switch (object) {
    case 0:
    case "UnknownStereopairChannel":
      return TIoTUserInfo_TStereopair_EStereopairChannel.UnknownStereopairChannel;
    case 1:
    case "LeftStereopairChannel":
      return TIoTUserInfo_TStereopair_EStereopairChannel.LeftStereopairChannel;
    case 2:
    case "RightStereopairChannel":
      return TIoTUserInfo_TStereopair_EStereopairChannel.RightStereopairChannel;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIoTUserInfo_TStereopair_EStereopairChannel.UNRECOGNIZED;
  }
}

export function tIoTUserInfo_TStereopair_EStereopairChannelToJSON(
  object: TIoTUserInfo_TStereopair_EStereopairChannel
): string {
  switch (object) {
    case TIoTUserInfo_TStereopair_EStereopairChannel.UnknownStereopairChannel:
      return "UnknownStereopairChannel";
    case TIoTUserInfo_TStereopair_EStereopairChannel.LeftStereopairChannel:
      return "LeftStereopairChannel";
    case TIoTUserInfo_TStereopair_EStereopairChannel.RightStereopairChannel:
      return "RightStereopairChannel";
    default:
      return "UNKNOWN";
  }
}

export interface TIoTUserInfo_TStereopair_TStereopairDevice {
  Id: string;
  Channel: TIoTUserInfo_TStereopair_EStereopairChannel;
  Role: TIoTUserInfo_TStereopair_EStereopairRole;
}

export interface TIoTCapabilityAction {
  Type: TIoTUserInfo_TCapability_ECapabilityType;
  OnOffCapabilityState?:
    | TIoTUserInfo_TCapability_TOnOffCapabilityState
    | undefined;
  ColorSettingCapabilityState?:
    | TIoTUserInfo_TCapability_TColorSettingCapabilityState
    | undefined;
  ModeCapabilityState?:
    | TIoTUserInfo_TCapability_TModeCapabilityState
    | undefined;
  RangeCapabilityState?:
    | TIoTUserInfo_TCapability_TRangeCapabilityState
    | undefined;
  ToggleCapabilityState?:
    | TIoTUserInfo_TCapability_TToggleCapabilityState
    | undefined;
  CustomButtonCapabilityState?:
    | TIoTUserInfo_TCapability_TCustomButtonCapabilityState
    | undefined;
  QuasarServerActionCapabilityState?:
    | TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState
    | undefined;
  QuasarCapabilityState?:
    | TIoTUserInfo_TCapability_TQuasarCapabilityState
    | undefined;
  VideoStreamCapabilityState?:
    | TIoTUserInfo_TCapability_TVideoStreamCapabilityState
    | undefined;
}

export interface TIoTActionIntentParameters {
  CapabilityType: string;
  CapabilityInstance: string;
  CapabilityValue?: TIoTActionIntentParameters_TCapabilityValue;
}

export interface TIoTActionIntentParameters_TCapabilityValue {
  RelativityType: string;
  Unit: string;
  BoolValue: boolean | undefined;
  NumValue: number | undefined;
  ModeValue: string | undefined;
}

export interface TIoTDeviceActionRequest {
  IntentParameters?: TIoTActionIntentParameters;
  RoomIDs: string[];
  HouseholdIDs: string[];
  GroupIDs: string[];
  DeviceIDs: string[];
  DeviceTypes: string[];
  /** time specification values for action */
  AtTimestamp: number;
  FromTimestamp: number;
  ToTimestamp: number;
  ForTimestamp: number;
}

export interface TStartIotDiscoveryRequest {
  Protocols: TIotDiscoveryCapability_TProtocol[];
}

export interface TFinishIotDiscoveryRequest {
  Protocols: TIotDiscoveryCapability_TProtocol[];
  Networks?: TIotDiscoveryCapability_TNetworks;
  DiscoveredEndpoints: TEndpoint[];
}

export interface TForgetIotEndpointsRequest {
  EndpointIds: string[];
}

export interface TIoTDeviceActions {
  DeviceId: string;
  Actions: TIoTCapabilityAction[];
  ExternalDeviceId: string;
  SkillId: string;
}

/** this is only relevant while https://st.yandex-team.ru/ZION-93 is not available and should be removed later */
export interface TIoTYandexIOActionRequest {
  EndpointActions: TIoTDeviceActions[];
}

export interface TIoTDeviceActionsBatch {
  Batch: TIoTDeviceActions[];
}

export interface TEndpointStateUpdatesRequest {
  /** Endpoints transfer partial state, notifying only actual capability state changes */
  EndpointUpdates: TEndpoint[];
}

function createBaseTIoTUserInfo(): TIoTUserInfo {
  return {
    Rooms: [],
    Groups: [],
    Devices: [],
    Scenarios: [],
    Colors: [],
    Households: [],
    Stereopairs: [],
    CurrentHouseholdId: "",
    RawUserInfo: "",
  };
}

export const TIoTUserInfo = {
  encode(message: TIoTUserInfo, writer: Writer = Writer.create()): Writer {
    for (const v of message.Rooms) {
      TUserRoom.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    for (const v of message.Groups) {
      TUserGroup.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    for (const v of message.Devices) {
      TIoTUserInfo_TDevice.encode(v!, writer.uint32(26).fork()).ldelim();
    }
    for (const v of message.Scenarios) {
      TIoTUserInfo_TScenario.encode(v!, writer.uint32(34).fork()).ldelim();
    }
    for (const v of message.Colors) {
      TIoTUserInfo_TColor.encode(v!, writer.uint32(42).fork()).ldelim();
    }
    for (const v of message.Households) {
      TIoTUserInfo_THousehold.encode(v!, writer.uint32(50).fork()).ldelim();
    }
    for (const v of message.Stereopairs) {
      TIoTUserInfo_TStereopair.encode(v!, writer.uint32(66).fork()).ldelim();
    }
    if (message.CurrentHouseholdId !== "") {
      writer.uint32(58).string(message.CurrentHouseholdId);
    }
    if (message.RawUserInfo !== "") {
      writer.uint32(802).string(message.RawUserInfo);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTUserInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Rooms.push(TUserRoom.decode(reader, reader.uint32()));
          break;
        case 2:
          message.Groups.push(TUserGroup.decode(reader, reader.uint32()));
          break;
        case 3:
          message.Devices.push(
            TIoTUserInfo_TDevice.decode(reader, reader.uint32())
          );
          break;
        case 4:
          message.Scenarios.push(
            TIoTUserInfo_TScenario.decode(reader, reader.uint32())
          );
          break;
        case 5:
          message.Colors.push(
            TIoTUserInfo_TColor.decode(reader, reader.uint32())
          );
          break;
        case 6:
          message.Households.push(
            TIoTUserInfo_THousehold.decode(reader, reader.uint32())
          );
          break;
        case 8:
          message.Stereopairs.push(
            TIoTUserInfo_TStereopair.decode(reader, reader.uint32())
          );
          break;
        case 7:
          message.CurrentHouseholdId = reader.string();
          break;
        case 100:
          message.RawUserInfo = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo {
    return {
      Rooms: Array.isArray(object?.rooms)
        ? object.rooms.map((e: any) => TUserRoom.fromJSON(e))
        : [],
      Groups: Array.isArray(object?.groups)
        ? object.groups.map((e: any) => TUserGroup.fromJSON(e))
        : [],
      Devices: Array.isArray(object?.devices)
        ? object.devices.map((e: any) => TIoTUserInfo_TDevice.fromJSON(e))
        : [],
      Scenarios: Array.isArray(object?.scenarios)
        ? object.scenarios.map((e: any) => TIoTUserInfo_TScenario.fromJSON(e))
        : [],
      Colors: Array.isArray(object?.colors)
        ? object.colors.map((e: any) => TIoTUserInfo_TColor.fromJSON(e))
        : [],
      Households: Array.isArray(object?.households)
        ? object.households.map((e: any) => TIoTUserInfo_THousehold.fromJSON(e))
        : [],
      Stereopairs: Array.isArray(object?.stereopairs)
        ? object.stereopairs.map((e: any) =>
            TIoTUserInfo_TStereopair.fromJSON(e)
          )
        : [],
      CurrentHouseholdId: isSet(object.current_household_id)
        ? String(object.current_household_id)
        : "",
      RawUserInfo: isSet(object.raw_user_info)
        ? String(object.raw_user_info)
        : "",
    };
  },

  toJSON(message: TIoTUserInfo): unknown {
    const obj: any = {};
    if (message.Rooms) {
      obj.rooms = message.Rooms.map((e) =>
        e ? TUserRoom.toJSON(e) : undefined
      );
    } else {
      obj.rooms = [];
    }
    if (message.Groups) {
      obj.groups = message.Groups.map((e) =>
        e ? TUserGroup.toJSON(e) : undefined
      );
    } else {
      obj.groups = [];
    }
    if (message.Devices) {
      obj.devices = message.Devices.map((e) =>
        e ? TIoTUserInfo_TDevice.toJSON(e) : undefined
      );
    } else {
      obj.devices = [];
    }
    if (message.Scenarios) {
      obj.scenarios = message.Scenarios.map((e) =>
        e ? TIoTUserInfo_TScenario.toJSON(e) : undefined
      );
    } else {
      obj.scenarios = [];
    }
    if (message.Colors) {
      obj.colors = message.Colors.map((e) =>
        e ? TIoTUserInfo_TColor.toJSON(e) : undefined
      );
    } else {
      obj.colors = [];
    }
    if (message.Households) {
      obj.households = message.Households.map((e) =>
        e ? TIoTUserInfo_THousehold.toJSON(e) : undefined
      );
    } else {
      obj.households = [];
    }
    if (message.Stereopairs) {
      obj.stereopairs = message.Stereopairs.map((e) =>
        e ? TIoTUserInfo_TStereopair.toJSON(e) : undefined
      );
    } else {
      obj.stereopairs = [];
    }
    message.CurrentHouseholdId !== undefined &&
      (obj.current_household_id = message.CurrentHouseholdId);
    message.RawUserInfo !== undefined &&
      (obj.raw_user_info = message.RawUserInfo);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability(): TIoTUserInfo_TCapability {
  return {
    Type: 0,
    Retrievable: false,
    Reportable: false,
    LastUpdated: 0,
    AnalyticsType: "",
    AnalyticsName: "",
    OnOffCapabilityParameters: undefined,
    ColorSettingCapabilityParameters: undefined,
    ModeCapabilityParameters: undefined,
    RangeCapabilityParameters: undefined,
    ToggleCapabilityParameters: undefined,
    CustomButtonCapabilityParameters: undefined,
    QuasarServerActionCapabilityParameters: undefined,
    QuasarCapabilityParameters: undefined,
    VideoStreamCapabilityParameters: undefined,
    OnOffCapabilityState: undefined,
    ColorSettingCapabilityState: undefined,
    ModeCapabilityState: undefined,
    RangeCapabilityState: undefined,
    ToggleCapabilityState: undefined,
    CustomButtonCapabilityState: undefined,
    QuasarServerActionCapabilityState: undefined,
    QuasarCapabilityState: undefined,
    VideoStreamCapabilityState: undefined,
  };
}

export const TIoTUserInfo_TCapability = {
  encode(
    message: TIoTUserInfo_TCapability,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.Retrievable === true) {
      writer.uint32(16).bool(message.Retrievable);
    }
    if (message.Reportable === true) {
      writer.uint32(24).bool(message.Reportable);
    }
    if (message.LastUpdated !== 0) {
      writer.uint32(33).double(message.LastUpdated);
    }
    if (message.AnalyticsType !== "") {
      writer.uint32(154).string(message.AnalyticsType);
    }
    if (message.AnalyticsName !== "") {
      writer.uint32(162).string(message.AnalyticsName);
    }
    if (message.OnOffCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TOnOffCapabilityParameters.encode(
        message.OnOffCapabilityParameters,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.ColorSettingCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityParameters.encode(
        message.ColorSettingCapabilityParameters,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.ModeCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TModeCapabilityParameters.encode(
        message.ModeCapabilityParameters,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.RangeCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TRangeCapabilityParameters.encode(
        message.RangeCapabilityParameters,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.ToggleCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TToggleCapabilityParameters.encode(
        message.ToggleCapabilityParameters,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.CustomButtonCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters.encode(
        message.CustomButtonCapabilityParameters,
        writer.uint32(82).fork()
      ).ldelim();
    }
    if (message.QuasarServerActionCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters.encode(
        message.QuasarServerActionCapabilityParameters,
        writer.uint32(90).fork()
      ).ldelim();
    }
    if (message.QuasarCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityParameters.encode(
        message.QuasarCapabilityParameters,
        writer.uint32(170).fork()
      ).ldelim();
    }
    if (message.VideoStreamCapabilityParameters !== undefined) {
      TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters.encode(
        message.VideoStreamCapabilityParameters,
        writer.uint32(186).fork()
      ).ldelim();
    }
    if (message.OnOffCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TOnOffCapabilityState.encode(
        message.OnOffCapabilityState,
        writer.uint32(98).fork()
      ).ldelim();
    }
    if (message.ColorSettingCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityState.encode(
        message.ColorSettingCapabilityState,
        writer.uint32(106).fork()
      ).ldelim();
    }
    if (message.ModeCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TModeCapabilityState.encode(
        message.ModeCapabilityState,
        writer.uint32(114).fork()
      ).ldelim();
    }
    if (message.RangeCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TRangeCapabilityState.encode(
        message.RangeCapabilityState,
        writer.uint32(122).fork()
      ).ldelim();
    }
    if (message.ToggleCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TToggleCapabilityState.encode(
        message.ToggleCapabilityState,
        writer.uint32(130).fork()
      ).ldelim();
    }
    if (message.CustomButtonCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TCustomButtonCapabilityState.encode(
        message.CustomButtonCapabilityState,
        writer.uint32(138).fork()
      ).ldelim();
    }
    if (message.QuasarServerActionCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.encode(
        message.QuasarServerActionCapabilityState,
        writer.uint32(146).fork()
      ).ldelim();
    }
    if (message.QuasarCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState.encode(
        message.QuasarCapabilityState,
        writer.uint32(178).fork()
      ).ldelim();
    }
    if (message.VideoStreamCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TVideoStreamCapabilityState.encode(
        message.VideoStreamCapabilityState,
        writer.uint32(194).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.Retrievable = reader.bool();
          break;
        case 3:
          message.Reportable = reader.bool();
          break;
        case 4:
          message.LastUpdated = reader.double();
          break;
        case 19:
          message.AnalyticsType = reader.string();
          break;
        case 20:
          message.AnalyticsName = reader.string();
          break;
        case 5:
          message.OnOffCapabilityParameters =
            TIoTUserInfo_TCapability_TOnOffCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.ColorSettingCapabilityParameters =
            TIoTUserInfo_TCapability_TColorSettingCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 7:
          message.ModeCapabilityParameters =
            TIoTUserInfo_TCapability_TModeCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 8:
          message.RangeCapabilityParameters =
            TIoTUserInfo_TCapability_TRangeCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 9:
          message.ToggleCapabilityParameters =
            TIoTUserInfo_TCapability_TToggleCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 10:
          message.CustomButtonCapabilityParameters =
            TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 11:
          message.QuasarServerActionCapabilityParameters =
            TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 21:
          message.QuasarCapabilityParameters =
            TIoTUserInfo_TCapability_TQuasarCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 23:
          message.VideoStreamCapabilityParameters =
            TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 12:
          message.OnOffCapabilityState =
            TIoTUserInfo_TCapability_TOnOffCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 13:
          message.ColorSettingCapabilityState =
            TIoTUserInfo_TCapability_TColorSettingCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 14:
          message.ModeCapabilityState =
            TIoTUserInfo_TCapability_TModeCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 15:
          message.RangeCapabilityState =
            TIoTUserInfo_TCapability_TRangeCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 16:
          message.ToggleCapabilityState =
            TIoTUserInfo_TCapability_TToggleCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 17:
          message.CustomButtonCapabilityState =
            TIoTUserInfo_TCapability_TCustomButtonCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 18:
          message.QuasarServerActionCapabilityState =
            TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 22:
          message.QuasarCapabilityState =
            TIoTUserInfo_TCapability_TQuasarCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 24:
          message.VideoStreamCapabilityState =
            TIoTUserInfo_TCapability_TVideoStreamCapabilityState.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability {
    return {
      Type: isSet(object.type)
        ? tIoTUserInfo_TCapability_ECapabilityTypeFromJSON(object.type)
        : 0,
      Retrievable: isSet(object.retrievable)
        ? Boolean(object.retrievable)
        : false,
      Reportable: isSet(object.reportable) ? Boolean(object.reportable) : false,
      LastUpdated: isSet(object.last_updated) ? Number(object.last_updated) : 0,
      AnalyticsType: isSet(object.analytics_type)
        ? String(object.analytics_type)
        : "",
      AnalyticsName: isSet(object.analytics_name)
        ? String(object.analytics_name)
        : "",
      OnOffCapabilityParameters: isSet(object.on_off_capability_parameters)
        ? TIoTUserInfo_TCapability_TOnOffCapabilityParameters.fromJSON(
            object.on_off_capability_parameters
          )
        : undefined,
      ColorSettingCapabilityParameters: isSet(
        object.color_setting_capability_parameters
      )
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters.fromJSON(
            object.color_setting_capability_parameters
          )
        : undefined,
      ModeCapabilityParameters: isSet(object.mode_capability_parameters)
        ? TIoTUserInfo_TCapability_TModeCapabilityParameters.fromJSON(
            object.mode_capability_parameters
          )
        : undefined,
      RangeCapabilityParameters: isSet(object.range_capability_parameters)
        ? TIoTUserInfo_TCapability_TRangeCapabilityParameters.fromJSON(
            object.range_capability_parameters
          )
        : undefined,
      ToggleCapabilityParameters: isSet(object.toggle_capability_parameters)
        ? TIoTUserInfo_TCapability_TToggleCapabilityParameters.fromJSON(
            object.toggle_capability_parameters
          )
        : undefined,
      CustomButtonCapabilityParameters: isSet(
        object.custom_button_capability_parameters
      )
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters.fromJSON(
            object.custom_button_capability_parameters
          )
        : undefined,
      QuasarServerActionCapabilityParameters: isSet(
        object.quasar_server_action_capability_parameters
      )
        ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters.fromJSON(
            object.quasar_server_action_capability_parameters
          )
        : undefined,
      QuasarCapabilityParameters: isSet(object.quasar_capability_parameters)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityParameters.fromJSON(
            object.quasar_capability_parameters
          )
        : undefined,
      VideoStreamCapabilityParameters: isSet(
        object.video_stream_capability_parameters
      )
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters.fromJSON(
            object.video_stream_capability_parameters
          )
        : undefined,
      OnOffCapabilityState: isSet(object.on_off_capability_state)
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState.fromJSON(
            object.on_off_capability_state
          )
        : undefined,
      ColorSettingCapabilityState: isSet(object.color_setting_capability_state)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState.fromJSON(
            object.color_setting_capability_state
          )
        : undefined,
      ModeCapabilityState: isSet(object.mode_capability_state)
        ? TIoTUserInfo_TCapability_TModeCapabilityState.fromJSON(
            object.mode_capability_state
          )
        : undefined,
      RangeCapabilityState: isSet(object.range_capability_state)
        ? TIoTUserInfo_TCapability_TRangeCapabilityState.fromJSON(
            object.range_capability_state
          )
        : undefined,
      ToggleCapabilityState: isSet(object.toggle_capability_state)
        ? TIoTUserInfo_TCapability_TToggleCapabilityState.fromJSON(
            object.toggle_capability_state
          )
        : undefined,
      CustomButtonCapabilityState: isSet(object.custom_button_capability_state)
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityState.fromJSON(
            object.custom_button_capability_state
          )
        : undefined,
      QuasarServerActionCapabilityState: isSet(
        object.quasar_server_action_capability_state
      )
        ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.fromJSON(
            object.quasar_server_action_capability_state
          )
        : undefined,
      QuasarCapabilityState: isSet(object.quasar_capability_state)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState.fromJSON(
            object.quasar_capability_state
          )
        : undefined,
      VideoStreamCapabilityState: isSet(object.video_stream_capability_state)
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState.fromJSON(
            object.video_stream_capability_state
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TCapability): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tIoTUserInfo_TCapability_ECapabilityTypeToJSON(message.Type));
    message.Retrievable !== undefined &&
      (obj.retrievable = message.Retrievable);
    message.Reportable !== undefined && (obj.reportable = message.Reportable);
    message.LastUpdated !== undefined &&
      (obj.last_updated = message.LastUpdated);
    message.AnalyticsType !== undefined &&
      (obj.analytics_type = message.AnalyticsType);
    message.AnalyticsName !== undefined &&
      (obj.analytics_name = message.AnalyticsName);
    message.OnOffCapabilityParameters !== undefined &&
      (obj.on_off_capability_parameters = message.OnOffCapabilityParameters
        ? TIoTUserInfo_TCapability_TOnOffCapabilityParameters.toJSON(
            message.OnOffCapabilityParameters
          )
        : undefined);
    message.ColorSettingCapabilityParameters !== undefined &&
      (obj.color_setting_capability_parameters =
        message.ColorSettingCapabilityParameters
          ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters.toJSON(
              message.ColorSettingCapabilityParameters
            )
          : undefined);
    message.ModeCapabilityParameters !== undefined &&
      (obj.mode_capability_parameters = message.ModeCapabilityParameters
        ? TIoTUserInfo_TCapability_TModeCapabilityParameters.toJSON(
            message.ModeCapabilityParameters
          )
        : undefined);
    message.RangeCapabilityParameters !== undefined &&
      (obj.range_capability_parameters = message.RangeCapabilityParameters
        ? TIoTUserInfo_TCapability_TRangeCapabilityParameters.toJSON(
            message.RangeCapabilityParameters
          )
        : undefined);
    message.ToggleCapabilityParameters !== undefined &&
      (obj.toggle_capability_parameters = message.ToggleCapabilityParameters
        ? TIoTUserInfo_TCapability_TToggleCapabilityParameters.toJSON(
            message.ToggleCapabilityParameters
          )
        : undefined);
    message.CustomButtonCapabilityParameters !== undefined &&
      (obj.custom_button_capability_parameters =
        message.CustomButtonCapabilityParameters
          ? TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters.toJSON(
              message.CustomButtonCapabilityParameters
            )
          : undefined);
    message.QuasarServerActionCapabilityParameters !== undefined &&
      (obj.quasar_server_action_capability_parameters =
        message.QuasarServerActionCapabilityParameters
          ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters.toJSON(
              message.QuasarServerActionCapabilityParameters
            )
          : undefined);
    message.QuasarCapabilityParameters !== undefined &&
      (obj.quasar_capability_parameters = message.QuasarCapabilityParameters
        ? TIoTUserInfo_TCapability_TQuasarCapabilityParameters.toJSON(
            message.QuasarCapabilityParameters
          )
        : undefined);
    message.VideoStreamCapabilityParameters !== undefined &&
      (obj.video_stream_capability_parameters =
        message.VideoStreamCapabilityParameters
          ? TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters.toJSON(
              message.VideoStreamCapabilityParameters
            )
          : undefined);
    message.OnOffCapabilityState !== undefined &&
      (obj.on_off_capability_state = message.OnOffCapabilityState
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState.toJSON(
            message.OnOffCapabilityState
          )
        : undefined);
    message.ColorSettingCapabilityState !== undefined &&
      (obj.color_setting_capability_state = message.ColorSettingCapabilityState
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState.toJSON(
            message.ColorSettingCapabilityState
          )
        : undefined);
    message.ModeCapabilityState !== undefined &&
      (obj.mode_capability_state = message.ModeCapabilityState
        ? TIoTUserInfo_TCapability_TModeCapabilityState.toJSON(
            message.ModeCapabilityState
          )
        : undefined);
    message.RangeCapabilityState !== undefined &&
      (obj.range_capability_state = message.RangeCapabilityState
        ? TIoTUserInfo_TCapability_TRangeCapabilityState.toJSON(
            message.RangeCapabilityState
          )
        : undefined);
    message.ToggleCapabilityState !== undefined &&
      (obj.toggle_capability_state = message.ToggleCapabilityState
        ? TIoTUserInfo_TCapability_TToggleCapabilityState.toJSON(
            message.ToggleCapabilityState
          )
        : undefined);
    message.CustomButtonCapabilityState !== undefined &&
      (obj.custom_button_capability_state = message.CustomButtonCapabilityState
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityState.toJSON(
            message.CustomButtonCapabilityState
          )
        : undefined);
    message.QuasarServerActionCapabilityState !== undefined &&
      (obj.quasar_server_action_capability_state =
        message.QuasarServerActionCapabilityState
          ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.toJSON(
              message.QuasarServerActionCapabilityState
            )
          : undefined);
    message.QuasarCapabilityState !== undefined &&
      (obj.quasar_capability_state = message.QuasarCapabilityState
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState.toJSON(
            message.QuasarCapabilityState
          )
        : undefined);
    message.VideoStreamCapabilityState !== undefined &&
      (obj.video_stream_capability_state = message.VideoStreamCapabilityState
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState.toJSON(
            message.VideoStreamCapabilityState
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TOnOffCapabilityParameters(): TIoTUserInfo_TCapability_TOnOffCapabilityParameters {
  return { Split: false };
}

export const TIoTUserInfo_TCapability_TOnOffCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TOnOffCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Split === true) {
      writer.uint32(8).bool(message.Split);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TOnOffCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TOnOffCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Split = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TOnOffCapabilityParameters {
    return {
      Split: isSet(object.split) ? Boolean(object.split) : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TOnOffCapabilityParameters
  ): unknown {
    const obj: any = {};
    message.Split !== undefined && (obj.split = message.Split);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters(): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters {
  return {
    ColorModel: undefined,
    TemperatureK: undefined,
    ColorSceneParameters: undefined,
  };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ColorModel !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel.encode(
        message.ColorModel,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.TemperatureK !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters.encode(
        message.TemperatureK,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ColorSceneParameters !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters.encode(
        message.ColorSceneParameters,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ColorModel =
            TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.TemperatureK =
            TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.ColorSceneParameters =
            TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters.decode(
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
  ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters {
    return {
      ColorModel: isSet(object.color_model)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel.fromJSON(
            object.color_model
          )
        : undefined,
      TemperatureK: isSet(object.temperature_k)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters.fromJSON(
            object.temperature_k
          )
        : undefined,
      ColorSceneParameters: isSet(object.color_scene)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters.fromJSON(
            object.color_scene
          )
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters
  ): unknown {
    const obj: any = {};
    message.ColorModel !== undefined &&
      (obj.color_model = message.ColorModel
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel.toJSON(
            message.ColorModel
          )
        : undefined);
    message.TemperatureK !== undefined &&
      (obj.temperature_k = message.TemperatureK
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters.toJSON(
            message.TemperatureK
          )
        : undefined);
    message.ColorSceneParameters !== undefined &&
      (obj.color_scene = message.ColorSceneParameters
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters.toJSON(
            message.ColorSceneParameters
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel(): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel {
  return { Type: 0, AnalyticsName: "" };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel =
  {
    encode(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Type !== 0) {
        writer.uint32(8).int32(message.Type);
      }
      if (message.AnalyticsName !== "") {
        writer.uint32(18).string(message.AnalyticsName);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Type = reader.int32() as any;
            break;
          case 2:
            message.AnalyticsName = reader.string();
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
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel {
      return {
        Type: isSet(object.type)
          ? tIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelTypeFromJSON(
              object.type
            )
          : 0,
        AnalyticsName: isSet(object.analytics_name)
          ? String(object.analytics_name)
          : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel
    ): unknown {
      const obj: any = {};
      message.Type !== undefined &&
        (obj.type =
          tIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelTypeToJSON(
            message.Type
          ));
      message.AnalyticsName !== undefined &&
        (obj.analytics_name = message.AnalyticsName);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters(): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters {
  return { Min: 0, Max: 0, AnalyticsName: "" };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters =
  {
    encode(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Min !== 0) {
        writer.uint32(8).int32(message.Min);
      }
      if (message.Max !== 0) {
        writer.uint32(16).int32(message.Max);
      }
      if (message.AnalyticsName !== "") {
        writer.uint32(26).string(message.AnalyticsName);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Min = reader.int32();
            break;
          case 2:
            message.Max = reader.int32();
            break;
          case 3:
            message.AnalyticsName = reader.string();
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
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters {
      return {
        Min: isSet(object.Min) ? Number(object.Min) : 0,
        Max: isSet(object.Max) ? Number(object.Max) : 0,
        AnalyticsName: isSet(object.analytics_name)
          ? String(object.analytics_name)
          : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters
    ): unknown {
      const obj: any = {};
      message.Min !== undefined && (obj.Min = Math.round(message.Min));
      message.Max !== undefined && (obj.Max = Math.round(message.Max));
      message.AnalyticsName !== undefined &&
        (obj.analytics_name = message.AnalyticsName);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene(): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene {
  return { ID: "", Name: "" };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene =
  {
    encode(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.ID !== "") {
        writer.uint32(10).string(message.ID);
      }
      if (message.Name !== "") {
        writer.uint32(18).string(message.Name);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.ID = reader.string();
            break;
          case 2:
            message.Name = reader.string();
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
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene {
      return {
        ID: isSet(object.id) ? String(object.id) : "",
        Name: isSet(object.name) ? String(object.name) : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene
    ): unknown {
      const obj: any = {};
      message.ID !== undefined && (obj.id = message.ID);
      message.Name !== undefined && (obj.name = message.Name);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters(): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters {
  return { Scenes: [], AnalyticsName: "" };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters =
  {
    encode(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters,
      writer: Writer = Writer.create()
    ): Writer {
      for (const v of message.Scenes) {
        TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene.encode(
          v!,
          writer.uint32(10).fork()
        ).ldelim();
      }
      if (message.AnalyticsName !== "") {
        writer.uint32(18).string(message.AnalyticsName);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Scenes.push(
              TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene.decode(
                reader,
                reader.uint32()
              )
            );
            break;
          case 2:
            message.AnalyticsName = reader.string();
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
    ): TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters {
      return {
        Scenes: Array.isArray(object?.scenes)
          ? object.scenes.map((e: any) =>
              TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene.fromJSON(
                e
              )
            )
          : [],
        AnalyticsName: isSet(object.analytics_name)
          ? String(object.analytics_name)
          : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters
    ): unknown {
      const obj: any = {};
      if (message.Scenes) {
        obj.scenes = message.Scenes.map((e) =>
          e
            ? TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene.toJSON(
                e
              )
            : undefined
        );
      } else {
        obj.scenes = [];
      }
      message.AnalyticsName !== undefined &&
        (obj.analytics_name = message.AnalyticsName);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TModeCapabilityParameters(): TIoTUserInfo_TCapability_TModeCapabilityParameters {
  return { Instance: "", Modes: [] };
}

export const TIoTUserInfo_TCapability_TModeCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TModeCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    for (const v of message.Modes) {
      TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TModeCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TModeCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Modes.push(
            TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability_TModeCapabilityParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Modes: Array.isArray(object?.modes)
        ? object.modes.map((e: any) =>
            TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TIoTUserInfo_TCapability_TModeCapabilityParameters): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    if (message.Modes) {
      obj.modes = message.Modes.map((e) =>
        e
          ? TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode.toJSON(e)
          : undefined
      );
    } else {
      obj.modes = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TModeCapabilityParameters_TMode(): TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode {
  return { Name: "", Value: "" };
}

export const TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode = {
  encode(
    message: TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    if (message.Value !== "") {
      writer.uint32(18).string(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TModeCapabilityParameters_TMode();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Value = reader.string();
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
  ): TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Value: isSet(object.value) ? String(object.value) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode
  ): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TRangeCapabilityParameters(): TIoTUserInfo_TCapability_TRangeCapabilityParameters {
  return {
    Instance: "",
    Unit: "",
    RandomAccess: false,
    Looped: false,
    Range: undefined,
  };
}

export const TIoTUserInfo_TCapability_TRangeCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TRangeCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Unit !== "") {
      writer.uint32(18).string(message.Unit);
    }
    if (message.RandomAccess === true) {
      writer.uint32(24).bool(message.RandomAccess);
    }
    if (message.Looped === true) {
      writer.uint32(32).bool(message.Looped);
    }
    if (message.Range !== undefined) {
      TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange.encode(
        message.Range,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TRangeCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TRangeCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Unit = reader.string();
          break;
        case 3:
          message.RandomAccess = reader.bool();
          break;
        case 4:
          message.Looped = reader.bool();
          break;
        case 5:
          message.Range =
            TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability_TRangeCapabilityParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Unit: isSet(object.unit) ? String(object.unit) : "",
      RandomAccess: isSet(object.random_access)
        ? Boolean(object.random_access)
        : false,
      Looped: isSet(object.looped) ? Boolean(object.looped) : false,
      Range: isSet(object.range)
        ? TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange.fromJSON(
            object.range
          )
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TRangeCapabilityParameters
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Unit !== undefined && (obj.unit = message.Unit);
    message.RandomAccess !== undefined &&
      (obj.random_access = message.RandomAccess);
    message.Looped !== undefined && (obj.looped = message.Looped);
    message.Range !== undefined &&
      (obj.range = message.Range
        ? TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange.toJSON(
            message.Range
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange(): TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange {
  return { Min: 0, Max: 0, Precision: 0 };
}

export const TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange = {
  encode(
    message: TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Min !== 0) {
      writer.uint32(9).double(message.Min);
    }
    if (message.Max !== 0) {
      writer.uint32(17).double(message.Max);
    }
    if (message.Precision !== 0) {
      writer.uint32(25).double(message.Precision);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Min = reader.double();
          break;
        case 2:
          message.Max = reader.double();
          break;
        case 3:
          message.Precision = reader.double();
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
  ): TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange {
    return {
      Min: isSet(object.min) ? Number(object.min) : 0,
      Max: isSet(object.max) ? Number(object.max) : 0,
      Precision: isSet(object.precision) ? Number(object.precision) : 0,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange
  ): unknown {
    const obj: any = {};
    message.Min !== undefined && (obj.min = message.Min);
    message.Max !== undefined && (obj.max = message.Max);
    message.Precision !== undefined && (obj.precision = message.Precision);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TToggleCapabilityParameters(): TIoTUserInfo_TCapability_TToggleCapabilityParameters {
  return { Instance: "" };
}

export const TIoTUserInfo_TCapability_TToggleCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TToggleCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TToggleCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TToggleCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TToggleCapabilityParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TToggleCapabilityParameters
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TCustomButtonCapabilityParameters(): TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters {
  return { Instance: "", InstanceNames: [] };
}

export const TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    for (const v of message.InstanceNames) {
      writer.uint32(18).string(v!);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TCustomButtonCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.InstanceNames.push(reader.string());
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
  ): TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      InstanceNames: Array.isArray(object?.instance_names)
        ? object.instance_names.map((e: any) => String(e))
        : [],
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    if (message.InstanceNames) {
      obj.instance_names = message.InstanceNames.map((e) => e);
    } else {
      obj.instance_names = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters(): TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters {
  return { Instance: "" };
}

export const TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters =
  {
    encode(
      message: TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Instance !== "") {
        writer.uint32(10).string(message.Instance);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Instance = reader.string();
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
    ): TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters {
      return {
        Instance: isSet(object.instance) ? String(object.instance) : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters
    ): unknown {
      const obj: any = {};
      message.Instance !== undefined && (obj.instance = message.Instance);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityParameters(): TIoTUserInfo_TCapability_TQuasarCapabilityParameters {
  return { Instance: "" };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TQuasarCapabilityParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityParameters
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TVideoStreamCapabilityParameters(): TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters {
  return { Protocols: [] };
}

export const TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters = {
  encode(
    message: TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.Protocols) {
      writer.uint32(10).string(v!);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TVideoStreamCapabilityParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Protocols.push(reader.string());
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
  ): TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters {
    return {
      Protocols: Array.isArray(object?.protocols)
        ? object.protocols.map((e: any) => String(e))
        : [],
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters
  ): unknown {
    const obj: any = {};
    if (message.Protocols) {
      obj.protocols = message.Protocols.map((e) => e);
    } else {
      obj.protocols = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TOnOffCapabilityState(): TIoTUserInfo_TCapability_TOnOffCapabilityState {
  return { Instance: "", Value: false, Relative: undefined };
}

export const TIoTUserInfo_TCapability_TOnOffCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TOnOffCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value === true) {
      writer.uint32(16).bool(message.Value);
    }
    if (message.Relative !== undefined) {
      TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative.encode(
        message.Relative,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TOnOffCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TCapability_TOnOffCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.bool();
          break;
        case 3:
          message.Relative =
            TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability_TOnOffCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? Boolean(object.value) : false,
      Relative: isSet(object.relative)
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative.fromJSON(
            object.relative
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TCapability_TOnOffCapabilityState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    message.Relative !== undefined &&
      (obj.relative = message.Relative
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative.toJSON(
            message.Relative
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative(): TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative {
  return { IsRelative: false };
}

export const TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative = {
  encode(
    message: TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.IsRelative === true) {
      writer.uint32(8).bool(message.IsRelative);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.IsRelative = reader.bool();
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
  ): TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative {
    return {
      IsRelative: isSet(object.is_relative)
        ? Boolean(object.is_relative)
        : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative
  ): unknown {
    const obj: any = {};
    message.IsRelative !== undefined && (obj.is_relative = message.IsRelative);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityState(): TIoTUserInfo_TCapability_TColorSettingCapabilityState {
  return {
    Instance: "",
    TemperatureK: undefined,
    RGB: undefined,
    HSV: undefined,
    ColorSceneID: undefined,
  };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TColorSettingCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.TemperatureK !== undefined) {
      writer.uint32(16).int32(message.TemperatureK);
    }
    if (message.RGB !== undefined) {
      writer.uint32(24).int32(message.RGB);
    }
    if (message.HSV !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV.encode(
        message.HSV,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.ColorSceneID !== undefined) {
      writer.uint32(42).string(message.ColorSceneID);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TColorSettingCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.TemperatureK = reader.int32();
          break;
        case 3:
          message.RGB = reader.int32();
          break;
        case 4:
          message.HSV =
            TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV.decode(
              reader,
              reader.uint32()
            );
          break;
        case 5:
          message.ColorSceneID = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TColorSettingCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      TemperatureK: isSet(object.temperature_k)
        ? Number(object.temperature_k)
        : undefined,
      RGB: isSet(object.rgb) ? Number(object.rgb) : undefined,
      HSV: isSet(object.hsv)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV.fromJSON(
            object.hsv
          )
        : undefined,
      ColorSceneID: isSet(object.color_scene_id)
        ? String(object.color_scene_id)
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TColorSettingCapabilityState
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.TemperatureK !== undefined &&
      (obj.temperature_k = Math.round(message.TemperatureK));
    message.RGB !== undefined && (obj.rgb = Math.round(message.RGB));
    message.HSV !== undefined &&
      (obj.hsv = message.HSV
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV.toJSON(
            message.HSV
          )
        : undefined);
    message.ColorSceneID !== undefined &&
      (obj.color_scene_id = message.ColorSceneID);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV(): TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV {
  return { H: 0, S: 0, V: 0 };
}

export const TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV = {
  encode(
    message: TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.H !== 0) {
      writer.uint32(8).int32(message.H);
    }
    if (message.S !== 0) {
      writer.uint32(16).int32(message.S);
    }
    if (message.V !== 0) {
      writer.uint32(24).int32(message.V);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.H = reader.int32();
          break;
        case 2:
          message.S = reader.int32();
          break;
        case 3:
          message.V = reader.int32();
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
  ): TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV {
    return {
      H: isSet(object.h) ? Number(object.h) : 0,
      S: isSet(object.s) ? Number(object.s) : 0,
      V: isSet(object.v) ? Number(object.v) : 0,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV
  ): unknown {
    const obj: any = {};
    message.H !== undefined && (obj.h = Math.round(message.H));
    message.S !== undefined && (obj.s = Math.round(message.S));
    message.V !== undefined && (obj.v = Math.round(message.V));
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TRangeCapabilityState(): TIoTUserInfo_TCapability_TRangeCapabilityState {
  return { Instance: "", Value: 0, Relative: undefined };
}

export const TIoTUserInfo_TCapability_TRangeCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TRangeCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value !== 0) {
      writer.uint32(17).double(message.Value);
    }
    if (message.Relative !== undefined) {
      TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative.encode(
        message.Relative,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TRangeCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TCapability_TRangeCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.double();
          break;
        case 3:
          message.Relative =
            TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability_TRangeCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? Number(object.value) : 0,
      Relative: isSet(object.relative)
        ? TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative.fromJSON(
            object.relative
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TCapability_TRangeCapabilityState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    message.Relative !== undefined &&
      (obj.relative = message.Relative
        ? TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative.toJSON(
            message.Relative
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TRangeCapabilityState_TRelative(): TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative {
  return { IsRelative: false };
}

export const TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative = {
  encode(
    message: TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.IsRelative === true) {
      writer.uint32(8).bool(message.IsRelative);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TRangeCapabilityState_TRelative();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.IsRelative = reader.bool();
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
  ): TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative {
    return {
      IsRelative: isSet(object.is_relative)
        ? Boolean(object.is_relative)
        : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative
  ): unknown {
    const obj: any = {};
    message.IsRelative !== undefined && (obj.is_relative = message.IsRelative);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TModeCapabilityState(): TIoTUserInfo_TCapability_TModeCapabilityState {
  return { Instance: "", Value: "" };
}

export const TIoTUserInfo_TCapability_TModeCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TModeCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value !== "") {
      writer.uint32(18).string(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TModeCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TCapability_TModeCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TModeCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? String(object.value) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TCapability_TModeCapabilityState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TToggleCapabilityState(): TIoTUserInfo_TCapability_TToggleCapabilityState {
  return { Instance: "", Value: false };
}

export const TIoTUserInfo_TCapability_TToggleCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TToggleCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value === true) {
      writer.uint32(16).bool(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TToggleCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TCapability_TToggleCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TToggleCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? Boolean(object.value) : false,
    };
  },

  toJSON(message: TIoTUserInfo_TCapability_TToggleCapabilityState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TCustomButtonCapabilityState(): TIoTUserInfo_TCapability_TCustomButtonCapabilityState {
  return { Instance: "", Value: false };
}

export const TIoTUserInfo_TCapability_TCustomButtonCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TCustomButtonCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value === true) {
      writer.uint32(16).bool(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TCustomButtonCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TCustomButtonCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TCapability_TCustomButtonCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? Boolean(object.value) : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TCustomButtonCapabilityState
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarServerActionCapabilityState(): TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState {
  return { Instance: "", Value: "" };
}

export const TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value !== "") {
      writer.uint32(18).string(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarServerActionCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.string();
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
  ): TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? String(object.value) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState(): TIoTUserInfo_TCapability_TQuasarCapabilityState {
  return {
    Instance: "",
    MusicPlayValue: undefined,
    NewsValue: undefined,
    SoundPlayValue: undefined,
    StopEverythingValue: undefined,
    VolumeValue: undefined,
    WeatherValue: undefined,
    TtsValue: undefined,
    AliceShowValue: undefined,
  };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.MusicPlayValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue.encode(
        message.MusicPlayValue,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.NewsValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue.encode(
        message.NewsValue,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.SoundPlayValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue.encode(
        message.SoundPlayValue,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.StopEverythingValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue.encode(
        message.StopEverythingValue,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.VolumeValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue.encode(
        message.VolumeValue,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.WeatherValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue.encode(
        message.WeatherValue,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.TtsValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue.encode(
        message.TtsValue,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.AliceShowValue !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue.encode(
        message.AliceShowValue,
        writer.uint32(74).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.MusicPlayValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.NewsValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.SoundPlayValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 5:
          message.StopEverythingValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.VolumeValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 7:
          message.WeatherValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 8:
          message.TtsValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue.decode(
              reader,
              reader.uint32()
            );
          break;
        case 9:
          message.AliceShowValue =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability_TQuasarCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      MusicPlayValue: isSet(object.music_play_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue.fromJSON(
            object.music_play_value
          )
        : undefined,
      NewsValue: isSet(object.news_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue.fromJSON(
            object.news_value
          )
        : undefined,
      SoundPlayValue: isSet(object.sound_play_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue.fromJSON(
            object.sound_play_value
          )
        : undefined,
      StopEverythingValue: isSet(object.stop_everything_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue.fromJSON(
            object.stop_everything_value
          )
        : undefined,
      VolumeValue: isSet(object.volume_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue.fromJSON(
            object.volume_value
          )
        : undefined,
      WeatherValue: isSet(object.weather_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue.fromJSON(
            object.weather_value
          )
        : undefined,
      TtsValue: isSet(object.tts_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue.fromJSON(
            object.tts_value
          )
        : undefined,
      AliceShowValue: isSet(object.alice_show_value)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue.fromJSON(
            object.alice_show_value
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TCapability_TQuasarCapabilityState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.MusicPlayValue !== undefined &&
      (obj.music_play_value = message.MusicPlayValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue.toJSON(
            message.MusicPlayValue
          )
        : undefined);
    message.NewsValue !== undefined &&
      (obj.news_value = message.NewsValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue.toJSON(
            message.NewsValue
          )
        : undefined);
    message.SoundPlayValue !== undefined &&
      (obj.sound_play_value = message.SoundPlayValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue.toJSON(
            message.SoundPlayValue
          )
        : undefined);
    message.StopEverythingValue !== undefined &&
      (obj.stop_everything_value = message.StopEverythingValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue.toJSON(
            message.StopEverythingValue
          )
        : undefined);
    message.VolumeValue !== undefined &&
      (obj.volume_value = message.VolumeValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue.toJSON(
            message.VolumeValue
          )
        : undefined);
    message.WeatherValue !== undefined &&
      (obj.weather_value = message.WeatherValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue.toJSON(
            message.WeatherValue
          )
        : undefined);
    message.TtsValue !== undefined &&
      (obj.tts_value = message.TtsValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue.toJSON(
            message.TtsValue
          )
        : undefined);
    message.AliceShowValue !== undefined &&
      (obj.alice_show_value = message.AliceShowValue
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue.toJSON(
            message.AliceShowValue
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue {
  return { Object: undefined, SearchText: undefined, PlayInBackground: false };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Object !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject.encode(
        message.Object,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.SearchText !== undefined) {
      writer.uint32(18).string(message.SearchText);
    }
    if (message.PlayInBackground === true) {
      writer.uint32(24).bool(message.PlayInBackground);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Object =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.SearchText = reader.string();
          break;
        case 3:
          message.PlayInBackground = reader.bool();
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
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue {
    return {
      Object: isSet(object.object)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject.fromJSON(
            object.object
          )
        : undefined,
      SearchText: isSet(object.search_text)
        ? String(object.search_text)
        : undefined,
      PlayInBackground: isSet(object.play_in_background)
        ? Boolean(object.play_in_background)
        : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue
  ): unknown {
    const obj: any = {};
    message.Object !== undefined &&
      (obj.object = message.Object
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject.toJSON(
            message.Object
          )
        : undefined);
    message.SearchText !== undefined && (obj.search_text = message.SearchText);
    message.PlayInBackground !== undefined &&
      (obj.play_in_background = message.PlayInBackground);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject {
  return { Id: "", Type: "", Name: "" };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject =
  {
    encode(
      message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Id !== "") {
        writer.uint32(10).string(message.Id);
      }
      if (message.Type !== "") {
        writer.uint32(18).string(message.Type);
      }
      if (message.Name !== "") {
        writer.uint32(26).string(message.Name);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Id = reader.string();
            break;
          case 2:
            message.Type = reader.string();
            break;
          case 3:
            message.Name = reader.string();
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
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject {
      return {
        Id: isSet(object.id) ? String(object.id) : "",
        Type: isSet(object.type) ? String(object.type) : "",
        Name: isSet(object.name) ? String(object.name) : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject
    ): unknown {
      const obj: any = {};
      message.Id !== undefined && (obj.id = message.Id);
      message.Type !== undefined && (obj.type = message.Type);
      message.Name !== undefined && (obj.name = message.Name);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue {
  return { Topic: "", Provider: "", PlayInBackground: false };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Topic !== "") {
      writer.uint32(10).string(message.Topic);
    }
    if (message.Provider !== "") {
      writer.uint32(18).string(message.Provider);
    }
    if (message.PlayInBackground === true) {
      writer.uint32(24).bool(message.PlayInBackground);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Topic = reader.string();
          break;
        case 2:
          message.Provider = reader.string();
          break;
        case 3:
          message.PlayInBackground = reader.bool();
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
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue {
    return {
      Topic: isSet(object.topic) ? String(object.topic) : "",
      Provider: isSet(object.provider) ? String(object.provider) : "",
      PlayInBackground: isSet(object.play_in_background)
        ? Boolean(object.play_in_background)
        : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue
  ): unknown {
    const obj: any = {};
    message.Topic !== undefined && (obj.topic = message.Topic);
    message.Provider !== undefined && (obj.provider = message.Provider);
    message.PlayInBackground !== undefined &&
      (obj.play_in_background = message.PlayInBackground);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue {
  return { Sound: "" };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Sound !== "") {
      writer.uint32(10).string(message.Sound);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Sound = reader.string();
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
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue {
    return {
      Sound: isSet(object.sound) ? String(object.sound) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue
  ): unknown {
    const obj: any = {};
    message.Sound !== undefined && (obj.sound = message.Sound);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue {
  return {};
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue =
  {
    encode(
      _: TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue,
      writer: Writer = Writer.create()
    ): Writer {
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue();
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

    fromJSON(
      _: any
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue {
      return {};
    },

    toJSON(
      _: TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue
    ): unknown {
      const obj: any = {};
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue {
  return { Value: 0, Relative: false };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Value !== 0) {
      writer.uint32(8).int32(message.Value);
    }
    if (message.Relative === true) {
      writer.uint32(16).bool(message.Relative);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Value = reader.int32();
          break;
        case 2:
          message.Relative = reader.bool();
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
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue {
    return {
      Value: isSet(object.value) ? Number(object.value) : 0,
      Relative: isSet(object.relative) ? Boolean(object.relative) : false,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue
  ): unknown {
    const obj: any = {};
    message.Value !== undefined && (obj.value = Math.round(message.Value));
    message.Relative !== undefined && (obj.relative = message.Relative);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue {
  return { Where: undefined, Household: undefined };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Where !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation.encode(
        message.Where,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Household !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo.encode(
        message.Household,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Where =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.Household =
            TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo.decode(
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
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue {
    return {
      Where: isSet(object.where)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation.fromJSON(
            object.where
          )
        : undefined,
      Household: isSet(object.household)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo.fromJSON(
            object.household
          )
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue
  ): unknown {
    const obj: any = {};
    message.Where !== undefined &&
      (obj.where = message.Where
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation.toJSON(
            message.Where
          )
        : undefined);
    message.Household !== undefined &&
      (obj.household = message.Household
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo.toJSON(
            message.Household
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation {
  return { Longitude: 0, Latitude: 0, Address: "", ShortAddress: "" };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation =
  {
    encode(
      message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Longitude !== 0) {
        writer.uint32(9).double(message.Longitude);
      }
      if (message.Latitude !== 0) {
        writer.uint32(17).double(message.Latitude);
      }
      if (message.Address !== "") {
        writer.uint32(26).string(message.Address);
      }
      if (message.ShortAddress !== "") {
        writer.uint32(34).string(message.ShortAddress);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Longitude = reader.double();
            break;
          case 2:
            message.Latitude = reader.double();
            break;
          case 3:
            message.Address = reader.string();
            break;
          case 4:
            message.ShortAddress = reader.string();
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
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation {
      return {
        Longitude: isSet(object.longitude) ? Number(object.longitude) : 0,
        Latitude: isSet(object.latitude) ? Number(object.latitude) : 0,
        Address: isSet(object.address) ? String(object.address) : "",
        ShortAddress: isSet(object.short_address)
          ? String(object.short_address)
          : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation
    ): unknown {
      const obj: any = {};
      message.Longitude !== undefined && (obj.longitude = message.Longitude);
      message.Latitude !== undefined && (obj.latitude = message.Latitude);
      message.Address !== undefined && (obj.address = message.Address);
      message.ShortAddress !== undefined &&
        (obj.short_address = message.ShortAddress);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo {
  return { Id: "", Name: "" };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo =
  {
    encode(
      message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Id !== "") {
        writer.uint32(10).string(message.Id);
      }
      if (message.Name !== "") {
        writer.uint32(18).string(message.Name);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Id = reader.string();
            break;
          case 2:
            message.Name = reader.string();
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
    ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo {
      return {
        Id: isSet(object.id) ? String(object.id) : "",
        Name: isSet(object.name) ? String(object.name) : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo
    ): unknown {
      const obj: any = {};
      message.Id !== undefined && (obj.id = message.Id);
      message.Name !== undefined && (obj.name = message.Name);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue {
  return { Text: "" };
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue = {
  encode(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== "") {
      writer.uint32(10).string(message.Text);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = reader.string();
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
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue {
    return {
      Text: isSet(object.text) ? String(object.text) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue
  ): unknown {
    const obj: any = {};
    message.Text !== undefined && (obj.text = message.Text);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue(): TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue {
  return {};
}

export const TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue = {
  encode(
    _: TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue();
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

  fromJSON(
    _: any
  ): TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue {
    return {};
  },

  toJSON(
    _: TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue
  ): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TVideoStreamCapabilityState(): TIoTUserInfo_TCapability_TVideoStreamCapabilityState {
  return { Instance: "", Value: undefined };
}

export const TIoTUserInfo_TCapability_TVideoStreamCapabilityState = {
  encode(
    message: TIoTUserInfo_TCapability_TVideoStreamCapabilityState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value !== undefined) {
      TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue.encode(
        message.Value,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TCapability_TVideoStreamCapabilityState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TCapability_TVideoStreamCapabilityState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value =
            TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue.decode(
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

  fromJSON(object: any): TIoTUserInfo_TCapability_TVideoStreamCapabilityState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value)
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue.fromJSON(
            object.value
          )
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TCapability_TVideoStreamCapabilityState
  ): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined &&
      (obj.value = message.Value
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue.toJSON(
            message.Value
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue(): TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue {
  return { Protocol: "", StreamURL: "", ExpirationTime: 0, Protocols: [] };
}

export const TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue =
  {
    encode(
      message: TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Protocol !== "") {
        writer.uint32(10).string(message.Protocol);
      }
      if (message.StreamURL !== "") {
        writer.uint32(18).string(message.StreamURL);
      }
      if (message.ExpirationTime !== 0) {
        writer.uint32(24).uint64(message.ExpirationTime);
      }
      for (const v of message.Protocols) {
        writer.uint32(34).string(v!);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Protocol = reader.string();
            break;
          case 2:
            message.StreamURL = reader.string();
            break;
          case 3:
            message.ExpirationTime = longToNumber(reader.uint64() as Long);
            break;
          case 4:
            message.Protocols.push(reader.string());
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
    ): TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue {
      return {
        Protocol: isSet(object.protocol) ? String(object.protocol) : "",
        StreamURL: isSet(object.stream_url) ? String(object.stream_url) : "",
        ExpirationTime: isSet(object.expiration_time)
          ? Number(object.expiration_time)
          : 0,
        Protocols: Array.isArray(object?.protocols)
          ? object.protocols.map((e: any) => String(e))
          : [],
      };
    },

    toJSON(
      message: TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue
    ): unknown {
      const obj: any = {};
      message.Protocol !== undefined && (obj.protocol = message.Protocol);
      message.StreamURL !== undefined && (obj.stream_url = message.StreamURL);
      message.ExpirationTime !== undefined &&
        (obj.expiration_time = Math.round(message.ExpirationTime));
      if (message.Protocols) {
        obj.protocols = message.Protocols.map((e) => e);
      } else {
        obj.protocols = [];
      }
      return obj;
    },
  };

function createBaseTIoTUserInfo_TProperty(): TIoTUserInfo_TProperty {
  return {
    Type: 0,
    Retrievable: false,
    Reportable: false,
    StateChangedAt: 0,
    LastUpdated: 0,
    LastActivated: 0,
    AnalyticsType: "",
    AnalyticsName: "",
    FloatPropertyParameters: undefined,
    BoolPropertyParameters: undefined,
    EventPropertyParameters: undefined,
    BoolPropertyState: undefined,
    FloatPropertyState: undefined,
    EventPropertyState: undefined,
  };
}

export const TIoTUserInfo_TProperty = {
  encode(
    message: TIoTUserInfo_TProperty,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.Retrievable === true) {
      writer.uint32(16).bool(message.Retrievable);
    }
    if (message.Reportable === true) {
      writer.uint32(24).bool(message.Reportable);
    }
    if (message.StateChangedAt !== 0) {
      writer.uint32(121).double(message.StateChangedAt);
    }
    if (message.LastUpdated !== 0) {
      writer.uint32(33).double(message.LastUpdated);
    }
    if (message.LastActivated !== 0) {
      writer.uint32(97).double(message.LastActivated);
    }
    if (message.AnalyticsType !== "") {
      writer.uint32(82).string(message.AnalyticsType);
    }
    if (message.AnalyticsName !== "") {
      writer.uint32(90).string(message.AnalyticsName);
    }
    if (message.FloatPropertyParameters !== undefined) {
      TIoTUserInfo_TProperty_TFloatPropertyParameters.encode(
        message.FloatPropertyParameters,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.BoolPropertyParameters !== undefined) {
      TIoTUserInfo_TProperty_TBoolPropertyParameters.encode(
        message.BoolPropertyParameters,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.EventPropertyParameters !== undefined) {
      TIoTUserInfo_TProperty_TEventPropertyParameters.encode(
        message.EventPropertyParameters,
        writer.uint32(106).fork()
      ).ldelim();
    }
    if (message.BoolPropertyState !== undefined) {
      TIoTUserInfo_TProperty_TBoolPropertyState.encode(
        message.BoolPropertyState,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.FloatPropertyState !== undefined) {
      TIoTUserInfo_TProperty_TFloatPropertyState.encode(
        message.FloatPropertyState,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.EventPropertyState !== undefined) {
      TIoTUserInfo_TProperty_TEventPropertyState.encode(
        message.EventPropertyState,
        writer.uint32(114).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTUserInfo_TProperty {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.Retrievable = reader.bool();
          break;
        case 3:
          message.Reportable = reader.bool();
          break;
        case 15:
          message.StateChangedAt = reader.double();
          break;
        case 4:
          message.LastUpdated = reader.double();
          break;
        case 12:
          message.LastActivated = reader.double();
          break;
        case 10:
          message.AnalyticsType = reader.string();
          break;
        case 11:
          message.AnalyticsName = reader.string();
          break;
        case 5:
          message.FloatPropertyParameters =
            TIoTUserInfo_TProperty_TFloatPropertyParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.BoolPropertyParameters =
            TIoTUserInfo_TProperty_TBoolPropertyParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 13:
          message.EventPropertyParameters =
            TIoTUserInfo_TProperty_TEventPropertyParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 8:
          message.BoolPropertyState =
            TIoTUserInfo_TProperty_TBoolPropertyState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 9:
          message.FloatPropertyState =
            TIoTUserInfo_TProperty_TFloatPropertyState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 14:
          message.EventPropertyState =
            TIoTUserInfo_TProperty_TEventPropertyState.decode(
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

  fromJSON(object: any): TIoTUserInfo_TProperty {
    return {
      Type: isSet(object.type)
        ? tIoTUserInfo_TProperty_EPropertyTypeFromJSON(object.type)
        : 0,
      Retrievable: isSet(object.retrievable)
        ? Boolean(object.retrievable)
        : false,
      Reportable: isSet(object.reportable) ? Boolean(object.reportable) : false,
      StateChangedAt: isSet(object.state_changed_at)
        ? Number(object.state_changed_at)
        : 0,
      LastUpdated: isSet(object.last_updated) ? Number(object.last_updated) : 0,
      LastActivated: isSet(object.last_activated)
        ? Number(object.last_activated)
        : 0,
      AnalyticsType: isSet(object.analytics_type)
        ? String(object.analytics_type)
        : "",
      AnalyticsName: isSet(object.analytics_name)
        ? String(object.analytics_name)
        : "",
      FloatPropertyParameters: isSet(object.float_property_parameters)
        ? TIoTUserInfo_TProperty_TFloatPropertyParameters.fromJSON(
            object.float_property_parameters
          )
        : undefined,
      BoolPropertyParameters: isSet(object.bool_property_parameters)
        ? TIoTUserInfo_TProperty_TBoolPropertyParameters.fromJSON(
            object.bool_property_parameters
          )
        : undefined,
      EventPropertyParameters: isSet(object.event_property_parameters)
        ? TIoTUserInfo_TProperty_TEventPropertyParameters.fromJSON(
            object.event_property_parameters
          )
        : undefined,
      BoolPropertyState: isSet(object.bool_property_state)
        ? TIoTUserInfo_TProperty_TBoolPropertyState.fromJSON(
            object.bool_property_state
          )
        : undefined,
      FloatPropertyState: isSet(object.float_property_state)
        ? TIoTUserInfo_TProperty_TFloatPropertyState.fromJSON(
            object.float_property_state
          )
        : undefined,
      EventPropertyState: isSet(object.event_property_state)
        ? TIoTUserInfo_TProperty_TEventPropertyState.fromJSON(
            object.event_property_state
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TProperty): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tIoTUserInfo_TProperty_EPropertyTypeToJSON(message.Type));
    message.Retrievable !== undefined &&
      (obj.retrievable = message.Retrievable);
    message.Reportable !== undefined && (obj.reportable = message.Reportable);
    message.StateChangedAt !== undefined &&
      (obj.state_changed_at = message.StateChangedAt);
    message.LastUpdated !== undefined &&
      (obj.last_updated = message.LastUpdated);
    message.LastActivated !== undefined &&
      (obj.last_activated = message.LastActivated);
    message.AnalyticsType !== undefined &&
      (obj.analytics_type = message.AnalyticsType);
    message.AnalyticsName !== undefined &&
      (obj.analytics_name = message.AnalyticsName);
    message.FloatPropertyParameters !== undefined &&
      (obj.float_property_parameters = message.FloatPropertyParameters
        ? TIoTUserInfo_TProperty_TFloatPropertyParameters.toJSON(
            message.FloatPropertyParameters
          )
        : undefined);
    message.BoolPropertyParameters !== undefined &&
      (obj.bool_property_parameters = message.BoolPropertyParameters
        ? TIoTUserInfo_TProperty_TBoolPropertyParameters.toJSON(
            message.BoolPropertyParameters
          )
        : undefined);
    message.EventPropertyParameters !== undefined &&
      (obj.event_property_parameters = message.EventPropertyParameters
        ? TIoTUserInfo_TProperty_TEventPropertyParameters.toJSON(
            message.EventPropertyParameters
          )
        : undefined);
    message.BoolPropertyState !== undefined &&
      (obj.bool_property_state = message.BoolPropertyState
        ? TIoTUserInfo_TProperty_TBoolPropertyState.toJSON(
            message.BoolPropertyState
          )
        : undefined);
    message.FloatPropertyState !== undefined &&
      (obj.float_property_state = message.FloatPropertyState
        ? TIoTUserInfo_TProperty_TFloatPropertyState.toJSON(
            message.FloatPropertyState
          )
        : undefined);
    message.EventPropertyState !== undefined &&
      (obj.event_property_state = message.EventPropertyState
        ? TIoTUserInfo_TProperty_TEventPropertyState.toJSON(
            message.EventPropertyState
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TFloatPropertyParameters(): TIoTUserInfo_TProperty_TFloatPropertyParameters {
  return { Instance: "", Unit: "" };
}

export const TIoTUserInfo_TProperty_TFloatPropertyParameters = {
  encode(
    message: TIoTUserInfo_TProperty_TFloatPropertyParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Unit !== "") {
      writer.uint32(18).string(message.Unit);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TFloatPropertyParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty_TFloatPropertyParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Unit = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TProperty_TFloatPropertyParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Unit: isSet(object.unit) ? String(object.unit) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TProperty_TFloatPropertyParameters): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Unit !== undefined && (obj.unit = message.Unit);
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TFloatPropertyState(): TIoTUserInfo_TProperty_TFloatPropertyState {
  return { Instance: "", Value: 0 };
}

export const TIoTUserInfo_TProperty_TFloatPropertyState = {
  encode(
    message: TIoTUserInfo_TProperty_TFloatPropertyState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value !== 0) {
      writer.uint32(17).double(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TFloatPropertyState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty_TFloatPropertyState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TProperty_TFloatPropertyState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? Number(object.value) : 0,
    };
  },

  toJSON(message: TIoTUserInfo_TProperty_TFloatPropertyState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TBoolPropertyParameters(): TIoTUserInfo_TProperty_TBoolPropertyParameters {
  return { Instance: "" };
}

export const TIoTUserInfo_TProperty_TBoolPropertyParameters = {
  encode(
    message: TIoTUserInfo_TProperty_TBoolPropertyParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TBoolPropertyParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty_TBoolPropertyParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TProperty_TBoolPropertyParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TProperty_TBoolPropertyParameters): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TBoolPropertyState(): TIoTUserInfo_TProperty_TBoolPropertyState {
  return { Instance: "", Value: false };
}

export const TIoTUserInfo_TProperty_TBoolPropertyState = {
  encode(
    message: TIoTUserInfo_TProperty_TBoolPropertyState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value === true) {
      writer.uint32(16).bool(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TBoolPropertyState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty_TBoolPropertyState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TProperty_TBoolPropertyState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? Boolean(object.value) : false,
    };
  },

  toJSON(message: TIoTUserInfo_TProperty_TBoolPropertyState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TEventPropertyParameters(): TIoTUserInfo_TProperty_TEventPropertyParameters {
  return { Instance: "", Events: [] };
}

export const TIoTUserInfo_TProperty_TEventPropertyParameters = {
  encode(
    message: TIoTUserInfo_TProperty_TEventPropertyParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    for (const v of message.Events) {
      TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TEventPropertyParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty_TEventPropertyParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Events.push(
            TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent.decode(
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

  fromJSON(object: any): TIoTUserInfo_TProperty_TEventPropertyParameters {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Events: Array.isArray(object?.events)
        ? object.events.map((e: any) =>
            TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TIoTUserInfo_TProperty_TEventPropertyParameters): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    if (message.Events) {
      obj.events = message.Events.map((e) =>
        e
          ? TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent.toJSON(e)
          : undefined
      );
    } else {
      obj.events = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TEventPropertyParameters_TEvent(): TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent {
  return { Name: "", Value: "" };
}

export const TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent = {
  encode(
    message: TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    if (message.Value !== "") {
      writer.uint32(18).string(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TProperty_TEventPropertyParameters_TEvent();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Value = reader.string();
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
  ): TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Value: isSet(object.value) ? String(object.value) : "",
    };
  },

  toJSON(
    message: TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent
  ): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TProperty_TEventPropertyState(): TIoTUserInfo_TProperty_TEventPropertyState {
  return { Instance: "", Value: "" };
}

export const TIoTUserInfo_TProperty_TEventPropertyState = {
  encode(
    message: TIoTUserInfo_TProperty_TEventPropertyState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== "") {
      writer.uint32(10).string(message.Instance);
    }
    if (message.Value !== "") {
      writer.uint32(18).string(message.Value);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TProperty_TEventPropertyState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TProperty_TEventPropertyState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.string();
          break;
        case 2:
          message.Value = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TProperty_TEventPropertyState {
    return {
      Instance: isSet(object.instance) ? String(object.instance) : "",
      Value: isSet(object.value) ? String(object.value) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TProperty_TEventPropertyState): unknown {
    const obj: any = {};
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.Value !== undefined && (obj.value = message.Value);
    return obj;
  },
};

function createBaseTIoTUserInfo_TDevice(): TIoTUserInfo_TDevice {
  return {
    Id: "",
    Name: "",
    Aliases: [],
    Type: 0,
    OriginalType: 0,
    ExternalId: "",
    ExternalName: "",
    SkillId: "",
    RoomId: "",
    GroupIds: [],
    Capabilities: [],
    Properties: [],
    DeviceInfo: undefined,
    QuasarInfo: undefined,
    CustomData: new Uint8Array(),
    Updated: 0,
    Created: 0,
    HouseholdId: "",
    Status: 0,
    StatusUpdated: 0,
    AnalyticsType: "",
    AnalyticsName: "",
    IconURL: "",
    Favorite: false,
    InternalConfig: undefined,
    SharingInfo: undefined,
  };
}

export const TIoTUserInfo_TDevice = {
  encode(
    message: TIoTUserInfo_TDevice,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    for (const v of message.Aliases) {
      writer.uint32(26).string(v!);
    }
    if (message.Type !== 0) {
      writer.uint32(32).int32(message.Type);
    }
    if (message.OriginalType !== 0) {
      writer.uint32(152).int32(message.OriginalType);
    }
    if (message.ExternalId !== "") {
      writer.uint32(50).string(message.ExternalId);
    }
    if (message.ExternalName !== "") {
      writer.uint32(58).string(message.ExternalName);
    }
    if (message.SkillId !== "") {
      writer.uint32(66).string(message.SkillId);
    }
    if (message.RoomId !== "") {
      writer.uint32(74).string(message.RoomId);
    }
    for (const v of message.GroupIds) {
      writer.uint32(82).string(v!);
    }
    for (const v of message.Capabilities) {
      TIoTUserInfo_TCapability.encode(v!, writer.uint32(90).fork()).ldelim();
    }
    for (const v of message.Properties) {
      TIoTUserInfo_TProperty.encode(v!, writer.uint32(98).fork()).ldelim();
    }
    if (message.DeviceInfo !== undefined) {
      TIoTUserInfo_TDevice_TDeviceInfo.encode(
        message.DeviceInfo,
        writer.uint32(106).fork()
      ).ldelim();
    }
    if (message.QuasarInfo !== undefined) {
      TIoTUserInfo_TDevice_TQuasarInfo.encode(
        message.QuasarInfo,
        writer.uint32(114).fork()
      ).ldelim();
    }
    if (message.CustomData.length !== 0) {
      writer.uint32(122).bytes(message.CustomData);
    }
    if (message.Updated !== 0) {
      writer.uint32(129).double(message.Updated);
    }
    if (message.Created !== 0) {
      writer.uint32(137).double(message.Created);
    }
    if (message.HouseholdId !== "") {
      writer.uint32(146).string(message.HouseholdId);
    }
    if (message.Status !== 0) {
      writer.uint32(160).int32(message.Status);
    }
    if (message.StatusUpdated !== 0) {
      writer.uint32(169).double(message.StatusUpdated);
    }
    if (message.AnalyticsType !== "") {
      writer.uint32(178).string(message.AnalyticsType);
    }
    if (message.AnalyticsName !== "") {
      writer.uint32(186).string(message.AnalyticsName);
    }
    if (message.IconURL !== "") {
      writer.uint32(194).string(message.IconURL);
    }
    if (message.Favorite === true) {
      writer.uint32(200).bool(message.Favorite);
    }
    if (message.InternalConfig !== undefined) {
      TIoTUserInfo_TDevice_TDeviceConfig.encode(
        message.InternalConfig,
        writer.uint32(210).fork()
      ).ldelim();
    }
    if (message.SharingInfo !== undefined) {
      TUserSharingInfo.encode(
        message.SharingInfo,
        writer.uint32(218).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTUserInfo_TDevice {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TDevice();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        case 3:
          message.Aliases.push(reader.string());
          break;
        case 4:
          message.Type = reader.int32() as any;
          break;
        case 19:
          message.OriginalType = reader.int32() as any;
          break;
        case 6:
          message.ExternalId = reader.string();
          break;
        case 7:
          message.ExternalName = reader.string();
          break;
        case 8:
          message.SkillId = reader.string();
          break;
        case 9:
          message.RoomId = reader.string();
          break;
        case 10:
          message.GroupIds.push(reader.string());
          break;
        case 11:
          message.Capabilities.push(
            TIoTUserInfo_TCapability.decode(reader, reader.uint32())
          );
          break;
        case 12:
          message.Properties.push(
            TIoTUserInfo_TProperty.decode(reader, reader.uint32())
          );
          break;
        case 13:
          message.DeviceInfo = TIoTUserInfo_TDevice_TDeviceInfo.decode(
            reader,
            reader.uint32()
          );
          break;
        case 14:
          message.QuasarInfo = TIoTUserInfo_TDevice_TQuasarInfo.decode(
            reader,
            reader.uint32()
          );
          break;
        case 15:
          message.CustomData = reader.bytes();
          break;
        case 16:
          message.Updated = reader.double();
          break;
        case 17:
          message.Created = reader.double();
          break;
        case 18:
          message.HouseholdId = reader.string();
          break;
        case 20:
          message.Status = reader.int32() as any;
          break;
        case 21:
          message.StatusUpdated = reader.double();
          break;
        case 22:
          message.AnalyticsType = reader.string();
          break;
        case 23:
          message.AnalyticsName = reader.string();
          break;
        case 24:
          message.IconURL = reader.string();
          break;
        case 25:
          message.Favorite = reader.bool();
          break;
        case 26:
          message.InternalConfig = TIoTUserInfo_TDevice_TDeviceConfig.decode(
            reader,
            reader.uint32()
          );
          break;
        case 27:
          message.SharingInfo = TUserSharingInfo.decode(
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

  fromJSON(object: any): TIoTUserInfo_TDevice {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      Aliases: Array.isArray(object?.aliases)
        ? object.aliases.map((e: any) => String(e))
        : [],
      Type: isSet(object.type) ? eUserDeviceTypeFromJSON(object.type) : 0,
      OriginalType: isSet(object.original_type)
        ? eUserDeviceTypeFromJSON(object.original_type)
        : 0,
      ExternalId: isSet(object.external_id) ? String(object.external_id) : "",
      ExternalName: isSet(object.external_name)
        ? String(object.external_name)
        : "",
      SkillId: isSet(object.skill_id) ? String(object.skill_id) : "",
      RoomId: isSet(object.room_id) ? String(object.room_id) : "",
      GroupIds: Array.isArray(object?.group_ids)
        ? object.group_ids.map((e: any) => String(e))
        : [],
      Capabilities: Array.isArray(object?.capabilities)
        ? object.capabilities.map((e: any) =>
            TIoTUserInfo_TCapability.fromJSON(e)
          )
        : [],
      Properties: Array.isArray(object?.properties)
        ? object.properties.map((e: any) => TIoTUserInfo_TProperty.fromJSON(e))
        : [],
      DeviceInfo: isSet(object.device_info)
        ? TIoTUserInfo_TDevice_TDeviceInfo.fromJSON(object.device_info)
        : undefined,
      QuasarInfo: isSet(object.quasar_info)
        ? TIoTUserInfo_TDevice_TQuasarInfo.fromJSON(object.quasar_info)
        : undefined,
      CustomData: isSet(object.custom_data)
        ? bytesFromBase64(object.custom_data)
        : new Uint8Array(),
      Updated: isSet(object.updated) ? Number(object.updated) : 0,
      Created: isSet(object.created) ? Number(object.created) : 0,
      HouseholdId: isSet(object.household_id)
        ? String(object.household_id)
        : "",
      Status: isSet(object.status)
        ? tIoTUserInfo_TDevice_EDeviceStateFromJSON(object.status)
        : 0,
      StatusUpdated: isSet(object.status_updated)
        ? Number(object.status_updated)
        : 0,
      AnalyticsType: isSet(object.analytics_type)
        ? String(object.analytics_type)
        : "",
      AnalyticsName: isSet(object.analytics_name)
        ? String(object.analytics_name)
        : "",
      IconURL: isSet(object.icon_url) ? String(object.icon_url) : "",
      Favorite: isSet(object.favorite) ? Boolean(object.favorite) : false,
      InternalConfig: isSet(object.internal_config)
        ? TIoTUserInfo_TDevice_TDeviceConfig.fromJSON(object.internal_config)
        : undefined,
      SharingInfo: isSet(object.sharing_info)
        ? TUserSharingInfo.fromJSON(object.sharing_info)
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TDevice): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Aliases) {
      obj.aliases = message.Aliases.map((e) => e);
    } else {
      obj.aliases = [];
    }
    message.Type !== undefined &&
      (obj.type = eUserDeviceTypeToJSON(message.Type));
    message.OriginalType !== undefined &&
      (obj.original_type = eUserDeviceTypeToJSON(message.OriginalType));
    message.ExternalId !== undefined && (obj.external_id = message.ExternalId);
    message.ExternalName !== undefined &&
      (obj.external_name = message.ExternalName);
    message.SkillId !== undefined && (obj.skill_id = message.SkillId);
    message.RoomId !== undefined && (obj.room_id = message.RoomId);
    if (message.GroupIds) {
      obj.group_ids = message.GroupIds.map((e) => e);
    } else {
      obj.group_ids = [];
    }
    if (message.Capabilities) {
      obj.capabilities = message.Capabilities.map((e) =>
        e ? TIoTUserInfo_TCapability.toJSON(e) : undefined
      );
    } else {
      obj.capabilities = [];
    }
    if (message.Properties) {
      obj.properties = message.Properties.map((e) =>
        e ? TIoTUserInfo_TProperty.toJSON(e) : undefined
      );
    } else {
      obj.properties = [];
    }
    message.DeviceInfo !== undefined &&
      (obj.device_info = message.DeviceInfo
        ? TIoTUserInfo_TDevice_TDeviceInfo.toJSON(message.DeviceInfo)
        : undefined);
    message.QuasarInfo !== undefined &&
      (obj.quasar_info = message.QuasarInfo
        ? TIoTUserInfo_TDevice_TQuasarInfo.toJSON(message.QuasarInfo)
        : undefined);
    message.CustomData !== undefined &&
      (obj.custom_data = base64FromBytes(
        message.CustomData !== undefined ? message.CustomData : new Uint8Array()
      ));
    message.Updated !== undefined && (obj.updated = message.Updated);
    message.Created !== undefined && (obj.created = message.Created);
    message.HouseholdId !== undefined &&
      (obj.household_id = message.HouseholdId);
    message.Status !== undefined &&
      (obj.status = tIoTUserInfo_TDevice_EDeviceStateToJSON(message.Status));
    message.StatusUpdated !== undefined &&
      (obj.status_updated = message.StatusUpdated);
    message.AnalyticsType !== undefined &&
      (obj.analytics_type = message.AnalyticsType);
    message.AnalyticsName !== undefined &&
      (obj.analytics_name = message.AnalyticsName);
    message.IconURL !== undefined && (obj.icon_url = message.IconURL);
    message.Favorite !== undefined && (obj.favorite = message.Favorite);
    message.InternalConfig !== undefined &&
      (obj.internal_config = message.InternalConfig
        ? TIoTUserInfo_TDevice_TDeviceConfig.toJSON(message.InternalConfig)
        : undefined);
    message.SharingInfo !== undefined &&
      (obj.sharing_info = message.SharingInfo
        ? TUserSharingInfo.toJSON(message.SharingInfo)
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TDevice_TDeviceInfo(): TIoTUserInfo_TDevice_TDeviceInfo {
  return { Manufacturer: "", Model: "", HwVersion: "", SwVersion: "" };
}

export const TIoTUserInfo_TDevice_TDeviceInfo = {
  encode(
    message: TIoTUserInfo_TDevice_TDeviceInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Manufacturer !== "") {
      writer.uint32(10).string(message.Manufacturer);
    }
    if (message.Model !== "") {
      writer.uint32(18).string(message.Model);
    }
    if (message.HwVersion !== "") {
      writer.uint32(26).string(message.HwVersion);
    }
    if (message.SwVersion !== "") {
      writer.uint32(34).string(message.SwVersion);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TDevice_TDeviceInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TDevice_TDeviceInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Manufacturer = reader.string();
          break;
        case 2:
          message.Model = reader.string();
          break;
        case 3:
          message.HwVersion = reader.string();
          break;
        case 4:
          message.SwVersion = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TDevice_TDeviceInfo {
    return {
      Manufacturer: isSet(object.manufacturer)
        ? String(object.manufacturer)
        : "",
      Model: isSet(object.model) ? String(object.model) : "",
      HwVersion: isSet(object.hw_version) ? String(object.hw_version) : "",
      SwVersion: isSet(object.sw_version) ? String(object.sw_version) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TDevice_TDeviceInfo): unknown {
    const obj: any = {};
    message.Manufacturer !== undefined &&
      (obj.manufacturer = message.Manufacturer);
    message.Model !== undefined && (obj.model = message.Model);
    message.HwVersion !== undefined && (obj.hw_version = message.HwVersion);
    message.SwVersion !== undefined && (obj.sw_version = message.SwVersion);
    return obj;
  },
};

function createBaseTIoTUserInfo_TDevice_TQuasarInfo(): TIoTUserInfo_TDevice_TQuasarInfo {
  return { DeviceId: "", Platform: "" };
}

export const TIoTUserInfo_TDevice_TQuasarInfo = {
  encode(
    message: TIoTUserInfo_TDevice_TQuasarInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DeviceId !== "") {
      writer.uint32(10).string(message.DeviceId);
    }
    if (message.Platform !== "") {
      writer.uint32(18).string(message.Platform);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TDevice_TQuasarInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TDevice_TQuasarInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceId = reader.string();
          break;
        case 2:
          message.Platform = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TDevice_TQuasarInfo {
    return {
      DeviceId: isSet(object.device_id) ? String(object.device_id) : "",
      Platform: isSet(object.platform) ? String(object.platform) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TDevice_TQuasarInfo): unknown {
    const obj: any = {};
    message.DeviceId !== undefined && (obj.device_id = message.DeviceId);
    message.Platform !== undefined && (obj.platform = message.Platform);
    return obj;
  },
};

function createBaseTIoTUserInfo_TDevice_TDeviceConfig(): TIoTUserInfo_TDevice_TDeviceConfig {
  return { Tandem: undefined, SpeakerYandexIOConfig: undefined };
}

export const TIoTUserInfo_TDevice_TDeviceConfig = {
  encode(
    message: TIoTUserInfo_TDevice_TDeviceConfig,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Tandem !== undefined) {
      TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig.encode(
        message.Tandem,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.SpeakerYandexIOConfig !== undefined) {
      TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig.encode(
        message.SpeakerYandexIOConfig,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TDevice_TDeviceConfig {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TDevice_TDeviceConfig();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Tandem =
            TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.SpeakerYandexIOConfig =
            TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig.decode(
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

  fromJSON(object: any): TIoTUserInfo_TDevice_TDeviceConfig {
    return {
      Tandem: isSet(object.tandem)
        ? TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig.fromJSON(
            object.tandem
          )
        : undefined,
      SpeakerYandexIOConfig: isSet(object.speaker_yandex_io)
        ? TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig.fromJSON(
            object.speaker_yandex_io
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TDevice_TDeviceConfig): unknown {
    const obj: any = {};
    message.Tandem !== undefined &&
      (obj.tandem = message.Tandem
        ? TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig.toJSON(
            message.Tandem
          )
        : undefined);
    message.SpeakerYandexIOConfig !== undefined &&
      (obj.speaker_yandex_io = message.SpeakerYandexIOConfig
        ? TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig.toJSON(
            message.SpeakerYandexIOConfig
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig(): TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig {
  return { Partner: undefined };
}

export const TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig = {
  encode(
    message: TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Partner !== undefined) {
      TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner.encode(
        message.Partner,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Partner =
            TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner.decode(
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
  ): TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig {
    return {
      Partner: isSet(object.partner)
        ? TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner.fromJSON(
            object.partner
          )
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig
  ): unknown {
    const obj: any = {};
    message.Partner !== undefined &&
      (obj.partner = message.Partner
        ? TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner.toJSON(
            message.Partner
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner(): TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner {
  return { Id: "" };
}

export const TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner =
  {
    encode(
      message: TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Id !== "") {
        writer.uint32(10).string(message.Id);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Id = reader.string();
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
    ): TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner {
      return {
        Id: isSet(object.id) ? String(object.id) : "",
      };
    },

    toJSON(
      message: TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner
    ): unknown {
      const obj: any = {};
      message.Id !== undefined && (obj.id = message.Id);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig(): TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig {
  return { Networks: undefined };
}

export const TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig = {
  encode(
    message: TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Networks !== undefined) {
      TIotDiscoveryCapability_TNetworks.encode(
        message.Networks,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Networks = TIotDiscoveryCapability_TNetworks.decode(
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
  ): TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig {
    return {
      Networks: isSet(object.networks)
        ? TIotDiscoveryCapability_TNetworks.fromJSON(object.networks)
        : undefined,
    };
  },

  toJSON(
    message: TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig
  ): unknown {
    const obj: any = {};
    message.Networks !== undefined &&
      (obj.networks = message.Networks
        ? TIotDiscoveryCapability_TNetworks.toJSON(message.Networks)
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_THousehold(): TIoTUserInfo_THousehold {
  return {
    Id: "",
    Name: "",
    Longitude: 0,
    Latitude: 0,
    Address: "",
    SharingInfo: undefined,
  };
}

export const TIoTUserInfo_THousehold = {
  encode(
    message: TIoTUserInfo_THousehold,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    if (message.Longitude !== 0) {
      writer.uint32(25).double(message.Longitude);
    }
    if (message.Latitude !== 0) {
      writer.uint32(33).double(message.Latitude);
    }
    if (message.Address !== "") {
      writer.uint32(42).string(message.Address);
    }
    if (message.SharingInfo !== undefined) {
      TUserSharingInfo.encode(
        message.SharingInfo,
        writer.uint32(50).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTUserInfo_THousehold {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_THousehold();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        case 3:
          message.Longitude = reader.double();
          break;
        case 4:
          message.Latitude = reader.double();
          break;
        case 5:
          message.Address = reader.string();
          break;
        case 6:
          message.SharingInfo = TUserSharingInfo.decode(
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

  fromJSON(object: any): TIoTUserInfo_THousehold {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      Longitude: isSet(object.longitude) ? Number(object.longitude) : 0,
      Latitude: isSet(object.latitude) ? Number(object.latitude) : 0,
      Address: isSet(object.address) ? String(object.address) : "",
      SharingInfo: isSet(object.sharing_info)
        ? TUserSharingInfo.fromJSON(object.sharing_info)
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_THousehold): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    message.Longitude !== undefined && (obj.longitude = message.Longitude);
    message.Latitude !== undefined && (obj.latitude = message.Latitude);
    message.Address !== undefined && (obj.address = message.Address);
    message.SharingInfo !== undefined &&
      (obj.sharing_info = message.SharingInfo
        ? TUserSharingInfo.toJSON(message.SharingInfo)
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TColor(): TIoTUserInfo_TColor {
  return { Id: "", Name: "" };
}

export const TIoTUserInfo_TColor = {
  encode(
    message: TIoTUserInfo_TColor,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTUserInfo_TColor {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TColor();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TColor {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TColor): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario(): TIoTUserInfo_TScenario {
  return {
    Id: "",
    Name: "",
    Icon: "",
    Devices: [],
    RequestedSpeakerCapabilities: [],
    Triggers: [],
    IsActive: false,
    Steps: [],
    PushOnInvoke: false,
  };
}

export const TIoTUserInfo_TScenario = {
  encode(
    message: TIoTUserInfo_TScenario,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    if (message.Icon !== "") {
      writer.uint32(26).string(message.Icon);
    }
    for (const v of message.Devices) {
      TIoTUserInfo_TScenario_TDevice.encode(
        v!,
        writer.uint32(34).fork()
      ).ldelim();
    }
    for (const v of message.RequestedSpeakerCapabilities) {
      TIoTUserInfo_TScenario_TCapability.encode(
        v!,
        writer.uint32(42).fork()
      ).ldelim();
    }
    for (const v of message.Triggers) {
      TIoTUserInfo_TScenario_TTrigger.encode(
        v!,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.IsActive === true) {
      writer.uint32(56).bool(message.IsActive);
    }
    for (const v of message.Steps) {
      TIoTUserInfo_TScenario_TStep.encode(
        v!,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.PushOnInvoke === true) {
      writer.uint32(72).bool(message.PushOnInvoke);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTUserInfo_TScenario {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        case 3:
          message.Icon = reader.string();
          break;
        case 4:
          message.Devices.push(
            TIoTUserInfo_TScenario_TDevice.decode(reader, reader.uint32())
          );
          break;
        case 5:
          message.RequestedSpeakerCapabilities.push(
            TIoTUserInfo_TScenario_TCapability.decode(reader, reader.uint32())
          );
          break;
        case 6:
          message.Triggers.push(
            TIoTUserInfo_TScenario_TTrigger.decode(reader, reader.uint32())
          );
          break;
        case 7:
          message.IsActive = reader.bool();
          break;
        case 8:
          message.Steps.push(
            TIoTUserInfo_TScenario_TStep.decode(reader, reader.uint32())
          );
          break;
        case 9:
          message.PushOnInvoke = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TScenario {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      Icon: isSet(object.icon) ? String(object.icon) : "",
      Devices: Array.isArray(object?.devices)
        ? object.devices.map((e: any) =>
            TIoTUserInfo_TScenario_TDevice.fromJSON(e)
          )
        : [],
      RequestedSpeakerCapabilities: Array.isArray(
        object?.requested_speaker_capabilities
      )
        ? object.requested_speaker_capabilities.map((e: any) =>
            TIoTUserInfo_TScenario_TCapability.fromJSON(e)
          )
        : [],
      Triggers: Array.isArray(object?.triggers)
        ? object.triggers.map((e: any) =>
            TIoTUserInfo_TScenario_TTrigger.fromJSON(e)
          )
        : [],
      IsActive: isSet(object.is_active) ? Boolean(object.is_active) : false,
      Steps: Array.isArray(object?.steps)
        ? object.steps.map((e: any) => TIoTUserInfo_TScenario_TStep.fromJSON(e))
        : [],
      PushOnInvoke: isSet(object.push_on_invoke)
        ? Boolean(object.push_on_invoke)
        : false,
    };
  },

  toJSON(message: TIoTUserInfo_TScenario): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    message.Icon !== undefined && (obj.icon = message.Icon);
    if (message.Devices) {
      obj.devices = message.Devices.map((e) =>
        e ? TIoTUserInfo_TScenario_TDevice.toJSON(e) : undefined
      );
    } else {
      obj.devices = [];
    }
    if (message.RequestedSpeakerCapabilities) {
      obj.requested_speaker_capabilities =
        message.RequestedSpeakerCapabilities.map((e) =>
          e ? TIoTUserInfo_TScenario_TCapability.toJSON(e) : undefined
        );
    } else {
      obj.requested_speaker_capabilities = [];
    }
    if (message.Triggers) {
      obj.triggers = message.Triggers.map((e) =>
        e ? TIoTUserInfo_TScenario_TTrigger.toJSON(e) : undefined
      );
    } else {
      obj.triggers = [];
    }
    message.IsActive !== undefined && (obj.is_active = message.IsActive);
    if (message.Steps) {
      obj.steps = message.Steps.map((e) =>
        e ? TIoTUserInfo_TScenario_TStep.toJSON(e) : undefined
      );
    } else {
      obj.steps = [];
    }
    message.PushOnInvoke !== undefined &&
      (obj.push_on_invoke = message.PushOnInvoke);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TCapability(): TIoTUserInfo_TScenario_TCapability {
  return {
    Type: 0,
    OnOffCapabilityState: undefined,
    ColorSettingCapabilityState: undefined,
    ModeCapabilityState: undefined,
    RangeCapabilityState: undefined,
    ToggleCapabilityState: undefined,
    CustomButtonCapabilityState: undefined,
    QuasarServerActionCapabilityState: undefined,
    QuasarCapabilityState: undefined,
    VideoStreamCapabilityState: undefined,
  };
}

export const TIoTUserInfo_TScenario_TCapability = {
  encode(
    message: TIoTUserInfo_TScenario_TCapability,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.OnOffCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TOnOffCapabilityState.encode(
        message.OnOffCapabilityState,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ColorSettingCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityState.encode(
        message.ColorSettingCapabilityState,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.ModeCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TModeCapabilityState.encode(
        message.ModeCapabilityState,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.RangeCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TRangeCapabilityState.encode(
        message.RangeCapabilityState,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.ToggleCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TToggleCapabilityState.encode(
        message.ToggleCapabilityState,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.CustomButtonCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TCustomButtonCapabilityState.encode(
        message.CustomButtonCapabilityState,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.QuasarServerActionCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.encode(
        message.QuasarServerActionCapabilityState,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.QuasarCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState.encode(
        message.QuasarCapabilityState,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.VideoStreamCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TVideoStreamCapabilityState.encode(
        message.VideoStreamCapabilityState,
        writer.uint32(82).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.OnOffCapabilityState =
            TIoTUserInfo_TCapability_TOnOffCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.ColorSettingCapabilityState =
            TIoTUserInfo_TCapability_TColorSettingCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.ModeCapabilityState =
            TIoTUserInfo_TCapability_TModeCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 5:
          message.RangeCapabilityState =
            TIoTUserInfo_TCapability_TRangeCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.ToggleCapabilityState =
            TIoTUserInfo_TCapability_TToggleCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 7:
          message.CustomButtonCapabilityState =
            TIoTUserInfo_TCapability_TCustomButtonCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 8:
          message.QuasarServerActionCapabilityState =
            TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 9:
          message.QuasarCapabilityState =
            TIoTUserInfo_TCapability_TQuasarCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 10:
          message.VideoStreamCapabilityState =
            TIoTUserInfo_TCapability_TVideoStreamCapabilityState.decode(
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

  fromJSON(object: any): TIoTUserInfo_TScenario_TCapability {
    return {
      Type: isSet(object.type)
        ? tIoTUserInfo_TCapability_ECapabilityTypeFromJSON(object.type)
        : 0,
      OnOffCapabilityState: isSet(object.on_off_capability_state)
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState.fromJSON(
            object.on_off_capability_state
          )
        : undefined,
      ColorSettingCapabilityState: isSet(object.color_setting_capability_state)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState.fromJSON(
            object.color_setting_capability_state
          )
        : undefined,
      ModeCapabilityState: isSet(object.mode_capability_state)
        ? TIoTUserInfo_TCapability_TModeCapabilityState.fromJSON(
            object.mode_capability_state
          )
        : undefined,
      RangeCapabilityState: isSet(object.range_capability_state)
        ? TIoTUserInfo_TCapability_TRangeCapabilityState.fromJSON(
            object.range_capability_state
          )
        : undefined,
      ToggleCapabilityState: isSet(object.toggle_capability_state)
        ? TIoTUserInfo_TCapability_TToggleCapabilityState.fromJSON(
            object.toggle_capability_state
          )
        : undefined,
      CustomButtonCapabilityState: isSet(object.custom_button_capability_state)
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityState.fromJSON(
            object.custom_button_capability_state
          )
        : undefined,
      QuasarServerActionCapabilityState: isSet(
        object.quasar_server_action_capability_state
      )
        ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.fromJSON(
            object.quasar_server_action_capability_state
          )
        : undefined,
      QuasarCapabilityState: isSet(object.quasar_capability_state)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState.fromJSON(
            object.quasar_capability_state
          )
        : undefined,
      VideoStreamCapabilityState: isSet(object.video_stream_capability_state)
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState.fromJSON(
            object.video_stream_capability_state
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TCapability): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tIoTUserInfo_TCapability_ECapabilityTypeToJSON(message.Type));
    message.OnOffCapabilityState !== undefined &&
      (obj.on_off_capability_state = message.OnOffCapabilityState
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState.toJSON(
            message.OnOffCapabilityState
          )
        : undefined);
    message.ColorSettingCapabilityState !== undefined &&
      (obj.color_setting_capability_state = message.ColorSettingCapabilityState
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState.toJSON(
            message.ColorSettingCapabilityState
          )
        : undefined);
    message.ModeCapabilityState !== undefined &&
      (obj.mode_capability_state = message.ModeCapabilityState
        ? TIoTUserInfo_TCapability_TModeCapabilityState.toJSON(
            message.ModeCapabilityState
          )
        : undefined);
    message.RangeCapabilityState !== undefined &&
      (obj.range_capability_state = message.RangeCapabilityState
        ? TIoTUserInfo_TCapability_TRangeCapabilityState.toJSON(
            message.RangeCapabilityState
          )
        : undefined);
    message.ToggleCapabilityState !== undefined &&
      (obj.toggle_capability_state = message.ToggleCapabilityState
        ? TIoTUserInfo_TCapability_TToggleCapabilityState.toJSON(
            message.ToggleCapabilityState
          )
        : undefined);
    message.CustomButtonCapabilityState !== undefined &&
      (obj.custom_button_capability_state = message.CustomButtonCapabilityState
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityState.toJSON(
            message.CustomButtonCapabilityState
          )
        : undefined);
    message.QuasarServerActionCapabilityState !== undefined &&
      (obj.quasar_server_action_capability_state =
        message.QuasarServerActionCapabilityState
          ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.toJSON(
              message.QuasarServerActionCapabilityState
            )
          : undefined);
    message.QuasarCapabilityState !== undefined &&
      (obj.quasar_capability_state = message.QuasarCapabilityState
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState.toJSON(
            message.QuasarCapabilityState
          )
        : undefined);
    message.VideoStreamCapabilityState !== undefined &&
      (obj.video_stream_capability_state = message.VideoStreamCapabilityState
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState.toJSON(
            message.VideoStreamCapabilityState
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TDevice(): TIoTUserInfo_TScenario_TDevice {
  return { Id: "", Capabilities: [] };
}

export const TIoTUserInfo_TScenario_TDevice = {
  encode(
    message: TIoTUserInfo_TScenario_TDevice,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    for (const v of message.Capabilities) {
      TIoTUserInfo_TScenario_TCapability.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TDevice {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TDevice();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Capabilities.push(
            TIoTUserInfo_TScenario_TCapability.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TScenario_TDevice {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Capabilities: Array.isArray(object?.capabilities)
        ? object.capabilities.map((e: any) =>
            TIoTUserInfo_TScenario_TCapability.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TDevice): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    if (message.Capabilities) {
      obj.capabilities = message.Capabilities.map((e) =>
        e ? TIoTUserInfo_TScenario_TCapability.toJSON(e) : undefined
      );
    } else {
      obj.capabilities = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TLaunchDevice(): TIoTUserInfo_TScenario_TLaunchDevice {
  return {
    Id: "",
    Name: "",
    Type: 0,
    Capabilities: [],
    CustomData: new Uint8Array(),
    SkillID: "",
  };
}

export const TIoTUserInfo_TScenario_TLaunchDevice = {
  encode(
    message: TIoTUserInfo_TScenario_TLaunchDevice,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    if (message.Type !== 0) {
      writer.uint32(24).int32(message.Type);
    }
    for (const v of message.Capabilities) {
      TIoTUserInfo_TCapability.encode(v!, writer.uint32(34).fork()).ldelim();
    }
    if (message.CustomData.length !== 0) {
      writer.uint32(42).bytes(message.CustomData);
    }
    if (message.SkillID !== "") {
      writer.uint32(50).string(message.SkillID);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TLaunchDevice {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TLaunchDevice();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        case 3:
          message.Type = reader.int32() as any;
          break;
        case 4:
          message.Capabilities.push(
            TIoTUserInfo_TCapability.decode(reader, reader.uint32())
          );
          break;
        case 5:
          message.CustomData = reader.bytes();
          break;
        case 6:
          message.SkillID = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TScenario_TLaunchDevice {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      Type: isSet(object.type) ? eUserDeviceTypeFromJSON(object.type) : 0,
      Capabilities: Array.isArray(object?.capabilities)
        ? object.capabilities.map((e: any) =>
            TIoTUserInfo_TCapability.fromJSON(e)
          )
        : [],
      CustomData: isSet(object.custom_data)
        ? bytesFromBase64(object.custom_data)
        : new Uint8Array(),
      SkillID: isSet(object.skill_id) ? String(object.skill_id) : "",
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TLaunchDevice): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    message.Type !== undefined &&
      (obj.type = eUserDeviceTypeToJSON(message.Type));
    if (message.Capabilities) {
      obj.capabilities = message.Capabilities.map((e) =>
        e ? TIoTUserInfo_TCapability.toJSON(e) : undefined
      );
    } else {
      obj.capabilities = [];
    }
    message.CustomData !== undefined &&
      (obj.custom_data = base64FromBytes(
        message.CustomData !== undefined ? message.CustomData : new Uint8Array()
      ));
    message.SkillID !== undefined && (obj.skill_id = message.SkillID);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TTrigger(): TIoTUserInfo_TScenario_TTrigger {
  return {
    Type: 0,
    VoiceTriggerPhrase: undefined,
    TimerTriggerTimestamp: undefined,
    Timetable: undefined,
    DeviceProperty: undefined,
  };
}

export const TIoTUserInfo_TScenario_TTrigger = {
  encode(
    message: TIoTUserInfo_TScenario_TTrigger,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.VoiceTriggerPhrase !== undefined) {
      writer.uint32(18).string(message.VoiceTriggerPhrase);
    }
    if (message.TimerTriggerTimestamp !== undefined) {
      writer.uint32(25).double(message.TimerTriggerTimestamp);
    }
    if (message.Timetable !== undefined) {
      TIoTUserInfo_TScenario_TTrigger_TTimetable.encode(
        message.Timetable,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.DeviceProperty !== undefined) {
      TIoTUserInfo_TScenario_TTrigger_TDeviceProperty.encode(
        message.DeviceProperty,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TTrigger {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TTrigger();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.VoiceTriggerPhrase = reader.string();
          break;
        case 3:
          message.TimerTriggerTimestamp = reader.double();
          break;
        case 4:
          message.Timetable = TIoTUserInfo_TScenario_TTrigger_TTimetable.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.DeviceProperty =
            TIoTUserInfo_TScenario_TTrigger_TDeviceProperty.decode(
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

  fromJSON(object: any): TIoTUserInfo_TScenario_TTrigger {
    return {
      Type: isSet(object.type)
        ? tIoTUserInfo_TScenario_TTrigger_ETriggerTypeFromJSON(object.type)
        : 0,
      VoiceTriggerPhrase: isSet(object.voice)
        ? String(object.voice)
        : undefined,
      TimerTriggerTimestamp: isSet(object.timer)
        ? Number(object.timer)
        : undefined,
      Timetable: isSet(object.timetable)
        ? TIoTUserInfo_TScenario_TTrigger_TTimetable.fromJSON(object.timetable)
        : undefined,
      DeviceProperty: isSet(object.device_property)
        ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty.fromJSON(
            object.device_property
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TTrigger): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tIoTUserInfo_TScenario_TTrigger_ETriggerTypeToJSON(
        message.Type
      ));
    message.VoiceTriggerPhrase !== undefined &&
      (obj.voice = message.VoiceTriggerPhrase);
    message.TimerTriggerTimestamp !== undefined &&
      (obj.timer = message.TimerTriggerTimestamp);
    message.Timetable !== undefined &&
      (obj.timetable = message.Timetable
        ? TIoTUserInfo_TScenario_TTrigger_TTimetable.toJSON(message.Timetable)
        : undefined);
    message.DeviceProperty !== undefined &&
      (obj.device_property = message.DeviceProperty
        ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty.toJSON(
            message.DeviceProperty
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TTrigger_TTimetable(): TIoTUserInfo_TScenario_TTrigger_TTimetable {
  return { TimeOffset: 0, Weekdays: [] };
}

export const TIoTUserInfo_TScenario_TTrigger_TTimetable = {
  encode(
    message: TIoTUserInfo_TScenario_TTrigger_TTimetable,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TimeOffset !== 0) {
      writer.uint32(9).double(message.TimeOffset);
    }
    writer.uint32(18).fork();
    for (const v of message.Weekdays) {
      writer.int32(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TTrigger_TTimetable {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TTrigger_TTimetable();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TimeOffset = reader.double();
          break;
        case 2:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.Weekdays.push(reader.int32());
            }
          } else {
            message.Weekdays.push(reader.int32());
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TScenario_TTrigger_TTimetable {
    return {
      TimeOffset: isSet(object.time_offset) ? Number(object.time_offset) : 0,
      Weekdays: Array.isArray(object?.weekdays)
        ? object.weekdays.map((e: any) => Number(e))
        : [],
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TTrigger_TTimetable): unknown {
    const obj: any = {};
    message.TimeOffset !== undefined && (obj.time_offset = message.TimeOffset);
    if (message.Weekdays) {
      obj.weekdays = message.Weekdays.map((e) => Math.round(e));
    } else {
      obj.weekdays = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty(): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty {
  return {
    DeviceID: "",
    PropertyType: "",
    Instance: "",
    ConditionType: 0,
    EventPropertyCondition: undefined,
    FloatPropertyCondition: undefined,
  };
}

export const TIoTUserInfo_TScenario_TTrigger_TDeviceProperty = {
  encode(
    message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DeviceID !== "") {
      writer.uint32(10).string(message.DeviceID);
    }
    if (message.PropertyType !== "") {
      writer.uint32(18).string(message.PropertyType);
    }
    if (message.Instance !== "") {
      writer.uint32(26).string(message.Instance);
    }
    if (message.ConditionType !== 0) {
      writer.uint32(32).int32(message.ConditionType);
    }
    if (message.EventPropertyCondition !== undefined) {
      TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition.encode(
        message.EventPropertyCondition,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.FloatPropertyCondition !== undefined) {
      TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition.encode(
        message.FloatPropertyCondition,
        writer.uint32(50).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceID = reader.string();
          break;
        case 2:
          message.PropertyType = reader.string();
          break;
        case 3:
          message.Instance = reader.string();
          break;
        case 4:
          message.ConditionType = reader.int32() as any;
          break;
        case 5:
          message.EventPropertyCondition =
            TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.FloatPropertyCondition =
            TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition.decode(
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

  fromJSON(object: any): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty {
    return {
      DeviceID: isSet(object.device_id) ? String(object.device_id) : "",
      PropertyType: isSet(object.property_type)
        ? String(object.property_type)
        : "",
      Instance: isSet(object.instance) ? String(object.instance) : "",
      ConditionType: isSet(object.condition_type)
        ? tIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionTypeFromJSON(
            object.condition_type
          )
        : 0,
      EventPropertyCondition: isSet(object.event_condition)
        ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition.fromJSON(
            object.event_condition
          )
        : undefined,
      FloatPropertyCondition: isSet(object.float_condition)
        ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition.fromJSON(
            object.float_condition
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty): unknown {
    const obj: any = {};
    message.DeviceID !== undefined && (obj.device_id = message.DeviceID);
    message.PropertyType !== undefined &&
      (obj.property_type = message.PropertyType);
    message.Instance !== undefined && (obj.instance = message.Instance);
    message.ConditionType !== undefined &&
      (obj.condition_type =
        tIoTUserInfo_TScenario_TTrigger_TDeviceProperty_PropertyTriggerConditionTypeToJSON(
          message.ConditionType
        ));
    message.EventPropertyCondition !== undefined &&
      (obj.event_condition = message.EventPropertyCondition
        ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition.toJSON(
            message.EventPropertyCondition
          )
        : undefined);
    message.FloatPropertyCondition !== undefined &&
      (obj.float_condition = message.FloatPropertyCondition
        ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition.toJSON(
            message.FloatPropertyCondition
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition(): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition {
  return { Values: [] };
}

export const TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition =
  {
    encode(
      message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition,
      writer: Writer = Writer.create()
    ): Writer {
      for (const v of message.Values) {
        writer.uint32(10).string(v!);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Values.push(reader.string());
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
    ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition {
      return {
        Values: Array.isArray(object?.values)
          ? object.values.map((e: any) => String(e))
          : [],
      };
    },

    toJSON(
      message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition
    ): unknown {
      const obj: any = {};
      if (message.Values) {
        obj.values = message.Values.map((e) => e);
      } else {
        obj.values = [];
      }
      return obj;
    },
  };

function createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition(): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition {
  return { LowerBound: undefined, UpperBound: undefined };
}

export const TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition =
  {
    encode(
      message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.LowerBound !== undefined) {
        TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.encode(
          message.LowerBound,
          writer.uint32(10).fork()
        ).ldelim();
      }
      if (message.UpperBound !== undefined) {
        TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.encode(
          message.UpperBound,
          writer.uint32(26).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.LowerBound =
              TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.decode(
                reader,
                reader.uint32()
              );
            break;
          case 3:
            message.UpperBound =
              TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.decode(
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
    ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition {
      return {
        LowerBound: isSet(object.lower_bound)
          ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.fromJSON(
              object.lower_bound
            )
          : undefined,
        UpperBound: isSet(object.upper_bound)
          ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.fromJSON(
              object.upper_bound
            )
          : undefined,
      };
    },

    toJSON(
      message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition
    ): unknown {
      const obj: any = {};
      message.LowerBound !== undefined &&
        (obj.lower_bound = message.LowerBound
          ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.toJSON(
              message.LowerBound
            )
          : undefined);
      message.UpperBound !== undefined &&
        (obj.upper_bound = message.UpperBound
          ? TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound.toJSON(
              message.UpperBound
            )
          : undefined);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound(): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound {
  return { Value: 0 };
}

export const TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound =
  {
    encode(
      message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Value !== 0) {
        writer.uint32(9).double(message.Value);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Value = reader.double();
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
    ): TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound {
      return {
        Value: isSet(object.value) ? Number(object.value) : 0,
      };
    },

    toJSON(
      message: TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound
    ): unknown {
      const obj: any = {};
      message.Value !== undefined && (obj.value = message.Value);
      return obj;
    },
  };

function createBaseTIoTUserInfo_TScenario_TStep(): TIoTUserInfo_TScenario_TStep {
  return {
    Type: 0,
    ScenarioStepActionsParameters: undefined,
    ScenarioStepDelayParameters: undefined,
  };
}

export const TIoTUserInfo_TScenario_TStep = {
  encode(
    message: TIoTUserInfo_TScenario_TStep,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.ScenarioStepActionsParameters !== undefined) {
      TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters.encode(
        message.ScenarioStepActionsParameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ScenarioStepDelayParameters !== undefined) {
      TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters.encode(
        message.ScenarioStepDelayParameters,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TStep {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TScenario_TStep();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.ScenarioStepActionsParameters =
            TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.ScenarioStepDelayParameters =
            TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters.decode(
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

  fromJSON(object: any): TIoTUserInfo_TScenario_TStep {
    return {
      Type: isSet(object.type)
        ? tIoTUserInfo_TScenario_TStep_EStepTypeFromJSON(object.type)
        : 0,
      ScenarioStepActionsParameters: isSet(
        object.scenario_step_actions_parameters
      )
        ? TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters.fromJSON(
            object.scenario_step_actions_parameters
          )
        : undefined,
      ScenarioStepDelayParameters: isSet(object.scenario_step_delay_parameters)
        ? TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters.fromJSON(
            object.scenario_step_delay_parameters
          )
        : undefined,
    };
  },

  toJSON(message: TIoTUserInfo_TScenario_TStep): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tIoTUserInfo_TScenario_TStep_EStepTypeToJSON(message.Type));
    message.ScenarioStepActionsParameters !== undefined &&
      (obj.scenario_step_actions_parameters =
        message.ScenarioStepActionsParameters
          ? TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters.toJSON(
              message.ScenarioStepActionsParameters
            )
          : undefined);
    message.ScenarioStepDelayParameters !== undefined &&
      (obj.scenario_step_delay_parameters = message.ScenarioStepDelayParameters
        ? TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters.toJSON(
            message.ScenarioStepDelayParameters
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters(): TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters {
  return { Devices: [], RequestedSpeakerCapabilities: [] };
}

export const TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters = {
  encode(
    message: TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.Devices) {
      TIoTUserInfo_TScenario_TLaunchDevice.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    for (const v of message.RequestedSpeakerCapabilities) {
      TIoTUserInfo_TScenario_TCapability.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Devices.push(
            TIoTUserInfo_TScenario_TLaunchDevice.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.RequestedSpeakerCapabilities.push(
            TIoTUserInfo_TScenario_TCapability.decode(reader, reader.uint32())
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
  ): TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters {
    return {
      Devices: Array.isArray(object?.devices)
        ? object.devices.map((e: any) =>
            TIoTUserInfo_TScenario_TLaunchDevice.fromJSON(e)
          )
        : [],
      RequestedSpeakerCapabilities: Array.isArray(
        object?.requested_speaker_capabilities
      )
        ? object.requested_speaker_capabilities.map((e: any) =>
            TIoTUserInfo_TScenario_TCapability.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(
    message: TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters
  ): unknown {
    const obj: any = {};
    if (message.Devices) {
      obj.devices = message.Devices.map((e) =>
        e ? TIoTUserInfo_TScenario_TLaunchDevice.toJSON(e) : undefined
      );
    } else {
      obj.devices = [];
    }
    if (message.RequestedSpeakerCapabilities) {
      obj.requested_speaker_capabilities =
        message.RequestedSpeakerCapabilities.map((e) =>
          e ? TIoTUserInfo_TScenario_TCapability.toJSON(e) : undefined
        );
    } else {
      obj.requested_speaker_capabilities = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters(): TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters {
  return { Delay: 0 };
}

export const TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters = {
  encode(
    message: TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Delay !== 0) {
      writer.uint32(9).double(message.Delay);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Delay = reader.double();
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
  ): TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters {
    return {
      Delay: isSet(object.delay) ? Number(object.delay) : 0,
    };
  },

  toJSON(
    message: TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters
  ): unknown {
    const obj: any = {};
    message.Delay !== undefined && (obj.delay = message.Delay);
    return obj;
  },
};

function createBaseTIoTUserInfo_TStereopair(): TIoTUserInfo_TStereopair {
  return { Id: "", Name: "", Devices: [] };
}

export const TIoTUserInfo_TStereopair = {
  encode(
    message: TIoTUserInfo_TStereopair,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Name !== "") {
      writer.uint32(18).string(message.Name);
    }
    for (const v of message.Devices) {
      TIoTUserInfo_TStereopair_TStereopairDevice.encode(
        v!,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TStereopair {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TStereopair();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Name = reader.string();
          break;
        case 3:
          message.Devices.push(
            TIoTUserInfo_TStereopair_TStereopairDevice.decode(
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

  fromJSON(object: any): TIoTUserInfo_TStereopair {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Name: isSet(object.name) ? String(object.name) : "",
      Devices: Array.isArray(object?.devices)
        ? object.devices.map((e: any) =>
            TIoTUserInfo_TStereopair_TStereopairDevice.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TIoTUserInfo_TStereopair): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Devices) {
      obj.devices = message.Devices.map((e) =>
        e ? TIoTUserInfo_TStereopair_TStereopairDevice.toJSON(e) : undefined
      );
    } else {
      obj.devices = [];
    }
    return obj;
  },
};

function createBaseTIoTUserInfo_TStereopair_TStereopairDevice(): TIoTUserInfo_TStereopair_TStereopairDevice {
  return { Id: "", Channel: 0, Role: 0 };
}

export const TIoTUserInfo_TStereopair_TStereopairDevice = {
  encode(
    message: TIoTUserInfo_TStereopair_TStereopairDevice,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== "") {
      writer.uint32(10).string(message.Id);
    }
    if (message.Channel !== 0) {
      writer.uint32(16).int32(message.Channel);
    }
    if (message.Role !== 0) {
      writer.uint32(24).int32(message.Role);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTUserInfo_TStereopair_TStereopairDevice {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTUserInfo_TStereopair_TStereopairDevice();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = reader.string();
          break;
        case 2:
          message.Channel = reader.int32() as any;
          break;
        case 3:
          message.Role = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTUserInfo_TStereopair_TStereopairDevice {
    return {
      Id: isSet(object.id) ? String(object.id) : "",
      Channel: isSet(object.channel)
        ? tIoTUserInfo_TStereopair_EStereopairChannelFromJSON(object.channel)
        : 0,
      Role: isSet(object.role)
        ? tIoTUserInfo_TStereopair_EStereopairRoleFromJSON(object.role)
        : 0,
    };
  },

  toJSON(message: TIoTUserInfo_TStereopair_TStereopairDevice): unknown {
    const obj: any = {};
    message.Id !== undefined && (obj.id = message.Id);
    message.Channel !== undefined &&
      (obj.channel = tIoTUserInfo_TStereopair_EStereopairChannelToJSON(
        message.Channel
      ));
    message.Role !== undefined &&
      (obj.role = tIoTUserInfo_TStereopair_EStereopairRoleToJSON(message.Role));
    return obj;
  },
};

function createBaseTIoTCapabilityAction(): TIoTCapabilityAction {
  return {
    Type: 0,
    OnOffCapabilityState: undefined,
    ColorSettingCapabilityState: undefined,
    ModeCapabilityState: undefined,
    RangeCapabilityState: undefined,
    ToggleCapabilityState: undefined,
    CustomButtonCapabilityState: undefined,
    QuasarServerActionCapabilityState: undefined,
    QuasarCapabilityState: undefined,
    VideoStreamCapabilityState: undefined,
  };
}

export const TIoTCapabilityAction = {
  encode(
    message: TIoTCapabilityAction,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Type !== 0) {
      writer.uint32(8).int32(message.Type);
    }
    if (message.OnOffCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TOnOffCapabilityState.encode(
        message.OnOffCapabilityState,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ColorSettingCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TColorSettingCapabilityState.encode(
        message.ColorSettingCapabilityState,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.ModeCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TModeCapabilityState.encode(
        message.ModeCapabilityState,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.RangeCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TRangeCapabilityState.encode(
        message.RangeCapabilityState,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.ToggleCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TToggleCapabilityState.encode(
        message.ToggleCapabilityState,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.CustomButtonCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TCustomButtonCapabilityState.encode(
        message.CustomButtonCapabilityState,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.QuasarServerActionCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.encode(
        message.QuasarServerActionCapabilityState,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.QuasarCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TQuasarCapabilityState.encode(
        message.QuasarCapabilityState,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.VideoStreamCapabilityState !== undefined) {
      TIoTUserInfo_TCapability_TVideoStreamCapabilityState.encode(
        message.VideoStreamCapabilityState,
        writer.uint32(82).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTCapabilityAction {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTCapabilityAction();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.int32() as any;
          break;
        case 2:
          message.OnOffCapabilityState =
            TIoTUserInfo_TCapability_TOnOffCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.ColorSettingCapabilityState =
            TIoTUserInfo_TCapability_TColorSettingCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.ModeCapabilityState =
            TIoTUserInfo_TCapability_TModeCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 5:
          message.RangeCapabilityState =
            TIoTUserInfo_TCapability_TRangeCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 6:
          message.ToggleCapabilityState =
            TIoTUserInfo_TCapability_TToggleCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 7:
          message.CustomButtonCapabilityState =
            TIoTUserInfo_TCapability_TCustomButtonCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 8:
          message.QuasarServerActionCapabilityState =
            TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 9:
          message.QuasarCapabilityState =
            TIoTUserInfo_TCapability_TQuasarCapabilityState.decode(
              reader,
              reader.uint32()
            );
          break;
        case 10:
          message.VideoStreamCapabilityState =
            TIoTUserInfo_TCapability_TVideoStreamCapabilityState.decode(
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

  fromJSON(object: any): TIoTCapabilityAction {
    return {
      Type: isSet(object.type)
        ? tIoTUserInfo_TCapability_ECapabilityTypeFromJSON(object.type)
        : 0,
      OnOffCapabilityState: isSet(object.on_off_capability_state)
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState.fromJSON(
            object.on_off_capability_state
          )
        : undefined,
      ColorSettingCapabilityState: isSet(object.color_setting_capability_state)
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState.fromJSON(
            object.color_setting_capability_state
          )
        : undefined,
      ModeCapabilityState: isSet(object.mode_capability_state)
        ? TIoTUserInfo_TCapability_TModeCapabilityState.fromJSON(
            object.mode_capability_state
          )
        : undefined,
      RangeCapabilityState: isSet(object.range_capability_state)
        ? TIoTUserInfo_TCapability_TRangeCapabilityState.fromJSON(
            object.range_capability_state
          )
        : undefined,
      ToggleCapabilityState: isSet(object.toggle_capability_state)
        ? TIoTUserInfo_TCapability_TToggleCapabilityState.fromJSON(
            object.toggle_capability_state
          )
        : undefined,
      CustomButtonCapabilityState: isSet(object.custom_button_capability_state)
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityState.fromJSON(
            object.custom_button_capability_state
          )
        : undefined,
      QuasarServerActionCapabilityState: isSet(
        object.quasar_server_action_capability_state
      )
        ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.fromJSON(
            object.quasar_server_action_capability_state
          )
        : undefined,
      QuasarCapabilityState: isSet(object.quasar_capability_state)
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState.fromJSON(
            object.quasar_capability_state
          )
        : undefined,
      VideoStreamCapabilityState: isSet(object.video_stream_capability_state)
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState.fromJSON(
            object.video_stream_capability_state
          )
        : undefined,
    };
  },

  toJSON(message: TIoTCapabilityAction): unknown {
    const obj: any = {};
    message.Type !== undefined &&
      (obj.type = tIoTUserInfo_TCapability_ECapabilityTypeToJSON(message.Type));
    message.OnOffCapabilityState !== undefined &&
      (obj.on_off_capability_state = message.OnOffCapabilityState
        ? TIoTUserInfo_TCapability_TOnOffCapabilityState.toJSON(
            message.OnOffCapabilityState
          )
        : undefined);
    message.ColorSettingCapabilityState !== undefined &&
      (obj.color_setting_capability_state = message.ColorSettingCapabilityState
        ? TIoTUserInfo_TCapability_TColorSettingCapabilityState.toJSON(
            message.ColorSettingCapabilityState
          )
        : undefined);
    message.ModeCapabilityState !== undefined &&
      (obj.mode_capability_state = message.ModeCapabilityState
        ? TIoTUserInfo_TCapability_TModeCapabilityState.toJSON(
            message.ModeCapabilityState
          )
        : undefined);
    message.RangeCapabilityState !== undefined &&
      (obj.range_capability_state = message.RangeCapabilityState
        ? TIoTUserInfo_TCapability_TRangeCapabilityState.toJSON(
            message.RangeCapabilityState
          )
        : undefined);
    message.ToggleCapabilityState !== undefined &&
      (obj.toggle_capability_state = message.ToggleCapabilityState
        ? TIoTUserInfo_TCapability_TToggleCapabilityState.toJSON(
            message.ToggleCapabilityState
          )
        : undefined);
    message.CustomButtonCapabilityState !== undefined &&
      (obj.custom_button_capability_state = message.CustomButtonCapabilityState
        ? TIoTUserInfo_TCapability_TCustomButtonCapabilityState.toJSON(
            message.CustomButtonCapabilityState
          )
        : undefined);
    message.QuasarServerActionCapabilityState !== undefined &&
      (obj.quasar_server_action_capability_state =
        message.QuasarServerActionCapabilityState
          ? TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState.toJSON(
              message.QuasarServerActionCapabilityState
            )
          : undefined);
    message.QuasarCapabilityState !== undefined &&
      (obj.quasar_capability_state = message.QuasarCapabilityState
        ? TIoTUserInfo_TCapability_TQuasarCapabilityState.toJSON(
            message.QuasarCapabilityState
          )
        : undefined);
    message.VideoStreamCapabilityState !== undefined &&
      (obj.video_stream_capability_state = message.VideoStreamCapabilityState
        ? TIoTUserInfo_TCapability_TVideoStreamCapabilityState.toJSON(
            message.VideoStreamCapabilityState
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTActionIntentParameters(): TIoTActionIntentParameters {
  return {
    CapabilityType: "",
    CapabilityInstance: "",
    CapabilityValue: undefined,
  };
}

export const TIoTActionIntentParameters = {
  encode(
    message: TIoTActionIntentParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CapabilityType !== "") {
      writer.uint32(10).string(message.CapabilityType);
    }
    if (message.CapabilityInstance !== "") {
      writer.uint32(18).string(message.CapabilityInstance);
    }
    if (message.CapabilityValue !== undefined) {
      TIoTActionIntentParameters_TCapabilityValue.encode(
        message.CapabilityValue,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTActionIntentParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTActionIntentParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CapabilityType = reader.string();
          break;
        case 2:
          message.CapabilityInstance = reader.string();
          break;
        case 3:
          message.CapabilityValue =
            TIoTActionIntentParameters_TCapabilityValue.decode(
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

  fromJSON(object: any): TIoTActionIntentParameters {
    return {
      CapabilityType: isSet(object.capability_type)
        ? String(object.capability_type)
        : "",
      CapabilityInstance: isSet(object.capability_instance)
        ? String(object.capability_instance)
        : "",
      CapabilityValue: isSet(object.capability_value)
        ? TIoTActionIntentParameters_TCapabilityValue.fromJSON(
            object.capability_value
          )
        : undefined,
    };
  },

  toJSON(message: TIoTActionIntentParameters): unknown {
    const obj: any = {};
    message.CapabilityType !== undefined &&
      (obj.capability_type = message.CapabilityType);
    message.CapabilityInstance !== undefined &&
      (obj.capability_instance = message.CapabilityInstance);
    message.CapabilityValue !== undefined &&
      (obj.capability_value = message.CapabilityValue
        ? TIoTActionIntentParameters_TCapabilityValue.toJSON(
            message.CapabilityValue
          )
        : undefined);
    return obj;
  },
};

function createBaseTIoTActionIntentParameters_TCapabilityValue(): TIoTActionIntentParameters_TCapabilityValue {
  return {
    RelativityType: "",
    Unit: "",
    BoolValue: undefined,
    NumValue: undefined,
    ModeValue: undefined,
  };
}

export const TIoTActionIntentParameters_TCapabilityValue = {
  encode(
    message: TIoTActionIntentParameters_TCapabilityValue,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RelativityType !== "") {
      writer.uint32(10).string(message.RelativityType);
    }
    if (message.Unit !== "") {
      writer.uint32(18).string(message.Unit);
    }
    if (message.BoolValue !== undefined) {
      writer.uint32(24).bool(message.BoolValue);
    }
    if (message.NumValue !== undefined) {
      writer.uint32(33).double(message.NumValue);
    }
    if (message.ModeValue !== undefined) {
      writer.uint32(42).string(message.ModeValue);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTActionIntentParameters_TCapabilityValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTActionIntentParameters_TCapabilityValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RelativityType = reader.string();
          break;
        case 2:
          message.Unit = reader.string();
          break;
        case 3:
          message.BoolValue = reader.bool();
          break;
        case 4:
          message.NumValue = reader.double();
          break;
        case 5:
          message.ModeValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTActionIntentParameters_TCapabilityValue {
    return {
      RelativityType: isSet(object.relativity_type)
        ? String(object.relativity_type)
        : "",
      Unit: isSet(object.unit) ? String(object.unit) : "",
      BoolValue: isSet(object.bool_value)
        ? Boolean(object.bool_value)
        : undefined,
      NumValue: isSet(object.num_value) ? Number(object.num_value) : undefined,
      ModeValue: isSet(object.mode_value)
        ? String(object.mode_value)
        : undefined,
    };
  },

  toJSON(message: TIoTActionIntentParameters_TCapabilityValue): unknown {
    const obj: any = {};
    message.RelativityType !== undefined &&
      (obj.relativity_type = message.RelativityType);
    message.Unit !== undefined && (obj.unit = message.Unit);
    message.BoolValue !== undefined && (obj.bool_value = message.BoolValue);
    message.NumValue !== undefined && (obj.num_value = message.NumValue);
    message.ModeValue !== undefined && (obj.mode_value = message.ModeValue);
    return obj;
  },
};

function createBaseTIoTDeviceActionRequest(): TIoTDeviceActionRequest {
  return {
    IntentParameters: undefined,
    RoomIDs: [],
    HouseholdIDs: [],
    GroupIDs: [],
    DeviceIDs: [],
    DeviceTypes: [],
    AtTimestamp: 0,
    FromTimestamp: 0,
    ToTimestamp: 0,
    ForTimestamp: 0,
  };
}

export const TIoTDeviceActionRequest = {
  encode(
    message: TIoTDeviceActionRequest,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.IntentParameters !== undefined) {
      TIoTActionIntentParameters.encode(
        message.IntentParameters,
        writer.uint32(10).fork()
      ).ldelim();
    }
    for (const v of message.RoomIDs) {
      writer.uint32(18).string(v!);
    }
    for (const v of message.HouseholdIDs) {
      writer.uint32(26).string(v!);
    }
    for (const v of message.GroupIDs) {
      writer.uint32(34).string(v!);
    }
    for (const v of message.DeviceIDs) {
      writer.uint32(42).string(v!);
    }
    for (const v of message.DeviceTypes) {
      writer.uint32(50).string(v!);
    }
    if (message.AtTimestamp !== 0) {
      writer.uint32(57).double(message.AtTimestamp);
    }
    if (message.FromTimestamp !== 0) {
      writer.uint32(65).double(message.FromTimestamp);
    }
    if (message.ToTimestamp !== 0) {
      writer.uint32(73).double(message.ToTimestamp);
    }
    if (message.ForTimestamp !== 0) {
      writer.uint32(81).double(message.ForTimestamp);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTDeviceActionRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDeviceActionRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.IntentParameters = TIoTActionIntentParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.RoomIDs.push(reader.string());
          break;
        case 3:
          message.HouseholdIDs.push(reader.string());
          break;
        case 4:
          message.GroupIDs.push(reader.string());
          break;
        case 5:
          message.DeviceIDs.push(reader.string());
          break;
        case 6:
          message.DeviceTypes.push(reader.string());
          break;
        case 7:
          message.AtTimestamp = reader.double();
          break;
        case 8:
          message.FromTimestamp = reader.double();
          break;
        case 9:
          message.ToTimestamp = reader.double();
          break;
        case 10:
          message.ForTimestamp = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTDeviceActionRequest {
    return {
      IntentParameters: isSet(object.intent_parameters)
        ? TIoTActionIntentParameters.fromJSON(object.intent_parameters)
        : undefined,
      RoomIDs: Array.isArray(object?.room_ids)
        ? object.room_ids.map((e: any) => String(e))
        : [],
      HouseholdIDs: Array.isArray(object?.household_ids)
        ? object.household_ids.map((e: any) => String(e))
        : [],
      GroupIDs: Array.isArray(object?.group_ids)
        ? object.group_ids.map((e: any) => String(e))
        : [],
      DeviceIDs: Array.isArray(object?.device_ids)
        ? object.device_ids.map((e: any) => String(e))
        : [],
      DeviceTypes: Array.isArray(object?.device_types)
        ? object.device_types.map((e: any) => String(e))
        : [],
      AtTimestamp: isSet(object.at_timestamp) ? Number(object.at_timestamp) : 0,
      FromTimestamp: isSet(object.from_timestamp)
        ? Number(object.from_timestamp)
        : 0,
      ToTimestamp: isSet(object.to_timestamp) ? Number(object.to_timestamp) : 0,
      ForTimestamp: isSet(object.for_timestamp)
        ? Number(object.for_timestamp)
        : 0,
    };
  },

  toJSON(message: TIoTDeviceActionRequest): unknown {
    const obj: any = {};
    message.IntentParameters !== undefined &&
      (obj.intent_parameters = message.IntentParameters
        ? TIoTActionIntentParameters.toJSON(message.IntentParameters)
        : undefined);
    if (message.RoomIDs) {
      obj.room_ids = message.RoomIDs.map((e) => e);
    } else {
      obj.room_ids = [];
    }
    if (message.HouseholdIDs) {
      obj.household_ids = message.HouseholdIDs.map((e) => e);
    } else {
      obj.household_ids = [];
    }
    if (message.GroupIDs) {
      obj.group_ids = message.GroupIDs.map((e) => e);
    } else {
      obj.group_ids = [];
    }
    if (message.DeviceIDs) {
      obj.device_ids = message.DeviceIDs.map((e) => e);
    } else {
      obj.device_ids = [];
    }
    if (message.DeviceTypes) {
      obj.device_types = message.DeviceTypes.map((e) => e);
    } else {
      obj.device_types = [];
    }
    message.AtTimestamp !== undefined &&
      (obj.at_timestamp = message.AtTimestamp);
    message.FromTimestamp !== undefined &&
      (obj.from_timestamp = message.FromTimestamp);
    message.ToTimestamp !== undefined &&
      (obj.to_timestamp = message.ToTimestamp);
    message.ForTimestamp !== undefined &&
      (obj.for_timestamp = message.ForTimestamp);
    return obj;
  },
};

function createBaseTStartIotDiscoveryRequest(): TStartIotDiscoveryRequest {
  return { Protocols: [] };
}

export const TStartIotDiscoveryRequest = {
  encode(
    message: TStartIotDiscoveryRequest,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.Protocols) {
      writer.int32(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TStartIotDiscoveryRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTStartIotDiscoveryRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.Protocols.push(reader.int32() as any);
            }
          } else {
            message.Protocols.push(reader.int32() as any);
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TStartIotDiscoveryRequest {
    return {
      Protocols: Array.isArray(object?.protocols)
        ? object.protocols.map((e: any) =>
            tIotDiscoveryCapability_TProtocolFromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TStartIotDiscoveryRequest): unknown {
    const obj: any = {};
    if (message.Protocols) {
      obj.protocols = message.Protocols.map((e) =>
        tIotDiscoveryCapability_TProtocolToJSON(e)
      );
    } else {
      obj.protocols = [];
    }
    return obj;
  },
};

function createBaseTFinishIotDiscoveryRequest(): TFinishIotDiscoveryRequest {
  return { Protocols: [], Networks: undefined, DiscoveredEndpoints: [] };
}

export const TFinishIotDiscoveryRequest = {
  encode(
    message: TFinishIotDiscoveryRequest,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.Protocols) {
      writer.int32(v);
    }
    writer.ldelim();
    if (message.Networks !== undefined) {
      TIotDiscoveryCapability_TNetworks.encode(
        message.Networks,
        writer.uint32(18).fork()
      ).ldelim();
    }
    for (const v of message.DiscoveredEndpoints) {
      TEndpoint.encode(v!, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TFinishIotDiscoveryRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFinishIotDiscoveryRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.Protocols.push(reader.int32() as any);
            }
          } else {
            message.Protocols.push(reader.int32() as any);
          }
          break;
        case 2:
          message.Networks = TIotDiscoveryCapability_TNetworks.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.DiscoveredEndpoints.push(
            TEndpoint.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFinishIotDiscoveryRequest {
    return {
      Protocols: Array.isArray(object?.protocols)
        ? object.protocols.map((e: any) =>
            tIotDiscoveryCapability_TProtocolFromJSON(e)
          )
        : [],
      Networks: isSet(object.networks)
        ? TIotDiscoveryCapability_TNetworks.fromJSON(object.networks)
        : undefined,
      DiscoveredEndpoints: Array.isArray(object?.discovered_endpoints)
        ? object.discovered_endpoints.map((e: any) => TEndpoint.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TFinishIotDiscoveryRequest): unknown {
    const obj: any = {};
    if (message.Protocols) {
      obj.protocols = message.Protocols.map((e) =>
        tIotDiscoveryCapability_TProtocolToJSON(e)
      );
    } else {
      obj.protocols = [];
    }
    message.Networks !== undefined &&
      (obj.networks = message.Networks
        ? TIotDiscoveryCapability_TNetworks.toJSON(message.Networks)
        : undefined);
    if (message.DiscoveredEndpoints) {
      obj.discovered_endpoints = message.DiscoveredEndpoints.map((e) =>
        e ? TEndpoint.toJSON(e) : undefined
      );
    } else {
      obj.discovered_endpoints = [];
    }
    return obj;
  },
};

function createBaseTForgetIotEndpointsRequest(): TForgetIotEndpointsRequest {
  return { EndpointIds: [] };
}

export const TForgetIotEndpointsRequest = {
  encode(
    message: TForgetIotEndpointsRequest,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.EndpointIds) {
      writer.uint32(10).string(v!);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TForgetIotEndpointsRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTForgetIotEndpointsRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EndpointIds.push(reader.string());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TForgetIotEndpointsRequest {
    return {
      EndpointIds: Array.isArray(object?.endpoint_ids)
        ? object.endpoint_ids.map((e: any) => String(e))
        : [],
    };
  },

  toJSON(message: TForgetIotEndpointsRequest): unknown {
    const obj: any = {};
    if (message.EndpointIds) {
      obj.endpoint_ids = message.EndpointIds.map((e) => e);
    } else {
      obj.endpoint_ids = [];
    }
    return obj;
  },
};

function createBaseTIoTDeviceActions(): TIoTDeviceActions {
  return { DeviceId: "", Actions: [], ExternalDeviceId: "", SkillId: "" };
}

export const TIoTDeviceActions = {
  encode(message: TIoTDeviceActions, writer: Writer = Writer.create()): Writer {
    if (message.DeviceId !== "") {
      writer.uint32(10).string(message.DeviceId);
    }
    for (const v of message.Actions) {
      TIoTCapabilityAction.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    if (message.ExternalDeviceId !== "") {
      writer.uint32(26).string(message.ExternalDeviceId);
    }
    if (message.SkillId !== "") {
      writer.uint32(34).string(message.SkillId);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTDeviceActions {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDeviceActions();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceId = reader.string();
          break;
        case 2:
          message.Actions.push(
            TIoTCapabilityAction.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.ExternalDeviceId = reader.string();
          break;
        case 4:
          message.SkillId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTDeviceActions {
    return {
      DeviceId: isSet(object.device_id) ? String(object.device_id) : "",
      Actions: Array.isArray(object?.actions)
        ? object.actions.map((e: any) => TIoTCapabilityAction.fromJSON(e))
        : [],
      ExternalDeviceId: isSet(object.external_device_id)
        ? String(object.external_device_id)
        : "",
      SkillId: isSet(object.skill_id) ? String(object.skill_id) : "",
    };
  },

  toJSON(message: TIoTDeviceActions): unknown {
    const obj: any = {};
    message.DeviceId !== undefined && (obj.device_id = message.DeviceId);
    if (message.Actions) {
      obj.actions = message.Actions.map((e) =>
        e ? TIoTCapabilityAction.toJSON(e) : undefined
      );
    } else {
      obj.actions = [];
    }
    message.ExternalDeviceId !== undefined &&
      (obj.external_device_id = message.ExternalDeviceId);
    message.SkillId !== undefined && (obj.skill_id = message.SkillId);
    return obj;
  },
};

function createBaseTIoTYandexIOActionRequest(): TIoTYandexIOActionRequest {
  return { EndpointActions: [] };
}

export const TIoTYandexIOActionRequest = {
  encode(
    message: TIoTYandexIOActionRequest,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.EndpointActions) {
      TIoTDeviceActions.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTYandexIOActionRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTYandexIOActionRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EndpointActions.push(
            TIoTDeviceActions.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTYandexIOActionRequest {
    return {
      EndpointActions: Array.isArray(object?.endpoint_actions)
        ? object.endpoint_actions.map((e: any) => TIoTDeviceActions.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TIoTYandexIOActionRequest): unknown {
    const obj: any = {};
    if (message.EndpointActions) {
      obj.endpoint_actions = message.EndpointActions.map((e) =>
        e ? TIoTDeviceActions.toJSON(e) : undefined
      );
    } else {
      obj.endpoint_actions = [];
    }
    return obj;
  },
};

function createBaseTIoTDeviceActionsBatch(): TIoTDeviceActionsBatch {
  return { Batch: [] };
}

export const TIoTDeviceActionsBatch = {
  encode(
    message: TIoTDeviceActionsBatch,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.Batch) {
      TIoTDeviceActions.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIoTDeviceActionsBatch {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDeviceActionsBatch();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Batch.push(TIoTDeviceActions.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTDeviceActionsBatch {
    return {
      Batch: Array.isArray(object?.batch)
        ? object.batch.map((e: any) => TIoTDeviceActions.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TIoTDeviceActionsBatch): unknown {
    const obj: any = {};
    if (message.Batch) {
      obj.batch = message.Batch.map((e) =>
        e ? TIoTDeviceActions.toJSON(e) : undefined
      );
    } else {
      obj.batch = [];
    }
    return obj;
  },
};

function createBaseTEndpointStateUpdatesRequest(): TEndpointStateUpdatesRequest {
  return { EndpointUpdates: [] };
}

export const TEndpointStateUpdatesRequest = {
  encode(
    message: TEndpointStateUpdatesRequest,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.EndpointUpdates) {
      TEndpoint.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointStateUpdatesRequest {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointStateUpdatesRequest();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EndpointUpdates.push(
            TEndpoint.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEndpointStateUpdatesRequest {
    return {
      EndpointUpdates: Array.isArray(object?.endpoint_updates)
        ? object.endpoint_updates.map((e: any) => TEndpoint.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TEndpointStateUpdatesRequest): unknown {
    const obj: any = {};
    if (message.EndpointUpdates) {
      obj.endpoint_updates = message.EndpointUpdates.map((e) =>
        e ? TEndpoint.toJSON(e) : undefined
      );
    } else {
      obj.endpoint_updates = [];
    }
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

const atob: (b64: string) => string =
  globalThis.atob ||
  ((b64) => globalThis.Buffer.from(b64, "base64").toString("binary"));
function bytesFromBase64(b64: string): Uint8Array {
  const bin = atob(b64);
  const arr = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; ++i) {
    arr[i] = bin.charCodeAt(i);
  }
  return arr;
}

const btoa: (bin: string) => string =
  globalThis.btoa ||
  ((bin) => globalThis.Buffer.from(bin, "binary").toString("base64"));
function base64FromBytes(arr: Uint8Array): string {
  const bin: string[] = [];
  for (const byte of arr) {
    bin.push(String.fromCharCode(byte));
  }
  return btoa(bin.join(""));
}

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
