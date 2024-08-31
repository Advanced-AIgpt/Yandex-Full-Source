/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import {
  TRange,
  EUnit,
  TPositiveIntegerRange,
  eUnitFromJSON,
  eUnitToJSON,
} from "../../../alice/protos/endpoint/common";
import {
  BytesValue,
  StringValue,
  DoubleValue,
} from "../../../google/protobuf/wrappers";
import { Struct } from "../../../google/protobuf/struct";

export const protobufPackage = "NAlice";

export interface TCapability {}

export enum TCapability_ECapabilityType {
  UnknownCapabilityType = 0,
  OnOffCapabilityType = 1,
  IotDiscoveryCapabilityType = 2,
  LevelCapabilityType = 3,
  ColorCapabilityType = 4,
  WebOSCapabilityType = 5,
  ButtonCapabilityType = 6,
  EqualizerCapabilityType = 7,
  AnimationCapabilityType = 8,
  MotionCapabilityType = 9,
  VideoCallCapabilityType = 10,
  RouteManagerCapabilityType = 11,
  OpeningSensorCapabilityType = 12,
  VibrationSensorCapabilityType = 13,
  WaterLeakSensorCapabilityType = 14,
  BatteryCapabilityType = 15,
  RangeCheckCapabilityType = 16,
  DeviceStateCapabilityType = 17,
  DivViewCapabilityType = 18,
  BioCapabilityType = 19,
  ScreensaverCapabilityType = 20,
  AlarmCapabilityType = 21,
  IotScenariosCapabilityType = 22,
  VolumeCapabilityType = 23,
  AudioFilePlayerCapabilityType = 24,
  LayeredDivUICapabilityType = 25,
  UNRECOGNIZED = -1,
}

export function tCapability_ECapabilityTypeFromJSON(
  object: any
): TCapability_ECapabilityType {
  switch (object) {
    case 0:
    case "UnknownCapabilityType":
      return TCapability_ECapabilityType.UnknownCapabilityType;
    case 1:
    case "OnOffCapabilityType":
      return TCapability_ECapabilityType.OnOffCapabilityType;
    case 2:
    case "IotDiscoveryCapabilityType":
      return TCapability_ECapabilityType.IotDiscoveryCapabilityType;
    case 3:
    case "LevelCapabilityType":
      return TCapability_ECapabilityType.LevelCapabilityType;
    case 4:
    case "ColorCapabilityType":
      return TCapability_ECapabilityType.ColorCapabilityType;
    case 5:
    case "WebOSCapabilityType":
      return TCapability_ECapabilityType.WebOSCapabilityType;
    case 6:
    case "ButtonCapabilityType":
      return TCapability_ECapabilityType.ButtonCapabilityType;
    case 7:
    case "EqualizerCapabilityType":
      return TCapability_ECapabilityType.EqualizerCapabilityType;
    case 8:
    case "AnimationCapabilityType":
      return TCapability_ECapabilityType.AnimationCapabilityType;
    case 9:
    case "MotionCapabilityType":
      return TCapability_ECapabilityType.MotionCapabilityType;
    case 10:
    case "VideoCallCapabilityType":
      return TCapability_ECapabilityType.VideoCallCapabilityType;
    case 11:
    case "RouteManagerCapabilityType":
      return TCapability_ECapabilityType.RouteManagerCapabilityType;
    case 12:
    case "OpeningSensorCapabilityType":
      return TCapability_ECapabilityType.OpeningSensorCapabilityType;
    case 13:
    case "VibrationSensorCapabilityType":
      return TCapability_ECapabilityType.VibrationSensorCapabilityType;
    case 14:
    case "WaterLeakSensorCapabilityType":
      return TCapability_ECapabilityType.WaterLeakSensorCapabilityType;
    case 15:
    case "BatteryCapabilityType":
      return TCapability_ECapabilityType.BatteryCapabilityType;
    case 16:
    case "RangeCheckCapabilityType":
      return TCapability_ECapabilityType.RangeCheckCapabilityType;
    case 17:
    case "DeviceStateCapabilityType":
      return TCapability_ECapabilityType.DeviceStateCapabilityType;
    case 18:
    case "DivViewCapabilityType":
      return TCapability_ECapabilityType.DivViewCapabilityType;
    case 19:
    case "BioCapabilityType":
      return TCapability_ECapabilityType.BioCapabilityType;
    case 20:
    case "ScreensaverCapabilityType":
      return TCapability_ECapabilityType.ScreensaverCapabilityType;
    case 21:
    case "AlarmCapabilityType":
      return TCapability_ECapabilityType.AlarmCapabilityType;
    case 22:
    case "IotScenariosCapabilityType":
      return TCapability_ECapabilityType.IotScenariosCapabilityType;
    case 23:
    case "VolumeCapabilityType":
      return TCapability_ECapabilityType.VolumeCapabilityType;
    case 24:
    case "AudioFilePlayerCapabilityType":
      return TCapability_ECapabilityType.AudioFilePlayerCapabilityType;
    case 25:
    case "LayeredDivUICapabilityType":
      return TCapability_ECapabilityType.LayeredDivUICapabilityType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TCapability_ECapabilityType.UNRECOGNIZED;
  }
}

export function tCapability_ECapabilityTypeToJSON(
  object: TCapability_ECapabilityType
): string {
  switch (object) {
    case TCapability_ECapabilityType.UnknownCapabilityType:
      return "UnknownCapabilityType";
    case TCapability_ECapabilityType.OnOffCapabilityType:
      return "OnOffCapabilityType";
    case TCapability_ECapabilityType.IotDiscoveryCapabilityType:
      return "IotDiscoveryCapabilityType";
    case TCapability_ECapabilityType.LevelCapabilityType:
      return "LevelCapabilityType";
    case TCapability_ECapabilityType.ColorCapabilityType:
      return "ColorCapabilityType";
    case TCapability_ECapabilityType.WebOSCapabilityType:
      return "WebOSCapabilityType";
    case TCapability_ECapabilityType.ButtonCapabilityType:
      return "ButtonCapabilityType";
    case TCapability_ECapabilityType.EqualizerCapabilityType:
      return "EqualizerCapabilityType";
    case TCapability_ECapabilityType.AnimationCapabilityType:
      return "AnimationCapabilityType";
    case TCapability_ECapabilityType.MotionCapabilityType:
      return "MotionCapabilityType";
    case TCapability_ECapabilityType.VideoCallCapabilityType:
      return "VideoCallCapabilityType";
    case TCapability_ECapabilityType.RouteManagerCapabilityType:
      return "RouteManagerCapabilityType";
    case TCapability_ECapabilityType.OpeningSensorCapabilityType:
      return "OpeningSensorCapabilityType";
    case TCapability_ECapabilityType.VibrationSensorCapabilityType:
      return "VibrationSensorCapabilityType";
    case TCapability_ECapabilityType.WaterLeakSensorCapabilityType:
      return "WaterLeakSensorCapabilityType";
    case TCapability_ECapabilityType.BatteryCapabilityType:
      return "BatteryCapabilityType";
    case TCapability_ECapabilityType.RangeCheckCapabilityType:
      return "RangeCheckCapabilityType";
    case TCapability_ECapabilityType.DeviceStateCapabilityType:
      return "DeviceStateCapabilityType";
    case TCapability_ECapabilityType.DivViewCapabilityType:
      return "DivViewCapabilityType";
    case TCapability_ECapabilityType.BioCapabilityType:
      return "BioCapabilityType";
    case TCapability_ECapabilityType.ScreensaverCapabilityType:
      return "ScreensaverCapabilityType";
    case TCapability_ECapabilityType.AlarmCapabilityType:
      return "AlarmCapabilityType";
    case TCapability_ECapabilityType.IotScenariosCapabilityType:
      return "IotScenariosCapabilityType";
    case TCapability_ECapabilityType.VolumeCapabilityType:
      return "VolumeCapabilityType";
    case TCapability_ECapabilityType.AudioFilePlayerCapabilityType:
      return "AudioFilePlayerCapabilityType";
    case TCapability_ECapabilityType.LayeredDivUICapabilityType:
      return "LayeredDivUICapabilityType";
    default:
      return "UNKNOWN";
  }
}

export enum TCapability_EEventType {
  UnknownEventType = 0,
  /** ButtonClickEventType - buttons, toggles and switches */
  ButtonClickEventType = 1,
  ButtonDoubleClickEventType = 2,
  ButtonLongPressEventType = 3,
  ButtonLongReleaseEventType = 4,
  /** AudioPlayerNextTrackEventType - audio_player event examples */
  AudioPlayerNextTrackEventType = 5,
  AudioPlayerPauseEventType = 6,
  /** MotionDetectedEventType - motion events */
  MotionDetectedEventType = 7,
  /** WaterLeakSensorLeakEventType - water leak sensor events */
  WaterLeakSensorLeakEventType = 8,
  WaterLeakSensorDryEventType = 9,
  /** VibrationSensorVibrationEventType - vibration sensor events */
  VibrationSensorVibrationEventType = 10,
  VibrationSensorTiltEventType = 11,
  VibrationSensorFallEventType = 12,
  /** OpeningSensorOpenedEventType - opening sensor events */
  OpeningSensorOpenedEventType = 13,
  OpeningSensorClosedEventType = 14,
  /** LevelUpdateStateEventType - level capability events */
  LevelUpdateStateEventType = 15,
  /** OnOffUpdateStateEventType - onOff capability events */
  OnOffUpdateStateEventType = 16,
  /** ColorUpdateStateEventType - color capability events */
  ColorUpdateStateEventType = 17,
  /** BatteryUpdateStateEventType - battery capability events */
  BatteryUpdateStateEventType = 18,
  /** RangeCheckEventType - range check capability events */
  RangeCheckEventType = 19,
  /** LocalStepsFinishedEventType - iot scenarios capability events */
  LocalStepsFinishedEventType = 20,
  UNRECOGNIZED = -1,
}

export function tCapability_EEventTypeFromJSON(
  object: any
): TCapability_EEventType {
  switch (object) {
    case 0:
    case "UnknownEventType":
      return TCapability_EEventType.UnknownEventType;
    case 1:
    case "ButtonClickEventType":
      return TCapability_EEventType.ButtonClickEventType;
    case 2:
    case "ButtonDoubleClickEventType":
      return TCapability_EEventType.ButtonDoubleClickEventType;
    case 3:
    case "ButtonLongPressEventType":
      return TCapability_EEventType.ButtonLongPressEventType;
    case 4:
    case "ButtonLongReleaseEventType":
      return TCapability_EEventType.ButtonLongReleaseEventType;
    case 5:
    case "AudioPlayerNextTrackEventType":
      return TCapability_EEventType.AudioPlayerNextTrackEventType;
    case 6:
    case "AudioPlayerPauseEventType":
      return TCapability_EEventType.AudioPlayerPauseEventType;
    case 7:
    case "MotionDetectedEventType":
      return TCapability_EEventType.MotionDetectedEventType;
    case 8:
    case "WaterLeakSensorLeakEventType":
      return TCapability_EEventType.WaterLeakSensorLeakEventType;
    case 9:
    case "WaterLeakSensorDryEventType":
      return TCapability_EEventType.WaterLeakSensorDryEventType;
    case 10:
    case "VibrationSensorVibrationEventType":
      return TCapability_EEventType.VibrationSensorVibrationEventType;
    case 11:
    case "VibrationSensorTiltEventType":
      return TCapability_EEventType.VibrationSensorTiltEventType;
    case 12:
    case "VibrationSensorFallEventType":
      return TCapability_EEventType.VibrationSensorFallEventType;
    case 13:
    case "OpeningSensorOpenedEventType":
      return TCapability_EEventType.OpeningSensorOpenedEventType;
    case 14:
    case "OpeningSensorClosedEventType":
      return TCapability_EEventType.OpeningSensorClosedEventType;
    case 15:
    case "LevelUpdateStateEventType":
      return TCapability_EEventType.LevelUpdateStateEventType;
    case 16:
    case "OnOffUpdateStateEventType":
      return TCapability_EEventType.OnOffUpdateStateEventType;
    case 17:
    case "ColorUpdateStateEventType":
      return TCapability_EEventType.ColorUpdateStateEventType;
    case 18:
    case "BatteryUpdateStateEventType":
      return TCapability_EEventType.BatteryUpdateStateEventType;
    case 19:
    case "RangeCheckEventType":
      return TCapability_EEventType.RangeCheckEventType;
    case 20:
    case "LocalStepsFinishedEventType":
      return TCapability_EEventType.LocalStepsFinishedEventType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TCapability_EEventType.UNRECOGNIZED;
  }
}

export function tCapability_EEventTypeToJSON(
  object: TCapability_EEventType
): string {
  switch (object) {
    case TCapability_EEventType.UnknownEventType:
      return "UnknownEventType";
    case TCapability_EEventType.ButtonClickEventType:
      return "ButtonClickEventType";
    case TCapability_EEventType.ButtonDoubleClickEventType:
      return "ButtonDoubleClickEventType";
    case TCapability_EEventType.ButtonLongPressEventType:
      return "ButtonLongPressEventType";
    case TCapability_EEventType.ButtonLongReleaseEventType:
      return "ButtonLongReleaseEventType";
    case TCapability_EEventType.AudioPlayerNextTrackEventType:
      return "AudioPlayerNextTrackEventType";
    case TCapability_EEventType.AudioPlayerPauseEventType:
      return "AudioPlayerPauseEventType";
    case TCapability_EEventType.MotionDetectedEventType:
      return "MotionDetectedEventType";
    case TCapability_EEventType.WaterLeakSensorLeakEventType:
      return "WaterLeakSensorLeakEventType";
    case TCapability_EEventType.WaterLeakSensorDryEventType:
      return "WaterLeakSensorDryEventType";
    case TCapability_EEventType.VibrationSensorVibrationEventType:
      return "VibrationSensorVibrationEventType";
    case TCapability_EEventType.VibrationSensorTiltEventType:
      return "VibrationSensorTiltEventType";
    case TCapability_EEventType.VibrationSensorFallEventType:
      return "VibrationSensorFallEventType";
    case TCapability_EEventType.OpeningSensorOpenedEventType:
      return "OpeningSensorOpenedEventType";
    case TCapability_EEventType.OpeningSensorClosedEventType:
      return "OpeningSensorClosedEventType";
    case TCapability_EEventType.LevelUpdateStateEventType:
      return "LevelUpdateStateEventType";
    case TCapability_EEventType.OnOffUpdateStateEventType:
      return "OnOffUpdateStateEventType";
    case TCapability_EEventType.ColorUpdateStateEventType:
      return "ColorUpdateStateEventType";
    case TCapability_EEventType.BatteryUpdateStateEventType:
      return "BatteryUpdateStateEventType";
    case TCapability_EEventType.RangeCheckEventType:
      return "RangeCheckEventType";
    case TCapability_EEventType.LocalStepsFinishedEventType:
      return "LocalStepsFinishedEventType";
    default:
      return "UNKNOWN";
  }
}

export enum TCapability_EDirectiveType {
  UnknownDirectiveType = 0,
  /** OnOffDirectiveType - OnOffCapability directives */
  OnOffDirectiveType = 1,
  /** IotStartDiscoveryDirectiveType - IotDiscoveryCapability directives */
  IotStartDiscoveryDirectiveType = 2,
  IotFinishDiscoveryDirectiveType = 3,
  IotForgetDevicesDirectiveType = 4,
  IotStartTuyaBroadcastDirectiveType = 10,
  IotRestoreNetworksDirectiveType = 13,
  IotCancelDiscoveryDirectiveType = 14,
  IotDeleteNetworksDirectiveType = 15,
  IotEnableNetworkDirectiveType = 27,
  /** SetAbsoluteLevelDirectiveType - LevelCapability directives */
  SetAbsoluteLevelDirectiveType = 5,
  SetRelativeLevelDirectiveType = 6,
  StartMoveLevelDirectiveType = 7,
  StopMoveLevelDirectiveType = 8,
  /** SetColorSceneDirectiveType - ColorCapability directives */
  SetColorSceneDirectiveType = 9,
  SetTemperatureKDirectiveType = 19,
  /** WebOSLaunchAppDirectiveType - WebOSCapability directives */
  WebOSLaunchAppDirectiveType = 11,
  WebOSShowGalleryDirectiveType = 12,
  /** SetAdjustableEqualizerBandsDirectiveType - EqualizerCapability directives */
  SetAdjustableEqualizerBandsDirectiveType = 16,
  SetFixedEqualizerBandsDirectiveType = 17,
  /** DrawAnimationDirectiveType - AnimationCapability directives */
  DrawAnimationDirectiveType = 18,
  EnableScreenDirectiveType = 38,
  DisableScreenDirectiveType = 39,
  /** StartVideoCallLoginDirectiveType - VideoCallCapability directives */
  StartVideoCallLoginDirectiveType = 20,
  StartVideoCallDirectiveType = 21,
  AcceptVideoCallDirectiveType = 22,
  DiscardVideoCallDirectiveType = 23,
  VideoCallMuteMicDirectiveType = 42,
  VideoCallUnmuteMicDirectiveType = 43,
  VideoCallTurnOnVideoDirectiveType = 44,
  VideoCallTurnOffVideoDirectiveType = 45,
  /** StartRouteManagerDirectiveType - RouteManagerCapability directives */
  StartRouteManagerDirectiveType = 24,
  StopRouteManagerDirectiveType = 25,
  ShowRouteManagerDirectiveType = 26,
  ContinueRouteManagerDirectiveType = 28,
  /** OpenScreensaverDirectiveType - ControlsCapability directives */
  OpenScreensaverDirectiveType = 29,
  /** StashViewDirectiveType - DivViewCapability directives */
  StashViewDirectiveType = 30,
  UnstashViewDirectiveType = 31,
  /** AlarmAddDirectiveType - AlarmCapabilityType directives */
  AlarmAddDirectiveType = 32,
  AlarmRemoveDirectiveType = 33,
  AlarmUpdateDirectiveType = 34,
  /** AddIotScenariosDirectiveType - IotScenarioCapability directives */
  AddIotScenariosDirectiveType = 35,
  RemoveIotScenariosDirectiveType = 36,
  SyncIotScenariosDirectiveType = 37,
  /** VolumeMuteDirectiveType - VolumeCapability directives */
  VolumeMuteDirectiveType = 40,
  VolumeUnmuteDirectiveType = 41,
  /** BioStartSoundEnrollmentDirectiveType - BioCapability directives */
  BioStartSoundEnrollmentDirectiveType = 46,
  /** LocalAudioFilePlayDirectiveType - AudioFilePlayerCapabilityType directives */
  LocalAudioFilePlayDirectiveType = 47,
  LocalAudioFileStopDirectiveType = 48,
  /** DivUIShowViewDirectiveType - DivCards capabilities */
  DivUIShowViewDirectiveType = 49,
  DivUIPatchViewDirectiveType = 50,
  DivUIHideViewDirectiveType = 51,
  DivUIStashViewDirectiveType = 52,
  DivUIUnstashViewDirectiveType = 53,
  UNRECOGNIZED = -1,
}

export function tCapability_EDirectiveTypeFromJSON(
  object: any
): TCapability_EDirectiveType {
  switch (object) {
    case 0:
    case "UnknownDirectiveType":
      return TCapability_EDirectiveType.UnknownDirectiveType;
    case 1:
    case "OnOffDirectiveType":
      return TCapability_EDirectiveType.OnOffDirectiveType;
    case 2:
    case "IotStartDiscoveryDirectiveType":
      return TCapability_EDirectiveType.IotStartDiscoveryDirectiveType;
    case 3:
    case "IotFinishDiscoveryDirectiveType":
      return TCapability_EDirectiveType.IotFinishDiscoveryDirectiveType;
    case 4:
    case "IotForgetDevicesDirectiveType":
      return TCapability_EDirectiveType.IotForgetDevicesDirectiveType;
    case 10:
    case "IotStartTuyaBroadcastDirectiveType":
      return TCapability_EDirectiveType.IotStartTuyaBroadcastDirectiveType;
    case 13:
    case "IotRestoreNetworksDirectiveType":
      return TCapability_EDirectiveType.IotRestoreNetworksDirectiveType;
    case 14:
    case "IotCancelDiscoveryDirectiveType":
      return TCapability_EDirectiveType.IotCancelDiscoveryDirectiveType;
    case 15:
    case "IotDeleteNetworksDirectiveType":
      return TCapability_EDirectiveType.IotDeleteNetworksDirectiveType;
    case 27:
    case "IotEnableNetworkDirectiveType":
      return TCapability_EDirectiveType.IotEnableNetworkDirectiveType;
    case 5:
    case "SetAbsoluteLevelDirectiveType":
      return TCapability_EDirectiveType.SetAbsoluteLevelDirectiveType;
    case 6:
    case "SetRelativeLevelDirectiveType":
      return TCapability_EDirectiveType.SetRelativeLevelDirectiveType;
    case 7:
    case "StartMoveLevelDirectiveType":
      return TCapability_EDirectiveType.StartMoveLevelDirectiveType;
    case 8:
    case "StopMoveLevelDirectiveType":
      return TCapability_EDirectiveType.StopMoveLevelDirectiveType;
    case 9:
    case "SetColorSceneDirectiveType":
      return TCapability_EDirectiveType.SetColorSceneDirectiveType;
    case 19:
    case "SetTemperatureKDirectiveType":
      return TCapability_EDirectiveType.SetTemperatureKDirectiveType;
    case 11:
    case "WebOSLaunchAppDirectiveType":
      return TCapability_EDirectiveType.WebOSLaunchAppDirectiveType;
    case 12:
    case "WebOSShowGalleryDirectiveType":
      return TCapability_EDirectiveType.WebOSShowGalleryDirectiveType;
    case 16:
    case "SetAdjustableEqualizerBandsDirectiveType":
      return TCapability_EDirectiveType.SetAdjustableEqualizerBandsDirectiveType;
    case 17:
    case "SetFixedEqualizerBandsDirectiveType":
      return TCapability_EDirectiveType.SetFixedEqualizerBandsDirectiveType;
    case 18:
    case "DrawAnimationDirectiveType":
      return TCapability_EDirectiveType.DrawAnimationDirectiveType;
    case 38:
    case "EnableScreenDirectiveType":
      return TCapability_EDirectiveType.EnableScreenDirectiveType;
    case 39:
    case "DisableScreenDirectiveType":
      return TCapability_EDirectiveType.DisableScreenDirectiveType;
    case 20:
    case "StartVideoCallLoginDirectiveType":
      return TCapability_EDirectiveType.StartVideoCallLoginDirectiveType;
    case 21:
    case "StartVideoCallDirectiveType":
      return TCapability_EDirectiveType.StartVideoCallDirectiveType;
    case 22:
    case "AcceptVideoCallDirectiveType":
      return TCapability_EDirectiveType.AcceptVideoCallDirectiveType;
    case 23:
    case "DiscardVideoCallDirectiveType":
      return TCapability_EDirectiveType.DiscardVideoCallDirectiveType;
    case 42:
    case "VideoCallMuteMicDirectiveType":
      return TCapability_EDirectiveType.VideoCallMuteMicDirectiveType;
    case 43:
    case "VideoCallUnmuteMicDirectiveType":
      return TCapability_EDirectiveType.VideoCallUnmuteMicDirectiveType;
    case 44:
    case "VideoCallTurnOnVideoDirectiveType":
      return TCapability_EDirectiveType.VideoCallTurnOnVideoDirectiveType;
    case 45:
    case "VideoCallTurnOffVideoDirectiveType":
      return TCapability_EDirectiveType.VideoCallTurnOffVideoDirectiveType;
    case 24:
    case "StartRouteManagerDirectiveType":
      return TCapability_EDirectiveType.StartRouteManagerDirectiveType;
    case 25:
    case "StopRouteManagerDirectiveType":
      return TCapability_EDirectiveType.StopRouteManagerDirectiveType;
    case 26:
    case "ShowRouteManagerDirectiveType":
      return TCapability_EDirectiveType.ShowRouteManagerDirectiveType;
    case 28:
    case "ContinueRouteManagerDirectiveType":
      return TCapability_EDirectiveType.ContinueRouteManagerDirectiveType;
    case 29:
    case "OpenScreensaverDirectiveType":
      return TCapability_EDirectiveType.OpenScreensaverDirectiveType;
    case 30:
    case "StashViewDirectiveType":
      return TCapability_EDirectiveType.StashViewDirectiveType;
    case 31:
    case "UnstashViewDirectiveType":
      return TCapability_EDirectiveType.UnstashViewDirectiveType;
    case 32:
    case "AlarmAddDirectiveType":
      return TCapability_EDirectiveType.AlarmAddDirectiveType;
    case 33:
    case "AlarmRemoveDirectiveType":
      return TCapability_EDirectiveType.AlarmRemoveDirectiveType;
    case 34:
    case "AlarmUpdateDirectiveType":
      return TCapability_EDirectiveType.AlarmUpdateDirectiveType;
    case 35:
    case "AddIotScenariosDirectiveType":
      return TCapability_EDirectiveType.AddIotScenariosDirectiveType;
    case 36:
    case "RemoveIotScenariosDirectiveType":
      return TCapability_EDirectiveType.RemoveIotScenariosDirectiveType;
    case 37:
    case "SyncIotScenariosDirectiveType":
      return TCapability_EDirectiveType.SyncIotScenariosDirectiveType;
    case 40:
    case "VolumeMuteDirectiveType":
      return TCapability_EDirectiveType.VolumeMuteDirectiveType;
    case 41:
    case "VolumeUnmuteDirectiveType":
      return TCapability_EDirectiveType.VolumeUnmuteDirectiveType;
    case 46:
    case "BioStartSoundEnrollmentDirectiveType":
      return TCapability_EDirectiveType.BioStartSoundEnrollmentDirectiveType;
    case 47:
    case "LocalAudioFilePlayDirectiveType":
      return TCapability_EDirectiveType.LocalAudioFilePlayDirectiveType;
    case 48:
    case "LocalAudioFileStopDirectiveType":
      return TCapability_EDirectiveType.LocalAudioFileStopDirectiveType;
    case 49:
    case "DivUIShowViewDirectiveType":
      return TCapability_EDirectiveType.DivUIShowViewDirectiveType;
    case 50:
    case "DivUIPatchViewDirectiveType":
      return TCapability_EDirectiveType.DivUIPatchViewDirectiveType;
    case 51:
    case "DivUIHideViewDirectiveType":
      return TCapability_EDirectiveType.DivUIHideViewDirectiveType;
    case 52:
    case "DivUIStashViewDirectiveType":
      return TCapability_EDirectiveType.DivUIStashViewDirectiveType;
    case 53:
    case "DivUIUnstashViewDirectiveType":
      return TCapability_EDirectiveType.DivUIUnstashViewDirectiveType;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TCapability_EDirectiveType.UNRECOGNIZED;
  }
}

export function tCapability_EDirectiveTypeToJSON(
  object: TCapability_EDirectiveType
): string {
  switch (object) {
    case TCapability_EDirectiveType.UnknownDirectiveType:
      return "UnknownDirectiveType";
    case TCapability_EDirectiveType.OnOffDirectiveType:
      return "OnOffDirectiveType";
    case TCapability_EDirectiveType.IotStartDiscoveryDirectiveType:
      return "IotStartDiscoveryDirectiveType";
    case TCapability_EDirectiveType.IotFinishDiscoveryDirectiveType:
      return "IotFinishDiscoveryDirectiveType";
    case TCapability_EDirectiveType.IotForgetDevicesDirectiveType:
      return "IotForgetDevicesDirectiveType";
    case TCapability_EDirectiveType.IotStartTuyaBroadcastDirectiveType:
      return "IotStartTuyaBroadcastDirectiveType";
    case TCapability_EDirectiveType.IotRestoreNetworksDirectiveType:
      return "IotRestoreNetworksDirectiveType";
    case TCapability_EDirectiveType.IotCancelDiscoveryDirectiveType:
      return "IotCancelDiscoveryDirectiveType";
    case TCapability_EDirectiveType.IotDeleteNetworksDirectiveType:
      return "IotDeleteNetworksDirectiveType";
    case TCapability_EDirectiveType.IotEnableNetworkDirectiveType:
      return "IotEnableNetworkDirectiveType";
    case TCapability_EDirectiveType.SetAbsoluteLevelDirectiveType:
      return "SetAbsoluteLevelDirectiveType";
    case TCapability_EDirectiveType.SetRelativeLevelDirectiveType:
      return "SetRelativeLevelDirectiveType";
    case TCapability_EDirectiveType.StartMoveLevelDirectiveType:
      return "StartMoveLevelDirectiveType";
    case TCapability_EDirectiveType.StopMoveLevelDirectiveType:
      return "StopMoveLevelDirectiveType";
    case TCapability_EDirectiveType.SetColorSceneDirectiveType:
      return "SetColorSceneDirectiveType";
    case TCapability_EDirectiveType.SetTemperatureKDirectiveType:
      return "SetTemperatureKDirectiveType";
    case TCapability_EDirectiveType.WebOSLaunchAppDirectiveType:
      return "WebOSLaunchAppDirectiveType";
    case TCapability_EDirectiveType.WebOSShowGalleryDirectiveType:
      return "WebOSShowGalleryDirectiveType";
    case TCapability_EDirectiveType.SetAdjustableEqualizerBandsDirectiveType:
      return "SetAdjustableEqualizerBandsDirectiveType";
    case TCapability_EDirectiveType.SetFixedEqualizerBandsDirectiveType:
      return "SetFixedEqualizerBandsDirectiveType";
    case TCapability_EDirectiveType.DrawAnimationDirectiveType:
      return "DrawAnimationDirectiveType";
    case TCapability_EDirectiveType.EnableScreenDirectiveType:
      return "EnableScreenDirectiveType";
    case TCapability_EDirectiveType.DisableScreenDirectiveType:
      return "DisableScreenDirectiveType";
    case TCapability_EDirectiveType.StartVideoCallLoginDirectiveType:
      return "StartVideoCallLoginDirectiveType";
    case TCapability_EDirectiveType.StartVideoCallDirectiveType:
      return "StartVideoCallDirectiveType";
    case TCapability_EDirectiveType.AcceptVideoCallDirectiveType:
      return "AcceptVideoCallDirectiveType";
    case TCapability_EDirectiveType.DiscardVideoCallDirectiveType:
      return "DiscardVideoCallDirectiveType";
    case TCapability_EDirectiveType.VideoCallMuteMicDirectiveType:
      return "VideoCallMuteMicDirectiveType";
    case TCapability_EDirectiveType.VideoCallUnmuteMicDirectiveType:
      return "VideoCallUnmuteMicDirectiveType";
    case TCapability_EDirectiveType.VideoCallTurnOnVideoDirectiveType:
      return "VideoCallTurnOnVideoDirectiveType";
    case TCapability_EDirectiveType.VideoCallTurnOffVideoDirectiveType:
      return "VideoCallTurnOffVideoDirectiveType";
    case TCapability_EDirectiveType.StartRouteManagerDirectiveType:
      return "StartRouteManagerDirectiveType";
    case TCapability_EDirectiveType.StopRouteManagerDirectiveType:
      return "StopRouteManagerDirectiveType";
    case TCapability_EDirectiveType.ShowRouteManagerDirectiveType:
      return "ShowRouteManagerDirectiveType";
    case TCapability_EDirectiveType.ContinueRouteManagerDirectiveType:
      return "ContinueRouteManagerDirectiveType";
    case TCapability_EDirectiveType.OpenScreensaverDirectiveType:
      return "OpenScreensaverDirectiveType";
    case TCapability_EDirectiveType.StashViewDirectiveType:
      return "StashViewDirectiveType";
    case TCapability_EDirectiveType.UnstashViewDirectiveType:
      return "UnstashViewDirectiveType";
    case TCapability_EDirectiveType.AlarmAddDirectiveType:
      return "AlarmAddDirectiveType";
    case TCapability_EDirectiveType.AlarmRemoveDirectiveType:
      return "AlarmRemoveDirectiveType";
    case TCapability_EDirectiveType.AlarmUpdateDirectiveType:
      return "AlarmUpdateDirectiveType";
    case TCapability_EDirectiveType.AddIotScenariosDirectiveType:
      return "AddIotScenariosDirectiveType";
    case TCapability_EDirectiveType.RemoveIotScenariosDirectiveType:
      return "RemoveIotScenariosDirectiveType";
    case TCapability_EDirectiveType.SyncIotScenariosDirectiveType:
      return "SyncIotScenariosDirectiveType";
    case TCapability_EDirectiveType.VolumeMuteDirectiveType:
      return "VolumeMuteDirectiveType";
    case TCapability_EDirectiveType.VolumeUnmuteDirectiveType:
      return "VolumeUnmuteDirectiveType";
    case TCapability_EDirectiveType.BioStartSoundEnrollmentDirectiveType:
      return "BioStartSoundEnrollmentDirectiveType";
    case TCapability_EDirectiveType.LocalAudioFilePlayDirectiveType:
      return "LocalAudioFilePlayDirectiveType";
    case TCapability_EDirectiveType.LocalAudioFileStopDirectiveType:
      return "LocalAudioFileStopDirectiveType";
    case TCapability_EDirectiveType.DivUIShowViewDirectiveType:
      return "DivUIShowViewDirectiveType";
    case TCapability_EDirectiveType.DivUIPatchViewDirectiveType:
      return "DivUIPatchViewDirectiveType";
    case TCapability_EDirectiveType.DivUIHideViewDirectiveType:
      return "DivUIHideViewDirectiveType";
    case TCapability_EDirectiveType.DivUIStashViewDirectiveType:
      return "DivUIStashViewDirectiveType";
    case TCapability_EDirectiveType.DivUIUnstashViewDirectiveType:
      return "DivUIUnstashViewDirectiveType";
    default:
      return "UNKNOWN";
  }
}

export interface TCapability_TMeta {
  SupportedDirectives: TCapability_EDirectiveType[];
  SupportedEvents: TCapability_EEventType[];
  Retrievable: boolean;
  Reportable: boolean;
}

export interface TOnOffCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TOnOffCapability_TParameters;
  State?: TOnOffCapability_TState;
}

export interface TOnOffCapability_TParameters {
  Split: boolean;
}

export interface TOnOffCapability_TState {
  On: boolean;
}

/** directives */
export interface TOnOffCapability_TOnOffDirective {
  Name: string;
  On: boolean;
}

/** events */
export interface TOnOffCapability_TUpdateStateEvent {
  Capability?: TOnOffCapability;
}

export interface TIotDiscoveryCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TIotDiscoveryCapability_TParameters;
  State?: TIotDiscoveryCapability_TState;
}

export enum TIotDiscoveryCapability_TProtocol {
  Zigbee = 0,
  WiFi = 1,
  UNRECOGNIZED = -1,
}

export function tIotDiscoveryCapability_TProtocolFromJSON(
  object: any
): TIotDiscoveryCapability_TProtocol {
  switch (object) {
    case 0:
    case "Zigbee":
      return TIotDiscoveryCapability_TProtocol.Zigbee;
    case 1:
    case "WiFi":
      return TIotDiscoveryCapability_TProtocol.WiFi;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TIotDiscoveryCapability_TProtocol.UNRECOGNIZED;
  }
}

export function tIotDiscoveryCapability_TProtocolToJSON(
  object: TIotDiscoveryCapability_TProtocol
): string {
  switch (object) {
    case TIotDiscoveryCapability_TProtocol.Zigbee:
      return "Zigbee";
    case TIotDiscoveryCapability_TProtocol.WiFi:
      return "WiFi";
    default:
      return "UNKNOWN";
  }
}

export interface TIotDiscoveryCapability_TParameters {
  SupportedProtocols: TIotDiscoveryCapability_TProtocol[];
}

export interface TIotDiscoveryCapability_TState {
  IsDiscoveryInProgress: boolean;
}

/** todo: remove redundant context */
export interface TIotDiscoveryCapability_TDiscoveryContext {
  DeviceType: string;
  Source: string;
  Attempt: number;
}

export interface TIotDiscoveryCapability_TNetworks {
  /**
   * deprecated due to megamind exceptions during speechkit directives transfer.
   * sk directive payload is stored inside google.protobuf.Struct
   * this disregards correct json marshal/unmarshal of google.protobuf.BytesValue (which uses base64 out of the box)
   * and produces non-ascii bytes, crashing conversion
   */
  ZigbeeNetwork?: Uint8Array;
  /** ZigbeeNetworkBase64 holds ZigbeeNetwork bytes in standard base64 encoding, as defined in RFC 4648 */
  ZigbeeNetworkBase64?: string;
}

/** directives */
export interface TIotDiscoveryCapability_TStartDiscoveryDirective {
  Name: string;
  Protocols: TIotDiscoveryCapability_TProtocol[];
  TimeoutMs: number;
  DiscoveryContext?: TIotDiscoveryCapability_TDiscoveryContext;
  DeviceLimit: number;
}

export interface TIotDiscoveryCapability_TCancelDiscoveryDirective {
  Name: string;
}

export interface TIotDiscoveryCapability_TFinishDiscoveryDirective {
  Name: string;
  AcceptedIds: string[];
}

export interface TIotDiscoveryCapability_TStartTuyaBroadcastDirective {
  Name: string;
  SSID: string;
  Password: string;
  Token: string;
  Cipher: string;
}

export interface TIotDiscoveryCapability_TRestoreNetworksDirective {
  Name: string;
  Networks?: TIotDiscoveryCapability_TNetworks;
}

export interface TIotDiscoveryCapability_TForgetDevicesDirective {
  Name: string;
  DeviceIds: string[];
}

export interface TIotDiscoveryCapability_TDeleteNetworksDirective {
  Name: string;
  Protocols: TIotDiscoveryCapability_TProtocol[];
}

export interface TIotDiscoveryCapability_TEnableNetworkDirective {
  Name: string;
  Protocol: TIotDiscoveryCapability_TProtocol;
  Enabled: boolean;
}

export interface TVideoCallCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TVideoCallCapability_TParameters;
  State?: TVideoCallCapability_TState;
}

export interface TVideoCallCapability_TParameters {}

export interface TVideoCallCapability_TState {
  ProviderStates: TVideoCallCapability_TProviderState[];
  Incoming: TVideoCallCapability_TProviderCall[];
  Current?: TVideoCallCapability_TProviderCall;
  Outgoing?: TVideoCallCapability_TProviderCall;
}

export interface TVideoCallCapability_TProviderState {
  TelegramProviderState?:
    | TVideoCallCapability_TTelegramProviderState
    | undefined;
}

export interface TVideoCallCapability_TTelegramProviderState {
  Login?: TVideoCallCapability_TTelegramProviderState_TLogin;
  ContactSync?: TVideoCallCapability_TTelegramProviderState_TContactSyncProgress;
}

export interface TVideoCallCapability_TTelegramProviderState_TLogin {
  /** telegram user id */
  UserId: string;
  State: TVideoCallCapability_TTelegramProviderState_TLogin_EState;
  /**
   * Значение true в поле гарантирует, что успешный полный синк контактов для логина был хотя бы раз и сейчас не идет
   * При false опираться на контакты из базы нельзя
   */
  FullContactsUploadFinished: boolean;
  RecentContacts: TVideoCallCapability_TTelegramProviderState_TRecentContactData[];
}

export enum TVideoCallCapability_TTelegramProviderState_TLogin_EState {
  InProgress = 0,
  Success = 1,
  UNRECOGNIZED = -1,
}

export function tVideoCallCapability_TTelegramProviderState_TLogin_EStateFromJSON(
  object: any
): TVideoCallCapability_TTelegramProviderState_TLogin_EState {
  switch (object) {
    case 0:
    case "InProgress":
      return TVideoCallCapability_TTelegramProviderState_TLogin_EState.InProgress;
    case 1:
    case "Success":
      return TVideoCallCapability_TTelegramProviderState_TLogin_EState.Success;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TVideoCallCapability_TTelegramProviderState_TLogin_EState.UNRECOGNIZED;
  }
}

export function tVideoCallCapability_TTelegramProviderState_TLogin_EStateToJSON(
  object: TVideoCallCapability_TTelegramProviderState_TLogin_EState
): string {
  switch (object) {
    case TVideoCallCapability_TTelegramProviderState_TLogin_EState.InProgress:
      return "InProgress";
    case TVideoCallCapability_TTelegramProviderState_TLogin_EState.Success:
      return "Success";
    default:
      return "UNKNOWN";
  }
}

export interface TVideoCallCapability_TTelegramProviderState_TContactSyncProgress {}

export interface TVideoCallCapability_TTelegramProviderState_TRecentContactData {
  /** telegram user id */
  UserId: string;
}

export interface TVideoCallCapability_TTelegramVideoCallOwnerData {
  /** telegram call id */
  CallId: string;
  /** device owner telegram user id */
  UserId: string;
}

export interface TVideoCallCapability_TProviderCall {
  State: TVideoCallCapability_TProviderCall_EState;
  TelegramCallData?:
    | TVideoCallCapability_TProviderCall_TTelegramCallData
    | undefined;
}

export enum TVideoCallCapability_TProviderCall_EState {
  Ringing = 0,
  Accepted = 1,
  Established = 2,
  UNRECOGNIZED = -1,
}

export function tVideoCallCapability_TProviderCall_EStateFromJSON(
  object: any
): TVideoCallCapability_TProviderCall_EState {
  switch (object) {
    case 0:
    case "Ringing":
      return TVideoCallCapability_TProviderCall_EState.Ringing;
    case 1:
    case "Accepted":
      return TVideoCallCapability_TProviderCall_EState.Accepted;
    case 2:
    case "Established":
      return TVideoCallCapability_TProviderCall_EState.Established;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TVideoCallCapability_TProviderCall_EState.UNRECOGNIZED;
  }
}

export function tVideoCallCapability_TProviderCall_EStateToJSON(
  object: TVideoCallCapability_TProviderCall_EState
): string {
  switch (object) {
    case TVideoCallCapability_TProviderCall_EState.Ringing:
      return "Ringing";
    case TVideoCallCapability_TProviderCall_EState.Accepted:
      return "Accepted";
    case TVideoCallCapability_TProviderCall_EState.Established:
      return "Established";
    default:
      return "UNKNOWN";
  }
}

export interface TVideoCallCapability_TProviderCall_TTelegramCallData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  Recipient?: TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData;
  MicMuted: boolean;
  VideoEnabled: boolean;
}

export interface TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData {
  UserId: string;
  DisplayName: string;
}

export interface TVideoCallCapability_TStartVideoCallLoginDirective {
  Name: string;
  TelegramStartLoginData?:
    | TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData
    | undefined;
}

export interface TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData {
  /** id для связи директивы с show_view */
  Id: string;
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TStartVideoCallDirective {
  Name: string;
  TelegramStartVideoCallData?:
    | TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData
    | undefined;
}

export interface TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData {
  /** id для связи директивы с show_view */
  Id: string;
  /** telegram id аккаунта, с которого звоним */
  UserId: string;
  /** telegram id аккаунта, на который звоним */
  RecipientUserId: string;
  VideoEnabled: boolean;
  OnAcceptedCallback?: { [key: string]: any };
  OnDiscardedCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TAcceptVideoCallDirective {
  Name: string;
  TelegramAcceptVideoCallData?:
    | TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData
    | undefined;
}

export interface TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  OnSuccessCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TDiscardVideoCallDirective {
  Name: string;
  TelegramDiscardVideoCallData?:
    | TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData
    | undefined;
}

export interface TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  OnSuccessCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TMuteMicDirective {
  Name: string;
  TelegramMuteMicData?:
    | TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData
    | undefined;
}

export interface TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  OnSuccessCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TUnmuteMicDirective {
  Name: string;
  TelegramUnmuteMicData?:
    | TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData
    | undefined;
}

export interface TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  OnSuccessCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TTurnOnVideoDirective {
  Name: string;
  TelegramMuteTurnOnVideoData?:
    | TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData
    | undefined;
}

export interface TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  OnSuccessCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TVideoCallCapability_TTurnOffVideoDirective {
  Name: string;
  TelegramMuteTurnOffVideoData?:
    | TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData
    | undefined;
}

export interface TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData {
  CallOwnerData?: TVideoCallCapability_TTelegramVideoCallOwnerData;
  OnSuccessCallback?: { [key: string]: any };
  OnFailCallback?: { [key: string]: any };
}

export interface TLevelCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TLevelCapability_TParameters;
  State?: TLevelCapability_TState;
}

export enum TLevelCapability_EInstance {
  UnknownInstance = 0,
  TemperatureInstance = 1,
  HumidityInstance = 2,
  PressureInstance = 3,
  BrightnessInstance = 4,
  IlluminanceInstance = 5,
  CoverInstance = 6,
  TVOCInstance = 7,
  AmperageInstance = 8,
  VoltageInstance = 9,
  PowerInstance = 10,
  UNRECOGNIZED = -1,
}

export function tLevelCapability_EInstanceFromJSON(
  object: any
): TLevelCapability_EInstance {
  switch (object) {
    case 0:
    case "UnknownInstance":
      return TLevelCapability_EInstance.UnknownInstance;
    case 1:
    case "TemperatureInstance":
      return TLevelCapability_EInstance.TemperatureInstance;
    case 2:
    case "HumidityInstance":
      return TLevelCapability_EInstance.HumidityInstance;
    case 3:
    case "PressureInstance":
      return TLevelCapability_EInstance.PressureInstance;
    case 4:
    case "BrightnessInstance":
      return TLevelCapability_EInstance.BrightnessInstance;
    case 5:
    case "IlluminanceInstance":
      return TLevelCapability_EInstance.IlluminanceInstance;
    case 6:
    case "CoverInstance":
      return TLevelCapability_EInstance.CoverInstance;
    case 7:
    case "TVOCInstance":
      return TLevelCapability_EInstance.TVOCInstance;
    case 8:
    case "AmperageInstance":
      return TLevelCapability_EInstance.AmperageInstance;
    case 9:
    case "VoltageInstance":
      return TLevelCapability_EInstance.VoltageInstance;
    case 10:
    case "PowerInstance":
      return TLevelCapability_EInstance.PowerInstance;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TLevelCapability_EInstance.UNRECOGNIZED;
  }
}

export function tLevelCapability_EInstanceToJSON(
  object: TLevelCapability_EInstance
): string {
  switch (object) {
    case TLevelCapability_EInstance.UnknownInstance:
      return "UnknownInstance";
    case TLevelCapability_EInstance.TemperatureInstance:
      return "TemperatureInstance";
    case TLevelCapability_EInstance.HumidityInstance:
      return "HumidityInstance";
    case TLevelCapability_EInstance.PressureInstance:
      return "PressureInstance";
    case TLevelCapability_EInstance.BrightnessInstance:
      return "BrightnessInstance";
    case TLevelCapability_EInstance.IlluminanceInstance:
      return "IlluminanceInstance";
    case TLevelCapability_EInstance.CoverInstance:
      return "CoverInstance";
    case TLevelCapability_EInstance.TVOCInstance:
      return "TVOCInstance";
    case TLevelCapability_EInstance.AmperageInstance:
      return "AmperageInstance";
    case TLevelCapability_EInstance.VoltageInstance:
      return "VoltageInstance";
    case TLevelCapability_EInstance.PowerInstance:
      return "PowerInstance";
    default:
      return "UNKNOWN";
  }
}

export enum TLevelCapability_EMoveMode {
  Up = 0,
  Down = 1,
  UNRECOGNIZED = -1,
}

export function tLevelCapability_EMoveModeFromJSON(
  object: any
): TLevelCapability_EMoveMode {
  switch (object) {
    case 0:
    case "Up":
      return TLevelCapability_EMoveMode.Up;
    case 1:
    case "Down":
      return TLevelCapability_EMoveMode.Down;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TLevelCapability_EMoveMode.UNRECOGNIZED;
  }
}

export function tLevelCapability_EMoveModeToJSON(
  object: TLevelCapability_EMoveMode
): string {
  switch (object) {
    case TLevelCapability_EMoveMode.Up:
      return "Up";
    case TLevelCapability_EMoveMode.Down:
      return "Down";
    default:
      return "UNKNOWN";
  }
}

export interface TLevelCapability_TParameters {
  Instance: TLevelCapability_EInstance;
  Range?: TRange;
  Unit: EUnit;
}

export interface TLevelCapability_TState {
  Level: number;
}

/** directives */
export interface TLevelCapability_TSetAbsoluteLevelDirective {
  Name: string;
  /** TargetLevel is the new target level of state */
  TargetLevel: number;
  /**
   * TransitionTime is the desired timespan, in which the change should occur
   * TransitionTime is measured in tenth of a second
   * 0 means as fast as possible
   */
  TransitionTime: number;
}

export interface TLevelCapability_TSetRelativeLevelDirective {
  Name: string;
  /** RelativeLevel is the change that should be applied to the current level of state */
  RelativeLevel: number;
  /**
   * TransitionTime is the desired timespan, in which the change should occur
   * TransitionTime is measured in tenth of a second
   * 0 means as fast as possible
   */
  TransitionTime: number;
}

export interface TLevelCapability_TStartMoveLevelDirective {
  Name: string;
  ModeMode: TLevelCapability_EMoveMode;
  /**
   * Rate represents the desired level change rate per second
   * 0 means that default change rate should be used
   */
  Rate: number;
}

export interface TLevelCapability_TStopMoveLevelDirective {
  Name: string;
}

/** events */
export interface TLevelCapability_TUpdateStateEvent {
  Capability?: TLevelCapability;
}

export interface TLevelCapability_TCondition {
  Instance: TLevelCapability_EInstance;
  LowerBound?: number;
  UpperBound?: number;
  Hysteresis: number;
}

export interface TColorCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TColorCapability_TParameters;
  State?: TColorCapability_TState;
}

export enum TColorCapability_EColorScene {
  Inactive = 0,
  LavaLampScene = 1,
  CandleScene = 2,
  NightScene = 3,
  UNRECOGNIZED = -1,
}

export function tColorCapability_EColorSceneFromJSON(
  object: any
): TColorCapability_EColorScene {
  switch (object) {
    case 0:
    case "Inactive":
      return TColorCapability_EColorScene.Inactive;
    case 1:
    case "LavaLampScene":
      return TColorCapability_EColorScene.LavaLampScene;
    case 2:
    case "CandleScene":
      return TColorCapability_EColorScene.CandleScene;
    case 3:
    case "NightScene":
      return TColorCapability_EColorScene.NightScene;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TColorCapability_EColorScene.UNRECOGNIZED;
  }
}

export function tColorCapability_EColorSceneToJSON(
  object: TColorCapability_EColorScene
): string {
  switch (object) {
    case TColorCapability_EColorScene.Inactive:
      return "Inactive";
    case TColorCapability_EColorScene.LavaLampScene:
      return "LavaLampScene";
    case TColorCapability_EColorScene.CandleScene:
      return "CandleScene";
    case TColorCapability_EColorScene.NightScene:
      return "NightScene";
    default:
      return "UNKNOWN";
  }
}

export interface TColorCapability_TParameters {
  ColorSceneParameters?: TColorCapability_TParameters_TColorSceneParameters;
  TemperatureKParameters?: TColorCapability_TParameters_TTemperatureKParameters;
}

export interface TColorCapability_TParameters_TTemperatureKParameters {
  Range?: TPositiveIntegerRange;
}

export interface TColorCapability_TParameters_TColorSceneParameters {
  SupportedScenes: TColorCapability_EColorScene[];
}

export interface TColorCapability_TState {
  ColorScene: TColorCapability_EColorScene | undefined;
  TemperatureK: number | undefined;
}

/** directives */
export interface TColorCapability_TSetColorSceneDirective {
  Name: string;
  ColorScene: TColorCapability_EColorScene;
}

export interface TColorCapability_TSetTemperatureKDirective {
  Name: string;
  TargetValue: number;
}

/** events */
export interface TColorCapability_TUpdateStateEvent {
  Capability?: TColorCapability;
}

export interface TWebOSCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TWebOSCapability_TParameters;
  State?: TWebOSCapability_TState;
}

export interface TWebOSCapability_TState {
  ForegroundAppId: string;
}

export interface TWebOSCapability_TParameters {
  AvailableApps: TWebOSCapability_TParameters_TAppInfo[];
}

export interface TWebOSCapability_TParameters_TAppInfo {
  AppId: string;
}

/** directives */
export interface TWebOSCapability_TWebOSLaunchAppDirective {
  Name: string;
  AppId: string;
  ParamsJson: Uint8Array;
}

export interface TWebOSCapability_TWebOSShowGalleryDirective {
  Name: string;
  /** ItemsJson will be changed to Items when the structure will be finalized */
  ItemsJson: Uint8Array[];
}

export interface TButtonCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TButtonCapability_TParameters;
  State?: TButtonCapability_TState;
}

export interface TButtonCapability_TParameters {}

export interface TButtonCapability_TState {}

/** events */
export interface TButtonCapability_TButtonClickEvent {}

export interface TButtonCapability_TButtonDoubleClickEvent {}

export interface TButtonCapability_TButtonLongPressEvent {}

export interface TButtonCapability_TButtonLongReleaseEvent {}

export interface TButtonCapability_TCondition {
  Events: TCapability_EEventType[];
}

export interface TEqualizerCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TEqualizerCapability_TParameters;
  State?: TEqualizerCapability_TState;
}

export enum TEqualizerCapability_EPresetMode {
  /** Default - default device equalizer settings */
  Default = 0,
  /** User - custom user preset */
  User = 1,
  /** MediaCorrection - auto genre preset */
  MediaCorrection = 2,
  UNRECOGNIZED = -1,
}

export function tEqualizerCapability_EPresetModeFromJSON(
  object: any
): TEqualizerCapability_EPresetMode {
  switch (object) {
    case 0:
    case "Default":
      return TEqualizerCapability_EPresetMode.Default;
    case 1:
    case "User":
      return TEqualizerCapability_EPresetMode.User;
    case 2:
    case "MediaCorrection":
      return TEqualizerCapability_EPresetMode.MediaCorrection;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TEqualizerCapability_EPresetMode.UNRECOGNIZED;
  }
}

export function tEqualizerCapability_EPresetModeToJSON(
  object: TEqualizerCapability_EPresetMode
): string {
  switch (object) {
    case TEqualizerCapability_EPresetMode.Default:
      return "Default";
    case TEqualizerCapability_EPresetMode.User:
      return "User";
    case TEqualizerCapability_EPresetMode.MediaCorrection:
      return "MediaCorrection";
    default:
      return "UNKNOWN";
  }
}

export interface TEqualizerCapability_TParameters {
  BandsLimits?: TEqualizerCapability_TParameters_TBandsLimits;
  SupportedPresetModes: TEqualizerCapability_EPresetMode[];
  Fixed?: TEqualizerCapability_TParameters_TFixedBandsConfiguration | undefined;
  Adjustable?:
    | TEqualizerCapability_TParameters_TAdjustableBandsConfiguration
    | undefined;
}

/** structures */
export interface TEqualizerCapability_TParameters_TBandsLimits {
  MinBandsCount: number;
  MaxBandsCount: number;
  MinBandGain: number;
  MaxBandGain: number;
}

export interface TEqualizerCapability_TParameters_TAdjustableBandsConfiguration {}

export interface TEqualizerCapability_TParameters_TFixedBandsConfiguration {
  /**
   * Fixed num of bands with fixed frequencies and widths
   * Server can't change the amount of bands and it's freq/width via directive
   */
  FixedBands: TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand[];
}

export interface TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand {
  Frequency: number;
  Width: number;
}

export interface TEqualizerCapability_TState {
  Bands: TEqualizerCapability_TBandState[];
  PresetMode: TEqualizerCapability_EPresetMode;
}

export interface TEqualizerCapability_TBandState {
  Frequency: number;
  Gain: number;
  Width: number;
}

export interface TEqualizerCapability_TSetAdjustableEqualizerBandsDirective {
  Name: string;
  Bands: TEqualizerCapability_TBandState[];
}

export interface TEqualizerCapability_TSetFixedEqualizerBandsDirective {
  Name: string;
  Gains: number[];
}

export interface TAnimationCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TAnimationCapability_TParameters;
  State?: TAnimationCapability_TState;
}

export enum TAnimationCapability_EFormat {
  S3Url = 0,
  Binary = 1,
  UNRECOGNIZED = -1,
}

export function tAnimationCapability_EFormatFromJSON(
  object: any
): TAnimationCapability_EFormat {
  switch (object) {
    case 0:
    case "S3Url":
      return TAnimationCapability_EFormat.S3Url;
    case 1:
    case "Binary":
      return TAnimationCapability_EFormat.Binary;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnimationCapability_EFormat.UNRECOGNIZED;
  }
}

export function tAnimationCapability_EFormatToJSON(
  object: TAnimationCapability_EFormat
): string {
  switch (object) {
    case TAnimationCapability_EFormat.S3Url:
      return "S3Url";
    case TAnimationCapability_EFormat.Binary:
      return "Binary";
    default:
      return "UNKNOWN";
  }
}

export interface TAnimationCapability_TParameters {
  SupportedFormats: TAnimationCapability_EFormat[];
  Screens: TAnimationCapability_TScreen[];
}

export interface TAnimationCapability_TScreen {
  Guid: string;
  ScreenType: TAnimationCapability_TScreen_EScreenType;
}

export enum TAnimationCapability_TScreen_EScreenType {
  UnknownScreenType = 0,
  Oknix = 1,
  Panel = 2,
  LedScreen = 3,
  Ring = 4,
  UNRECOGNIZED = -1,
}

export function tAnimationCapability_TScreen_EScreenTypeFromJSON(
  object: any
): TAnimationCapability_TScreen_EScreenType {
  switch (object) {
    case 0:
    case "UnknownScreenType":
      return TAnimationCapability_TScreen_EScreenType.UnknownScreenType;
    case 1:
    case "Oknix":
      return TAnimationCapability_TScreen_EScreenType.Oknix;
    case 2:
    case "Panel":
      return TAnimationCapability_TScreen_EScreenType.Panel;
    case 3:
    case "LedScreen":
      return TAnimationCapability_TScreen_EScreenType.LedScreen;
    case 4:
    case "Ring":
      return TAnimationCapability_TScreen_EScreenType.Ring;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnimationCapability_TScreen_EScreenType.UNRECOGNIZED;
  }
}

export function tAnimationCapability_TScreen_EScreenTypeToJSON(
  object: TAnimationCapability_TScreen_EScreenType
): string {
  switch (object) {
    case TAnimationCapability_TScreen_EScreenType.UnknownScreenType:
      return "UnknownScreenType";
    case TAnimationCapability_TScreen_EScreenType.Oknix:
      return "Oknix";
    case TAnimationCapability_TScreen_EScreenType.Panel:
      return "Panel";
    case TAnimationCapability_TScreen_EScreenType.LedScreen:
      return "LedScreen";
    case TAnimationCapability_TScreen_EScreenType.Ring:
      return "Ring";
    default:
      return "UNKNOWN";
  }
}

export interface TAnimationCapability_TAnimation {
  AnimationType: TAnimationCapability_TAnimation_EAnimationType;
}

export enum TAnimationCapability_TAnimation_EAnimationType {
  UnknownAnimationType = 0,
  Nothing = 1,
  Idle = 2,
  Notification = 3,
  TimerCountdown = 4,
  UNRECOGNIZED = -1,
}

export function tAnimationCapability_TAnimation_EAnimationTypeFromJSON(
  object: any
): TAnimationCapability_TAnimation_EAnimationType {
  switch (object) {
    case 0:
    case "UnknownAnimationType":
      return TAnimationCapability_TAnimation_EAnimationType.UnknownAnimationType;
    case 1:
    case "Nothing":
      return TAnimationCapability_TAnimation_EAnimationType.Nothing;
    case 2:
    case "Idle":
      return TAnimationCapability_TAnimation_EAnimationType.Idle;
    case 3:
    case "Notification":
      return TAnimationCapability_TAnimation_EAnimationType.Notification;
    case 4:
    case "TimerCountdown":
      return TAnimationCapability_TAnimation_EAnimationType.TimerCountdown;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnimationCapability_TAnimation_EAnimationType.UNRECOGNIZED;
  }
}

export function tAnimationCapability_TAnimation_EAnimationTypeToJSON(
  object: TAnimationCapability_TAnimation_EAnimationType
): string {
  switch (object) {
    case TAnimationCapability_TAnimation_EAnimationType.UnknownAnimationType:
      return "UnknownAnimationType";
    case TAnimationCapability_TAnimation_EAnimationType.Nothing:
      return "Nothing";
    case TAnimationCapability_TAnimation_EAnimationType.Idle:
      return "Idle";
    case TAnimationCapability_TAnimation_EAnimationType.Notification:
      return "Notification";
    case TAnimationCapability_TAnimation_EAnimationType.TimerCountdown:
      return "TimerCountdown";
    default:
      return "UNKNOWN";
  }
}

export interface TAnimationCapability_TState {
  /** guid -> state */
  ScreenStatesMap: { [key: string]: TAnimationCapability_TState_TScreenState };
}

export interface TAnimationCapability_TState_TScreenState {
  Enabled: boolean;
  Animation?: TAnimationCapability_TAnimation;
}

export interface TAnimationCapability_TState_ScreenStatesMapEntry {
  key: string;
  value?: TAnimationCapability_TState_TScreenState;
}

/** directives */
export interface TAnimationCapability_TDrawAnimationDirective {
  Name: string;
  Animations: TAnimationCapability_TDrawAnimationDirective_TAnimation[];
  AnimationStopPolicy: TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy;
  SpeakingAnimationPolicy: TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy;
}

export enum TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy {
  Unknown = 0,
  PlayOnce = 1,
  PlayOnceTillEndOfTTS = 2,
  RepeatLastTillEndOfTTS = 3,
  RepeatLastTillNextDirective = 4,
  UNRECOGNIZED = -1,
}

export function tAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicyFromJSON(
  object: any
): TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy {
  switch (object) {
    case 0:
    case "Unknown":
      return TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.Unknown;
    case 1:
    case "PlayOnce":
      return TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.PlayOnce;
    case 2:
    case "PlayOnceTillEndOfTTS":
      return TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.PlayOnceTillEndOfTTS;
    case 3:
    case "RepeatLastTillEndOfTTS":
      return TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.RepeatLastTillEndOfTTS;
    case 4:
    case "RepeatLastTillNextDirective":
      return TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.RepeatLastTillNextDirective;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.UNRECOGNIZED;
  }
}

export function tAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicyToJSON(
  object: TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy
): string {
  switch (object) {
    case TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.Unknown:
      return "Unknown";
    case TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.PlayOnce:
      return "PlayOnce";
    case TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.PlayOnceTillEndOfTTS:
      return "PlayOnceTillEndOfTTS";
    case TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.RepeatLastTillEndOfTTS:
      return "RepeatLastTillEndOfTTS";
    case TAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicy.RepeatLastTillNextDirective:
      return "RepeatLastTillNextDirective";
    default:
      return "UNKNOWN";
  }
}

export enum TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy {
  /** Default - default mode */
  Default = 0,
  /** PlaySpeakingEndOfTts - Play "speaking" animation if TTS is longer than server animation */
  PlaySpeakingEndOfTts = 1,
  /** SkipSpeakingAnimation - Don't play "speaking" animation */
  SkipSpeakingAnimation = 2,
  UNRECOGNIZED = -1,
}

export function tAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicyFromJSON(
  object: any
): TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy {
  switch (object) {
    case 0:
    case "Default":
      return TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.Default;
    case 1:
    case "PlaySpeakingEndOfTts":
      return TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.PlaySpeakingEndOfTts;
    case 2:
    case "SkipSpeakingAnimation":
      return TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.SkipSpeakingAnimation;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.UNRECOGNIZED;
  }
}

export function tAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicyToJSON(
  object: TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy
): string {
  switch (object) {
    case TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.Default:
      return "Default";
    case TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.PlaySpeakingEndOfTts:
      return "PlaySpeakingEndOfTts";
    case TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy.SkipSpeakingAnimation:
      return "SkipSpeakingAnimation";
    default:
      return "UNKNOWN";
  }
}

export interface TAnimationCapability_TDrawAnimationDirective_TAnimation {
  /** Binary encoded animation */
  BinaryAnimation?:
    | TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation
    | undefined;
  /** in this case all animation data is stored on S3 */
  S3Directory?:
    | TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory
    | undefined;
}

export interface TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory {
  /** bucket + fqdn, for example https://blablabla.s3.mds.yandex.net */
  Bucket: string;
  Path: string;
}

export interface TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation {
  Compression: TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType;
  Base64EncodedValue: string;
}

/** type of Base64EncodedValue */
export enum TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType {
  Unknown = 0,
  /** None - Base64EncodedValue contains direct scled binary data with EncodeBase64 */
  None = 1,
  /** Gzip - see https://wiki.yandex-team.ru/quasar/mini2/format-ledpatternov-chasov-mini-2/#binarnyjjformatdljaledpatternov */
  Gzip = 2,
  UNRECOGNIZED = -1,
}

export function tAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionTypeFromJSON(
  object: any
): TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType {
  switch (object) {
    case 0:
    case "Unknown":
      return TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.Unknown;
    case 1:
    case "None":
      return TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.None;
    case 2:
    case "Gzip":
      return TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.Gzip;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.UNRECOGNIZED;
  }
}

export function tAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionTypeToJSON(
  object: TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType
): string {
  switch (object) {
    case TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.Unknown:
      return "Unknown";
    case TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.None:
      return "None";
    case TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionType.Gzip:
      return "Gzip";
    default:
      return "UNKNOWN";
  }
}

export interface TAnimationCapability_TEnableScreenDirective {
  Name: string;
  Guid: string;
}

export interface TAnimationCapability_TDisableScreenDirective {
  Name: string;
  Guid: string;
}

export interface TMotionCapability {
  Meta?: TCapability_TMeta;
  Parameters?: TMotionCapability_TParameters;
  State?: TMotionCapability_TState;
}

export interface TMotionCapability_TParameters {}

export interface TMotionCapability_TState {}

export interface TMotionCapability_TMotionDetectedEvent {}

export interface TMotionCapability_TCondition {
  Events: TCapability_EEventType[];
}

function createBaseTCapability(): TCapability {
  return {};
}

export const TCapability = {
  encode(_: TCapability, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCapability();
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

  fromJSON(_: any): TCapability {
    return {};
  },

  toJSON(_: TCapability): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCapability_TMeta(): TCapability_TMeta {
  return {
    SupportedDirectives: [],
    SupportedEvents: [],
    Retrievable: false,
    Reportable: false,
  };
}

export const TCapability_TMeta = {
  encode(message: TCapability_TMeta, writer: Writer = Writer.create()): Writer {
    writer.uint32(10).fork();
    for (const v of message.SupportedDirectives) {
      writer.int32(v);
    }
    writer.ldelim();
    writer.uint32(34).fork();
    for (const v of message.SupportedEvents) {
      writer.int32(v);
    }
    writer.ldelim();
    if (message.Retrievable === true) {
      writer.uint32(16).bool(message.Retrievable);
    }
    if (message.Reportable === true) {
      writer.uint32(24).bool(message.Reportable);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCapability_TMeta {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCapability_TMeta();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.SupportedDirectives.push(reader.int32() as any);
            }
          } else {
            message.SupportedDirectives.push(reader.int32() as any);
          }
          break;
        case 4:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.SupportedEvents.push(reader.int32() as any);
            }
          } else {
            message.SupportedEvents.push(reader.int32() as any);
          }
          break;
        case 2:
          message.Retrievable = reader.bool();
          break;
        case 3:
          message.Reportable = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCapability_TMeta {
    return {
      SupportedDirectives: Array.isArray(object?.supported_directives)
        ? object.supported_directives.map((e: any) =>
            tCapability_EDirectiveTypeFromJSON(e)
          )
        : [],
      SupportedEvents: Array.isArray(object?.supported_events)
        ? object.supported_events.map((e: any) =>
            tCapability_EEventTypeFromJSON(e)
          )
        : [],
      Retrievable: isSet(object.retrievable)
        ? Boolean(object.retrievable)
        : false,
      Reportable: isSet(object.reportable) ? Boolean(object.reportable) : false,
    };
  },

  toJSON(message: TCapability_TMeta): unknown {
    const obj: any = {};
    if (message.SupportedDirectives) {
      obj.supported_directives = message.SupportedDirectives.map((e) =>
        tCapability_EDirectiveTypeToJSON(e)
      );
    } else {
      obj.supported_directives = [];
    }
    if (message.SupportedEvents) {
      obj.supported_events = message.SupportedEvents.map((e) =>
        tCapability_EEventTypeToJSON(e)
      );
    } else {
      obj.supported_events = [];
    }
    message.Retrievable !== undefined &&
      (obj.retrievable = message.Retrievable);
    message.Reportable !== undefined && (obj.reportable = message.Reportable);
    return obj;
  },
};

function createBaseTOnOffCapability(): TOnOffCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TOnOffCapability = {
  encode(message: TOnOffCapability, writer: Writer = Writer.create()): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TOnOffCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TOnOffCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOnOffCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnOffCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TOnOffCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TOnOffCapability_TState.decode(
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

  fromJSON(object: any): TOnOffCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TOnOffCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TOnOffCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TOnOffCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TOnOffCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TOnOffCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTOnOffCapability_TParameters(): TOnOffCapability_TParameters {
  return { Split: false };
}

export const TOnOffCapability_TParameters = {
  encode(
    message: TOnOffCapability_TParameters,
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
  ): TOnOffCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnOffCapability_TParameters();
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

  fromJSON(object: any): TOnOffCapability_TParameters {
    return {
      Split: isSet(object.split) ? Boolean(object.split) : false,
    };
  },

  toJSON(message: TOnOffCapability_TParameters): unknown {
    const obj: any = {};
    message.Split !== undefined && (obj.split = message.Split);
    return obj;
  },
};

function createBaseTOnOffCapability_TState(): TOnOffCapability_TState {
  return { On: false };
}

export const TOnOffCapability_TState = {
  encode(
    message: TOnOffCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.On === true) {
      writer.uint32(8).bool(message.On);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOnOffCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnOffCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.On = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOnOffCapability_TState {
    return {
      On: isSet(object.on) ? Boolean(object.on) : false,
    };
  },

  toJSON(message: TOnOffCapability_TState): unknown {
    const obj: any = {};
    message.On !== undefined && (obj.on = message.On);
    return obj;
  },
};

function createBaseTOnOffCapability_TOnOffDirective(): TOnOffCapability_TOnOffDirective {
  return { Name: "", On: false };
}

export const TOnOffCapability_TOnOffDirective = {
  encode(
    message: TOnOffCapability_TOnOffDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.On === true) {
      writer.uint32(8).bool(message.On);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOnOffCapability_TOnOffDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnOffCapability_TOnOffDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.On = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOnOffCapability_TOnOffDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      On: isSet(object.on) ? Boolean(object.on) : false,
    };
  },

  toJSON(message: TOnOffCapability_TOnOffDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.On !== undefined && (obj.on = message.On);
    return obj;
  },
};

function createBaseTOnOffCapability_TUpdateStateEvent(): TOnOffCapability_TUpdateStateEvent {
  return { Capability: undefined };
}

export const TOnOffCapability_TUpdateStateEvent = {
  encode(
    message: TOnOffCapability_TUpdateStateEvent,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Capability !== undefined) {
      TOnOffCapability.encode(
        message.Capability,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOnOffCapability_TUpdateStateEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnOffCapability_TUpdateStateEvent();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Capability = TOnOffCapability.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOnOffCapability_TUpdateStateEvent {
    return {
      Capability: isSet(object.capability)
        ? TOnOffCapability.fromJSON(object.capability)
        : undefined,
    };
  },

  toJSON(message: TOnOffCapability_TUpdateStateEvent): unknown {
    const obj: any = {};
    message.Capability !== undefined &&
      (obj.capability = message.Capability
        ? TOnOffCapability.toJSON(message.Capability)
        : undefined);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability(): TIotDiscoveryCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TIotDiscoveryCapability = {
  encode(
    message: TIotDiscoveryCapability,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TIotDiscoveryCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TIotDiscoveryCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIotDiscoveryCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TIotDiscoveryCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TIotDiscoveryCapability_TState.decode(
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

  fromJSON(object: any): TIotDiscoveryCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TIotDiscoveryCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TIotDiscoveryCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TIotDiscoveryCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TIotDiscoveryCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TIotDiscoveryCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TParameters(): TIotDiscoveryCapability_TParameters {
  return { SupportedProtocols: [] };
}

export const TIotDiscoveryCapability_TParameters = {
  encode(
    message: TIotDiscoveryCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.SupportedProtocols) {
      writer.int32(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability_TParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.SupportedProtocols.push(reader.int32() as any);
            }
          } else {
            message.SupportedProtocols.push(reader.int32() as any);
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TParameters {
    return {
      SupportedProtocols: Array.isArray(object?.supported_protocols)
        ? object.supported_protocols.map((e: any) =>
            tIotDiscoveryCapability_TProtocolFromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TIotDiscoveryCapability_TParameters): unknown {
    const obj: any = {};
    if (message.SupportedProtocols) {
      obj.supported_protocols = message.SupportedProtocols.map((e) =>
        tIotDiscoveryCapability_TProtocolToJSON(e)
      );
    } else {
      obj.supported_protocols = [];
    }
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TState(): TIotDiscoveryCapability_TState {
  return { IsDiscoveryInProgress: false };
}

export const TIotDiscoveryCapability_TState = {
  encode(
    message: TIotDiscoveryCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.IsDiscoveryInProgress === true) {
      writer.uint32(8).bool(message.IsDiscoveryInProgress);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.IsDiscoveryInProgress = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TState {
    return {
      IsDiscoveryInProgress: isSet(object.is_discovery_in_progress)
        ? Boolean(object.is_discovery_in_progress)
        : false,
    };
  },

  toJSON(message: TIotDiscoveryCapability_TState): unknown {
    const obj: any = {};
    message.IsDiscoveryInProgress !== undefined &&
      (obj.is_discovery_in_progress = message.IsDiscoveryInProgress);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TDiscoveryContext(): TIotDiscoveryCapability_TDiscoveryContext {
  return { DeviceType: "", Source: "", Attempt: 0 };
}

export const TIotDiscoveryCapability_TDiscoveryContext = {
  encode(
    message: TIotDiscoveryCapability_TDiscoveryContext,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DeviceType !== "") {
      writer.uint32(10).string(message.DeviceType);
    }
    if (message.Source !== "") {
      writer.uint32(18).string(message.Source);
    }
    if (message.Attempt !== 0) {
      writer.uint32(24).uint32(message.Attempt);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TDiscoveryContext {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability_TDiscoveryContext();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceType = reader.string();
          break;
        case 2:
          message.Source = reader.string();
          break;
        case 3:
          message.Attempt = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TDiscoveryContext {
    return {
      DeviceType: isSet(object.device_type) ? String(object.device_type) : "",
      Source: isSet(object.source) ? String(object.source) : "",
      Attempt: isSet(object.attempt) ? Number(object.attempt) : 0,
    };
  },

  toJSON(message: TIotDiscoveryCapability_TDiscoveryContext): unknown {
    const obj: any = {};
    message.DeviceType !== undefined && (obj.device_type = message.DeviceType);
    message.Source !== undefined && (obj.source = message.Source);
    message.Attempt !== undefined &&
      (obj.attempt = Math.round(message.Attempt));
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TNetworks(): TIotDiscoveryCapability_TNetworks {
  return { ZigbeeNetwork: undefined, ZigbeeNetworkBase64: undefined };
}

export const TIotDiscoveryCapability_TNetworks = {
  encode(
    message: TIotDiscoveryCapability_TNetworks,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ZigbeeNetwork !== undefined) {
      BytesValue.encode(
        { value: message.ZigbeeNetwork! },
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.ZigbeeNetworkBase64 !== undefined) {
      StringValue.encode(
        { value: message.ZigbeeNetworkBase64! },
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TNetworks {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability_TNetworks();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ZigbeeNetwork = BytesValue.decode(
            reader,
            reader.uint32()
          ).value;
          break;
        case 2:
          message.ZigbeeNetworkBase64 = StringValue.decode(
            reader,
            reader.uint32()
          ).value;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TNetworks {
    return {
      ZigbeeNetwork: isSet(object.zigbee_network)
        ? new Uint8Array(object.zigbee_network)
        : undefined,
      ZigbeeNetworkBase64: isSet(object.zigbee_network_base64)
        ? String(object.zigbee_network_base64)
        : undefined,
    };
  },

  toJSON(message: TIotDiscoveryCapability_TNetworks): unknown {
    const obj: any = {};
    message.ZigbeeNetwork !== undefined &&
      (obj.zigbee_network = message.ZigbeeNetwork);
    message.ZigbeeNetworkBase64 !== undefined &&
      (obj.zigbee_network_base64 = message.ZigbeeNetworkBase64);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TStartDiscoveryDirective(): TIotDiscoveryCapability_TStartDiscoveryDirective {
  return {
    Name: "",
    Protocols: [],
    TimeoutMs: 0,
    DiscoveryContext: undefined,
    DeviceLimit: 0,
  };
}

export const TIotDiscoveryCapability_TStartDiscoveryDirective = {
  encode(
    message: TIotDiscoveryCapability_TStartDiscoveryDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    writer.uint32(10).fork();
    for (const v of message.Protocols) {
      writer.int32(v);
    }
    writer.ldelim();
    if (message.TimeoutMs !== 0) {
      writer.uint32(16).uint32(message.TimeoutMs);
    }
    if (message.DiscoveryContext !== undefined) {
      TIotDiscoveryCapability_TDiscoveryContext.encode(
        message.DiscoveryContext,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.DeviceLimit !== 0) {
      writer.uint32(32).uint32(message.DeviceLimit);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TStartDiscoveryDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIotDiscoveryCapability_TStartDiscoveryDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
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
          message.TimeoutMs = reader.uint32();
          break;
        case 3:
          message.DiscoveryContext =
            TIotDiscoveryCapability_TDiscoveryContext.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.DeviceLimit = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TStartDiscoveryDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Protocols: Array.isArray(object?.protocols)
        ? object.protocols.map((e: any) =>
            tIotDiscoveryCapability_TProtocolFromJSON(e)
          )
        : [],
      TimeoutMs: isSet(object.timeout_ms) ? Number(object.timeout_ms) : 0,
      DiscoveryContext: isSet(object.discovery_context)
        ? TIotDiscoveryCapability_TDiscoveryContext.fromJSON(
            object.discovery_context
          )
        : undefined,
      DeviceLimit: isSet(object.device_limit) ? Number(object.device_limit) : 0,
    };
  },

  toJSON(message: TIotDiscoveryCapability_TStartDiscoveryDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Protocols) {
      obj.protocols = message.Protocols.map((e) =>
        tIotDiscoveryCapability_TProtocolToJSON(e)
      );
    } else {
      obj.protocols = [];
    }
    message.TimeoutMs !== undefined &&
      (obj.timeout_ms = Math.round(message.TimeoutMs));
    message.DiscoveryContext !== undefined &&
      (obj.discovery_context = message.DiscoveryContext
        ? TIotDiscoveryCapability_TDiscoveryContext.toJSON(
            message.DiscoveryContext
          )
        : undefined);
    message.DeviceLimit !== undefined &&
      (obj.device_limit = Math.round(message.DeviceLimit));
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TCancelDiscoveryDirective(): TIotDiscoveryCapability_TCancelDiscoveryDirective {
  return { Name: "" };
}

export const TIotDiscoveryCapability_TCancelDiscoveryDirective = {
  encode(
    message: TIotDiscoveryCapability_TCancelDiscoveryDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TCancelDiscoveryDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIotDiscoveryCapability_TCancelDiscoveryDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TCancelDiscoveryDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
    };
  },

  toJSON(message: TIotDiscoveryCapability_TCancelDiscoveryDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TFinishDiscoveryDirective(): TIotDiscoveryCapability_TFinishDiscoveryDirective {
  return { Name: "", AcceptedIds: [] };
}

export const TIotDiscoveryCapability_TFinishDiscoveryDirective = {
  encode(
    message: TIotDiscoveryCapability_TFinishDiscoveryDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    for (const v of message.AcceptedIds) {
      writer.uint32(10).string(v!);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TFinishDiscoveryDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIotDiscoveryCapability_TFinishDiscoveryDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.AcceptedIds.push(reader.string());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TFinishDiscoveryDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      AcceptedIds: Array.isArray(object?.accepted_ids)
        ? object.accepted_ids.map((e: any) => String(e))
        : [],
    };
  },

  toJSON(message: TIotDiscoveryCapability_TFinishDiscoveryDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.AcceptedIds) {
      obj.accepted_ids = message.AcceptedIds.map((e) => e);
    } else {
      obj.accepted_ids = [];
    }
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TStartTuyaBroadcastDirective(): TIotDiscoveryCapability_TStartTuyaBroadcastDirective {
  return { Name: "", SSID: "", Password: "", Token: "", Cipher: "" };
}

export const TIotDiscoveryCapability_TStartTuyaBroadcastDirective = {
  encode(
    message: TIotDiscoveryCapability_TStartTuyaBroadcastDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.SSID !== "") {
      writer.uint32(10).string(message.SSID);
    }
    if (message.Password !== "") {
      writer.uint32(18).string(message.Password);
    }
    if (message.Token !== "") {
      writer.uint32(26).string(message.Token);
    }
    if (message.Cipher !== "") {
      writer.uint32(34).string(message.Cipher);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TStartTuyaBroadcastDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIotDiscoveryCapability_TStartTuyaBroadcastDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.SSID = reader.string();
          break;
        case 2:
          message.Password = reader.string();
          break;
        case 3:
          message.Token = reader.string();
          break;
        case 4:
          message.Cipher = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TStartTuyaBroadcastDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      SSID: isSet(object.ssid) ? String(object.ssid) : "",
      Password: isSet(object.password) ? String(object.password) : "",
      Token: isSet(object.token) ? String(object.token) : "",
      Cipher: isSet(object.cipher) ? String(object.cipher) : "",
    };
  },

  toJSON(
    message: TIotDiscoveryCapability_TStartTuyaBroadcastDirective
  ): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.SSID !== undefined && (obj.ssid = message.SSID);
    message.Password !== undefined && (obj.password = message.Password);
    message.Token !== undefined && (obj.token = message.Token);
    message.Cipher !== undefined && (obj.cipher = message.Cipher);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TRestoreNetworksDirective(): TIotDiscoveryCapability_TRestoreNetworksDirective {
  return { Name: "", Networks: undefined };
}

export const TIotDiscoveryCapability_TRestoreNetworksDirective = {
  encode(
    message: TIotDiscoveryCapability_TRestoreNetworksDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
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
  ): TIotDiscoveryCapability_TRestoreNetworksDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIotDiscoveryCapability_TRestoreNetworksDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
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

  fromJSON(object: any): TIotDiscoveryCapability_TRestoreNetworksDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Networks: isSet(object.networks)
        ? TIotDiscoveryCapability_TNetworks.fromJSON(object.networks)
        : undefined,
    };
  },

  toJSON(message: TIotDiscoveryCapability_TRestoreNetworksDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Networks !== undefined &&
      (obj.networks = message.Networks
        ? TIotDiscoveryCapability_TNetworks.toJSON(message.Networks)
        : undefined);
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TForgetDevicesDirective(): TIotDiscoveryCapability_TForgetDevicesDirective {
  return { Name: "", DeviceIds: [] };
}

export const TIotDiscoveryCapability_TForgetDevicesDirective = {
  encode(
    message: TIotDiscoveryCapability_TForgetDevicesDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    for (const v of message.DeviceIds) {
      writer.uint32(10).string(v!);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TForgetDevicesDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability_TForgetDevicesDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.DeviceIds.push(reader.string());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TForgetDevicesDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      DeviceIds: Array.isArray(object?.device_ids)
        ? object.device_ids.map((e: any) => String(e))
        : [],
    };
  },

  toJSON(message: TIotDiscoveryCapability_TForgetDevicesDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.DeviceIds) {
      obj.device_ids = message.DeviceIds.map((e) => e);
    } else {
      obj.device_ids = [];
    }
    return obj;
  },
};

function createBaseTIotDiscoveryCapability_TDeleteNetworksDirective(): TIotDiscoveryCapability_TDeleteNetworksDirective {
  return { Name: "", Protocols: [] };
}

export const TIotDiscoveryCapability_TDeleteNetworksDirective = {
  encode(
    message: TIotDiscoveryCapability_TDeleteNetworksDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
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
  ): TIotDiscoveryCapability_TDeleteNetworksDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTIotDiscoveryCapability_TDeleteNetworksDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
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

  fromJSON(object: any): TIotDiscoveryCapability_TDeleteNetworksDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Protocols: Array.isArray(object?.protocols)
        ? object.protocols.map((e: any) =>
            tIotDiscoveryCapability_TProtocolFromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TIotDiscoveryCapability_TDeleteNetworksDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
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

function createBaseTIotDiscoveryCapability_TEnableNetworkDirective(): TIotDiscoveryCapability_TEnableNetworkDirective {
  return { Name: "", Protocol: 0, Enabled: false };
}

export const TIotDiscoveryCapability_TEnableNetworkDirective = {
  encode(
    message: TIotDiscoveryCapability_TEnableNetworkDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.Protocol !== 0) {
      writer.uint32(8).int32(message.Protocol);
    }
    if (message.Enabled === true) {
      writer.uint32(16).bool(message.Enabled);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotDiscoveryCapability_TEnableNetworkDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotDiscoveryCapability_TEnableNetworkDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.Protocol = reader.int32() as any;
          break;
        case 2:
          message.Enabled = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIotDiscoveryCapability_TEnableNetworkDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Protocol: isSet(object.protocol)
        ? tIotDiscoveryCapability_TProtocolFromJSON(object.protocol)
        : 0,
      Enabled: isSet(object.enabled) ? Boolean(object.enabled) : false,
    };
  },

  toJSON(message: TIotDiscoveryCapability_TEnableNetworkDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Protocol !== undefined &&
      (obj.protocol = tIotDiscoveryCapability_TProtocolToJSON(
        message.Protocol
      ));
    message.Enabled !== undefined && (obj.enabled = message.Enabled);
    return obj;
  },
};

function createBaseTVideoCallCapability(): TVideoCallCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TVideoCallCapability = {
  encode(
    message: TVideoCallCapability,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TVideoCallCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TVideoCallCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TVideoCallCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TVideoCallCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TVideoCallCapability_TState.decode(
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

  fromJSON(object: any): TVideoCallCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TVideoCallCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TVideoCallCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TVideoCallCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TVideoCallCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TParameters(): TVideoCallCapability_TParameters {
  return {};
}

export const TVideoCallCapability_TParameters = {
  encode(
    _: TVideoCallCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TParameters();
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

  fromJSON(_: any): TVideoCallCapability_TParameters {
    return {};
  },

  toJSON(_: TVideoCallCapability_TParameters): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTVideoCallCapability_TState(): TVideoCallCapability_TState {
  return {
    ProviderStates: [],
    Incoming: [],
    Current: undefined,
    Outgoing: undefined,
  };
}

export const TVideoCallCapability_TState = {
  encode(
    message: TVideoCallCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.ProviderStates) {
      TVideoCallCapability_TProviderState.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    for (const v of message.Incoming) {
      TVideoCallCapability_TProviderCall.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Current !== undefined) {
      TVideoCallCapability_TProviderCall.encode(
        message.Current,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Outgoing !== undefined) {
      TVideoCallCapability_TProviderCall.encode(
        message.Outgoing,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ProviderStates.push(
            TVideoCallCapability_TProviderState.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.Incoming.push(
            TVideoCallCapability_TProviderCall.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.Current = TVideoCallCapability_TProviderCall.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.Outgoing = TVideoCallCapability_TProviderCall.decode(
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

  fromJSON(object: any): TVideoCallCapability_TState {
    return {
      ProviderStates: Array.isArray(object?.provider_states)
        ? object.provider_states.map((e: any) =>
            TVideoCallCapability_TProviderState.fromJSON(e)
          )
        : [],
      Incoming: Array.isArray(object?.incoming)
        ? object.incoming.map((e: any) =>
            TVideoCallCapability_TProviderCall.fromJSON(e)
          )
        : [],
      Current: isSet(object.current)
        ? TVideoCallCapability_TProviderCall.fromJSON(object.current)
        : undefined,
      Outgoing: isSet(object.outgoing)
        ? TVideoCallCapability_TProviderCall.fromJSON(object.outgoing)
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TState): unknown {
    const obj: any = {};
    if (message.ProviderStates) {
      obj.provider_states = message.ProviderStates.map((e) =>
        e ? TVideoCallCapability_TProviderState.toJSON(e) : undefined
      );
    } else {
      obj.provider_states = [];
    }
    if (message.Incoming) {
      obj.incoming = message.Incoming.map((e) =>
        e ? TVideoCallCapability_TProviderCall.toJSON(e) : undefined
      );
    } else {
      obj.incoming = [];
    }
    message.Current !== undefined &&
      (obj.current = message.Current
        ? TVideoCallCapability_TProviderCall.toJSON(message.Current)
        : undefined);
    message.Outgoing !== undefined &&
      (obj.outgoing = message.Outgoing
        ? TVideoCallCapability_TProviderCall.toJSON(message.Outgoing)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TProviderState(): TVideoCallCapability_TProviderState {
  return { TelegramProviderState: undefined };
}

export const TVideoCallCapability_TProviderState = {
  encode(
    message: TVideoCallCapability_TProviderState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TelegramProviderState !== undefined) {
      TVideoCallCapability_TTelegramProviderState.encode(
        message.TelegramProviderState,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TProviderState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TProviderState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TelegramProviderState =
            TVideoCallCapability_TTelegramProviderState.decode(
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

  fromJSON(object: any): TVideoCallCapability_TProviderState {
    return {
      TelegramProviderState: isSet(object.telegram_provider_state)
        ? TVideoCallCapability_TTelegramProviderState.fromJSON(
            object.telegram_provider_state
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TProviderState): unknown {
    const obj: any = {};
    message.TelegramProviderState !== undefined &&
      (obj.telegram_provider_state = message.TelegramProviderState
        ? TVideoCallCapability_TTelegramProviderState.toJSON(
            message.TelegramProviderState
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TTelegramProviderState(): TVideoCallCapability_TTelegramProviderState {
  return { Login: undefined, ContactSync: undefined };
}

export const TVideoCallCapability_TTelegramProviderState = {
  encode(
    message: TVideoCallCapability_TTelegramProviderState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Login !== undefined) {
      TVideoCallCapability_TTelegramProviderState_TLogin.encode(
        message.Login,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.ContactSync !== undefined) {
      TVideoCallCapability_TTelegramProviderState_TContactSyncProgress.encode(
        message.ContactSync,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TTelegramProviderState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TTelegramProviderState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Login =
            TVideoCallCapability_TTelegramProviderState_TLogin.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.ContactSync =
            TVideoCallCapability_TTelegramProviderState_TContactSyncProgress.decode(
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

  fromJSON(object: any): TVideoCallCapability_TTelegramProviderState {
    return {
      Login: isSet(object.login)
        ? TVideoCallCapability_TTelegramProviderState_TLogin.fromJSON(
            object.login
          )
        : undefined,
      ContactSync: isSet(object.contact_sync)
        ? TVideoCallCapability_TTelegramProviderState_TContactSyncProgress.fromJSON(
            object.contact_sync
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TTelegramProviderState): unknown {
    const obj: any = {};
    message.Login !== undefined &&
      (obj.login = message.Login
        ? TVideoCallCapability_TTelegramProviderState_TLogin.toJSON(
            message.Login
          )
        : undefined);
    message.ContactSync !== undefined &&
      (obj.contact_sync = message.ContactSync
        ? TVideoCallCapability_TTelegramProviderState_TContactSyncProgress.toJSON(
            message.ContactSync
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TTelegramProviderState_TLogin(): TVideoCallCapability_TTelegramProviderState_TLogin {
  return {
    UserId: "",
    State: 0,
    FullContactsUploadFinished: false,
    RecentContacts: [],
  };
}

export const TVideoCallCapability_TTelegramProviderState_TLogin = {
  encode(
    message: TVideoCallCapability_TTelegramProviderState_TLogin,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== "") {
      writer.uint32(10).string(message.UserId);
    }
    if (message.State !== 0) {
      writer.uint32(16).int32(message.State);
    }
    if (message.FullContactsUploadFinished === true) {
      writer.uint32(24).bool(message.FullContactsUploadFinished);
    }
    for (const v of message.RecentContacts) {
      TVideoCallCapability_TTelegramProviderState_TRecentContactData.encode(
        v!,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TTelegramProviderState_TLogin {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TTelegramProviderState_TLogin();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = reader.string();
          break;
        case 2:
          message.State = reader.int32() as any;
          break;
        case 3:
          message.FullContactsUploadFinished = reader.bool();
          break;
        case 4:
          message.RecentContacts.push(
            TVideoCallCapability_TTelegramProviderState_TRecentContactData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TTelegramProviderState_TLogin {
    return {
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
      State: isSet(object.state)
        ? tVideoCallCapability_TTelegramProviderState_TLogin_EStateFromJSON(
            object.state
          )
        : 0,
      FullContactsUploadFinished: isSet(object.full_contacts_upload_finished)
        ? Boolean(object.full_contacts_upload_finished)
        : false,
      RecentContacts: Array.isArray(object?.recent_contacts)
        ? object.recent_contacts.map((e: any) =>
            TVideoCallCapability_TTelegramProviderState_TRecentContactData.fromJSON(
              e
            )
          )
        : [],
    };
  },

  toJSON(message: TVideoCallCapability_TTelegramProviderState_TLogin): unknown {
    const obj: any = {};
    message.UserId !== undefined && (obj.user_id = message.UserId);
    message.State !== undefined &&
      (obj.state =
        tVideoCallCapability_TTelegramProviderState_TLogin_EStateToJSON(
          message.State
        ));
    message.FullContactsUploadFinished !== undefined &&
      (obj.full_contacts_upload_finished = message.FullContactsUploadFinished);
    if (message.RecentContacts) {
      obj.recent_contacts = message.RecentContacts.map((e) =>
        e
          ? TVideoCallCapability_TTelegramProviderState_TRecentContactData.toJSON(
              e
            )
          : undefined
      );
    } else {
      obj.recent_contacts = [];
    }
    return obj;
  },
};

function createBaseTVideoCallCapability_TTelegramProviderState_TContactSyncProgress(): TVideoCallCapability_TTelegramProviderState_TContactSyncProgress {
  return {};
}

export const TVideoCallCapability_TTelegramProviderState_TContactSyncProgress =
  {
    encode(
      _: TVideoCallCapability_TTelegramProviderState_TContactSyncProgress,
      writer: Writer = Writer.create()
    ): Writer {
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TTelegramProviderState_TContactSyncProgress {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TTelegramProviderState_TContactSyncProgress();
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
    ): TVideoCallCapability_TTelegramProviderState_TContactSyncProgress {
      return {};
    },

    toJSON(
      _: TVideoCallCapability_TTelegramProviderState_TContactSyncProgress
    ): unknown {
      const obj: any = {};
      return obj;
    },
  };

function createBaseTVideoCallCapability_TTelegramProviderState_TRecentContactData(): TVideoCallCapability_TTelegramProviderState_TRecentContactData {
  return { UserId: "" };
}

export const TVideoCallCapability_TTelegramProviderState_TRecentContactData = {
  encode(
    message: TVideoCallCapability_TTelegramProviderState_TRecentContactData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== "") {
      writer.uint32(10).string(message.UserId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TTelegramProviderState_TRecentContactData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TTelegramProviderState_TRecentContactData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = reader.string();
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
  ): TVideoCallCapability_TTelegramProviderState_TRecentContactData {
    return {
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
    };
  },

  toJSON(
    message: TVideoCallCapability_TTelegramProviderState_TRecentContactData
  ): unknown {
    const obj: any = {};
    message.UserId !== undefined && (obj.user_id = message.UserId);
    return obj;
  },
};

function createBaseTVideoCallCapability_TTelegramVideoCallOwnerData(): TVideoCallCapability_TTelegramVideoCallOwnerData {
  return { CallId: "", UserId: "" };
}

export const TVideoCallCapability_TTelegramVideoCallOwnerData = {
  encode(
    message: TVideoCallCapability_TTelegramVideoCallOwnerData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CallId !== "") {
      writer.uint32(10).string(message.CallId);
    }
    if (message.UserId !== "") {
      writer.uint32(18).string(message.UserId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TTelegramVideoCallOwnerData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TTelegramVideoCallOwnerData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CallId = reader.string();
          break;
        case 2:
          message.UserId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallCapability_TTelegramVideoCallOwnerData {
    return {
      CallId: isSet(object.call_id) ? String(object.call_id) : "",
      UserId: isSet(object.user_id) ? String(object.user_id) : "",
    };
  },

  toJSON(message: TVideoCallCapability_TTelegramVideoCallOwnerData): unknown {
    const obj: any = {};
    message.CallId !== undefined && (obj.call_id = message.CallId);
    message.UserId !== undefined && (obj.user_id = message.UserId);
    return obj;
  },
};

function createBaseTVideoCallCapability_TProviderCall(): TVideoCallCapability_TProviderCall {
  return { State: 0, TelegramCallData: undefined };
}

export const TVideoCallCapability_TProviderCall = {
  encode(
    message: TVideoCallCapability_TProviderCall,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.State !== 0) {
      writer.uint32(8).int32(message.State);
    }
    if (message.TelegramCallData !== undefined) {
      TVideoCallCapability_TProviderCall_TTelegramCallData.encode(
        message.TelegramCallData,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TProviderCall {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TProviderCall();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.State = reader.int32() as any;
          break;
        case 2:
          message.TelegramCallData =
            TVideoCallCapability_TProviderCall_TTelegramCallData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TProviderCall {
    return {
      State: isSet(object.state)
        ? tVideoCallCapability_TProviderCall_EStateFromJSON(object.state)
        : 0,
      TelegramCallData: isSet(object.telegram_call_data)
        ? TVideoCallCapability_TProviderCall_TTelegramCallData.fromJSON(
            object.telegram_call_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TProviderCall): unknown {
    const obj: any = {};
    message.State !== undefined &&
      (obj.state = tVideoCallCapability_TProviderCall_EStateToJSON(
        message.State
      ));
    message.TelegramCallData !== undefined &&
      (obj.telegram_call_data = message.TelegramCallData
        ? TVideoCallCapability_TProviderCall_TTelegramCallData.toJSON(
            message.TelegramCallData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TProviderCall_TTelegramCallData(): TVideoCallCapability_TProviderCall_TTelegramCallData {
  return {
    CallOwnerData: undefined,
    Recipient: undefined,
    MicMuted: false,
    VideoEnabled: false,
  };
}

export const TVideoCallCapability_TProviderCall_TTelegramCallData = {
  encode(
    message: TVideoCallCapability_TProviderCall_TTelegramCallData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CallOwnerData !== undefined) {
      TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
        message.CallOwnerData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Recipient !== undefined) {
      TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData.encode(
        message.Recipient,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.MicMuted === true) {
      writer.uint32(24).bool(message.MicMuted);
    }
    if (message.VideoEnabled === true) {
      writer.uint32(32).bool(message.VideoEnabled);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TProviderCall_TTelegramCallData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TProviderCall_TTelegramCallData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CallOwnerData =
            TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.Recipient =
            TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 3:
          message.MicMuted = reader.bool();
          break;
        case 4:
          message.VideoEnabled = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallCapability_TProviderCall_TTelegramCallData {
    return {
      CallOwnerData: isSet(object.call_owner_data)
        ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
            object.call_owner_data
          )
        : undefined,
      Recipient: isSet(object.recipient)
        ? TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData.fromJSON(
            object.recipient
          )
        : undefined,
      MicMuted: isSet(object.mic_muted) ? Boolean(object.mic_muted) : false,
      VideoEnabled: isSet(object.video_enabled)
        ? Boolean(object.video_enabled)
        : false,
    };
  },

  toJSON(
    message: TVideoCallCapability_TProviderCall_TTelegramCallData
  ): unknown {
    const obj: any = {};
    message.CallOwnerData !== undefined &&
      (obj.call_owner_data = message.CallOwnerData
        ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
            message.CallOwnerData
          )
        : undefined);
    message.Recipient !== undefined &&
      (obj.recipient = message.Recipient
        ? TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData.toJSON(
            message.Recipient
          )
        : undefined);
    message.MicMuted !== undefined && (obj.mic_muted = message.MicMuted);
    message.VideoEnabled !== undefined &&
      (obj.video_enabled = message.VideoEnabled);
    return obj;
  },
};

function createBaseTVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData(): TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData {
  return { UserId: "", DisplayName: "" };
}

export const TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData =
  {
    encode(
      message: TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.UserId !== "") {
        writer.uint32(10).string(message.UserId);
      }
      if (message.DisplayName !== "") {
        writer.uint32(18).string(message.DisplayName);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.UserId = reader.string();
            break;
          case 2:
            message.DisplayName = reader.string();
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
    ): TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData {
      return {
        UserId: isSet(object.user_id) ? String(object.user_id) : "",
        DisplayName: isSet(object.display_name)
          ? String(object.display_name)
          : "",
      };
    },

    toJSON(
      message: TVideoCallCapability_TProviderCall_TTelegramCallData_TRecipientData
    ): unknown {
      const obj: any = {};
      message.UserId !== undefined && (obj.user_id = message.UserId);
      message.DisplayName !== undefined &&
        (obj.display_name = message.DisplayName);
      return obj;
    },
  };

function createBaseTVideoCallCapability_TStartVideoCallLoginDirective(): TVideoCallCapability_TStartVideoCallLoginDirective {
  return { Name: "", TelegramStartLoginData: undefined };
}

export const TVideoCallCapability_TStartVideoCallLoginDirective = {
  encode(
    message: TVideoCallCapability_TStartVideoCallLoginDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramStartLoginData !== undefined) {
      TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData.encode(
        message.TelegramStartLoginData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TStartVideoCallLoginDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TStartVideoCallLoginDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramStartLoginData =
            TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TStartVideoCallLoginDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramStartLoginData: isSet(object.telegram_start_login_data)
        ? TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData.fromJSON(
            object.telegram_start_login_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TStartVideoCallLoginDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramStartLoginData !== undefined &&
      (obj.telegram_start_login_data = message.TelegramStartLoginData
        ? TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData.toJSON(
            message.TelegramStartLoginData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData(): TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData {
  return { Id: "", OnFailCallback: undefined };
}

export const TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData =
  {
    encode(
      message: TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Id !== "") {
        writer.uint32(10).string(message.Id);
      }
      if (message.OnFailCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnFailCallback),
          writer.uint32(18).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Id = reader.string();
            break;
          case 2:
            message.OnFailCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
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
    ): TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData {
      return {
        Id: isSet(object.id) ? String(object.id) : "",
        OnFailCallback: isObject(object.on_fail_callback)
          ? object.on_fail_callback
          : undefined,
      };
    },

    toJSON(
      message: TVideoCallCapability_TStartVideoCallLoginDirective_TTelegramStartLoginData
    ): unknown {
      const obj: any = {};
      message.Id !== undefined && (obj.id = message.Id);
      message.OnFailCallback !== undefined &&
        (obj.on_fail_callback = message.OnFailCallback);
      return obj;
    },
  };

function createBaseTVideoCallCapability_TStartVideoCallDirective(): TVideoCallCapability_TStartVideoCallDirective {
  return { Name: "", TelegramStartVideoCallData: undefined };
}

export const TVideoCallCapability_TStartVideoCallDirective = {
  encode(
    message: TVideoCallCapability_TStartVideoCallDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramStartVideoCallData !== undefined) {
      TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData.encode(
        message.TelegramStartVideoCallData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TStartVideoCallDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TStartVideoCallDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramStartVideoCallData =
            TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TStartVideoCallDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramStartVideoCallData: isSet(object.telegram_start_video_call_data)
        ? TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData.fromJSON(
            object.telegram_start_video_call_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TStartVideoCallDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramStartVideoCallData !== undefined &&
      (obj.telegram_start_video_call_data = message.TelegramStartVideoCallData
        ? TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData.toJSON(
            message.TelegramStartVideoCallData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData(): TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData {
  return {
    Id: "",
    UserId: "",
    RecipientUserId: "",
    VideoEnabled: false,
    OnAcceptedCallback: undefined,
    OnDiscardedCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData =
  {
    encode(
      message: TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Id !== "") {
        writer.uint32(10).string(message.Id);
      }
      if (message.UserId !== "") {
        writer.uint32(18).string(message.UserId);
      }
      if (message.RecipientUserId !== "") {
        writer.uint32(26).string(message.RecipientUserId);
      }
      if (message.VideoEnabled === true) {
        writer.uint32(56).bool(message.VideoEnabled);
      }
      if (message.OnAcceptedCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnAcceptedCallback),
          writer.uint32(34).fork()
        ).ldelim();
      }
      if (message.OnDiscardedCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnDiscardedCallback),
          writer.uint32(42).fork()
        ).ldelim();
      }
      if (message.OnFailCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnFailCallback),
          writer.uint32(50).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Id = reader.string();
            break;
          case 2:
            message.UserId = reader.string();
            break;
          case 3:
            message.RecipientUserId = reader.string();
            break;
          case 7:
            message.VideoEnabled = reader.bool();
            break;
          case 4:
            message.OnAcceptedCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
            );
            break;
          case 5:
            message.OnDiscardedCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
            );
            break;
          case 6:
            message.OnFailCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
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
    ): TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData {
      return {
        Id: isSet(object.id) ? String(object.id) : "",
        UserId: isSet(object.user_id) ? String(object.user_id) : "",
        RecipientUserId: isSet(object.recipient_user_id)
          ? String(object.recipient_user_id)
          : "",
        VideoEnabled: isSet(object.video_enabled)
          ? Boolean(object.video_enabled)
          : false,
        OnAcceptedCallback: isObject(object.on_accepted_callback)
          ? object.on_accepted_callback
          : undefined,
        OnDiscardedCallback: isObject(object.on_discarded_callback)
          ? object.on_discarded_callback
          : undefined,
        OnFailCallback: isObject(object.on_fail_callback)
          ? object.on_fail_callback
          : undefined,
      };
    },

    toJSON(
      message: TVideoCallCapability_TStartVideoCallDirective_TTelegramStartVideoCallData
    ): unknown {
      const obj: any = {};
      message.Id !== undefined && (obj.id = message.Id);
      message.UserId !== undefined && (obj.user_id = message.UserId);
      message.RecipientUserId !== undefined &&
        (obj.recipient_user_id = message.RecipientUserId);
      message.VideoEnabled !== undefined &&
        (obj.video_enabled = message.VideoEnabled);
      message.OnAcceptedCallback !== undefined &&
        (obj.on_accepted_callback = message.OnAcceptedCallback);
      message.OnDiscardedCallback !== undefined &&
        (obj.on_discarded_callback = message.OnDiscardedCallback);
      message.OnFailCallback !== undefined &&
        (obj.on_fail_callback = message.OnFailCallback);
      return obj;
    },
  };

function createBaseTVideoCallCapability_TAcceptVideoCallDirective(): TVideoCallCapability_TAcceptVideoCallDirective {
  return { Name: "", TelegramAcceptVideoCallData: undefined };
}

export const TVideoCallCapability_TAcceptVideoCallDirective = {
  encode(
    message: TVideoCallCapability_TAcceptVideoCallDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramAcceptVideoCallData !== undefined) {
      TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData.encode(
        message.TelegramAcceptVideoCallData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TAcceptVideoCallDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TAcceptVideoCallDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramAcceptVideoCallData =
            TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TAcceptVideoCallDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramAcceptVideoCallData: isSet(object.telegram_accept_video_call_data)
        ? TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData.fromJSON(
            object.telegram_accept_video_call_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TAcceptVideoCallDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramAcceptVideoCallData !== undefined &&
      (obj.telegram_accept_video_call_data = message.TelegramAcceptVideoCallData
        ? TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData.toJSON(
            message.TelegramAcceptVideoCallData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData(): TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData {
  return {
    CallOwnerData: undefined,
    OnSuccessCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData =
  {
    encode(
      message: TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.CallOwnerData !== undefined) {
        TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
          message.CallOwnerData,
          writer.uint32(10).fork()
        ).ldelim();
      }
      if (message.OnSuccessCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnSuccessCallback),
          writer.uint32(18).fork()
        ).ldelim();
      }
      if (message.OnFailCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnFailCallback),
          writer.uint32(26).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.CallOwnerData =
              TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
                reader,
                reader.uint32()
              );
            break;
          case 2:
            message.OnSuccessCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
            );
            break;
          case 3:
            message.OnFailCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
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
    ): TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData {
      return {
        CallOwnerData: isSet(object.call_owner_data)
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
              object.call_owner_data
            )
          : undefined,
        OnSuccessCallback: isObject(object.on_success_callback)
          ? object.on_success_callback
          : undefined,
        OnFailCallback: isObject(object.on_fail_callback)
          ? object.on_fail_callback
          : undefined,
      };
    },

    toJSON(
      message: TVideoCallCapability_TAcceptVideoCallDirective_TTelegramAcceptVideoCallData
    ): unknown {
      const obj: any = {};
      message.CallOwnerData !== undefined &&
        (obj.call_owner_data = message.CallOwnerData
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
              message.CallOwnerData
            )
          : undefined);
      message.OnSuccessCallback !== undefined &&
        (obj.on_success_callback = message.OnSuccessCallback);
      message.OnFailCallback !== undefined &&
        (obj.on_fail_callback = message.OnFailCallback);
      return obj;
    },
  };

function createBaseTVideoCallCapability_TDiscardVideoCallDirective(): TVideoCallCapability_TDiscardVideoCallDirective {
  return { Name: "", TelegramDiscardVideoCallData: undefined };
}

export const TVideoCallCapability_TDiscardVideoCallDirective = {
  encode(
    message: TVideoCallCapability_TDiscardVideoCallDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramDiscardVideoCallData !== undefined) {
      TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData.encode(
        message.TelegramDiscardVideoCallData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TDiscardVideoCallDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TDiscardVideoCallDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramDiscardVideoCallData =
            TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TDiscardVideoCallDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramDiscardVideoCallData: isSet(
        object.telegram_discard_video_call_data
      )
        ? TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData.fromJSON(
            object.telegram_discard_video_call_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TDiscardVideoCallDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramDiscardVideoCallData !== undefined &&
      (obj.telegram_discard_video_call_data =
        message.TelegramDiscardVideoCallData
          ? TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData.toJSON(
              message.TelegramDiscardVideoCallData
            )
          : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData(): TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData {
  return {
    CallOwnerData: undefined,
    OnSuccessCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData =
  {
    encode(
      message: TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.CallOwnerData !== undefined) {
        TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
          message.CallOwnerData,
          writer.uint32(10).fork()
        ).ldelim();
      }
      if (message.OnSuccessCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnSuccessCallback),
          writer.uint32(18).fork()
        ).ldelim();
      }
      if (message.OnFailCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnFailCallback),
          writer.uint32(26).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.CallOwnerData =
              TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
                reader,
                reader.uint32()
              );
            break;
          case 2:
            message.OnSuccessCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
            );
            break;
          case 3:
            message.OnFailCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
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
    ): TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData {
      return {
        CallOwnerData: isSet(object.call_owner_data)
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
              object.call_owner_data
            )
          : undefined,
        OnSuccessCallback: isObject(object.on_success_callback)
          ? object.on_success_callback
          : undefined,
        OnFailCallback: isObject(object.on_fail_callback)
          ? object.on_fail_callback
          : undefined,
      };
    },

    toJSON(
      message: TVideoCallCapability_TDiscardVideoCallDirective_TTelegramDiscardVideoCallData
    ): unknown {
      const obj: any = {};
      message.CallOwnerData !== undefined &&
        (obj.call_owner_data = message.CallOwnerData
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
              message.CallOwnerData
            )
          : undefined);
      message.OnSuccessCallback !== undefined &&
        (obj.on_success_callback = message.OnSuccessCallback);
      message.OnFailCallback !== undefined &&
        (obj.on_fail_callback = message.OnFailCallback);
      return obj;
    },
  };

function createBaseTVideoCallCapability_TMuteMicDirective(): TVideoCallCapability_TMuteMicDirective {
  return { Name: "", TelegramMuteMicData: undefined };
}

export const TVideoCallCapability_TMuteMicDirective = {
  encode(
    message: TVideoCallCapability_TMuteMicDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramMuteMicData !== undefined) {
      TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData.encode(
        message.TelegramMuteMicData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TMuteMicDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TMuteMicDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramMuteMicData =
            TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TMuteMicDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramMuteMicData: isSet(object.telegram_mute_mic_data)
        ? TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData.fromJSON(
            object.telegram_mute_mic_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TMuteMicDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramMuteMicData !== undefined &&
      (obj.telegram_mute_mic_data = message.TelegramMuteMicData
        ? TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData.toJSON(
            message.TelegramMuteMicData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData(): TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData {
  return {
    CallOwnerData: undefined,
    OnSuccessCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData = {
  encode(
    message: TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CallOwnerData !== undefined) {
      TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
        message.CallOwnerData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.OnSuccessCallback !== undefined) {
      Struct.encode(
        Struct.wrap(message.OnSuccessCallback),
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.OnFailCallback !== undefined) {
      Struct.encode(
        Struct.wrap(message.OnFailCallback),
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CallOwnerData =
            TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.OnSuccessCallback = Struct.unwrap(
            Struct.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.OnFailCallback = Struct.unwrap(
            Struct.decode(reader, reader.uint32())
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
  ): TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData {
    return {
      CallOwnerData: isSet(object.call_owner_data)
        ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
            object.call_owner_data
          )
        : undefined,
      OnSuccessCallback: isObject(object.on_success_callback)
        ? object.on_success_callback
        : undefined,
      OnFailCallback: isObject(object.on_fail_callback)
        ? object.on_fail_callback
        : undefined,
    };
  },

  toJSON(
    message: TVideoCallCapability_TMuteMicDirective_TTelegramMuteMicData
  ): unknown {
    const obj: any = {};
    message.CallOwnerData !== undefined &&
      (obj.call_owner_data = message.CallOwnerData
        ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
            message.CallOwnerData
          )
        : undefined);
    message.OnSuccessCallback !== undefined &&
      (obj.on_success_callback = message.OnSuccessCallback);
    message.OnFailCallback !== undefined &&
      (obj.on_fail_callback = message.OnFailCallback);
    return obj;
  },
};

function createBaseTVideoCallCapability_TUnmuteMicDirective(): TVideoCallCapability_TUnmuteMicDirective {
  return { Name: "", TelegramUnmuteMicData: undefined };
}

export const TVideoCallCapability_TUnmuteMicDirective = {
  encode(
    message: TVideoCallCapability_TUnmuteMicDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramUnmuteMicData !== undefined) {
      TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData.encode(
        message.TelegramUnmuteMicData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TUnmuteMicDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TUnmuteMicDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramUnmuteMicData =
            TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TUnmuteMicDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramUnmuteMicData: isSet(object.telegram_unmute_mic_data)
        ? TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData.fromJSON(
            object.telegram_unmute_mic_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TUnmuteMicDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramUnmuteMicData !== undefined &&
      (obj.telegram_unmute_mic_data = message.TelegramUnmuteMicData
        ? TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData.toJSON(
            message.TelegramUnmuteMicData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData(): TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData {
  return {
    CallOwnerData: undefined,
    OnSuccessCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData = {
  encode(
    message: TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CallOwnerData !== undefined) {
      TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
        message.CallOwnerData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.OnSuccessCallback !== undefined) {
      Struct.encode(
        Struct.wrap(message.OnSuccessCallback),
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.OnFailCallback !== undefined) {
      Struct.encode(
        Struct.wrap(message.OnFailCallback),
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CallOwnerData =
            TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.OnSuccessCallback = Struct.unwrap(
            Struct.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.OnFailCallback = Struct.unwrap(
            Struct.decode(reader, reader.uint32())
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
  ): TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData {
    return {
      CallOwnerData: isSet(object.call_owner_data)
        ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
            object.call_owner_data
          )
        : undefined,
      OnSuccessCallback: isObject(object.on_success_callback)
        ? object.on_success_callback
        : undefined,
      OnFailCallback: isObject(object.on_fail_callback)
        ? object.on_fail_callback
        : undefined,
    };
  },

  toJSON(
    message: TVideoCallCapability_TUnmuteMicDirective_TTelegramUnmuteMicData
  ): unknown {
    const obj: any = {};
    message.CallOwnerData !== undefined &&
      (obj.call_owner_data = message.CallOwnerData
        ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
            message.CallOwnerData
          )
        : undefined);
    message.OnSuccessCallback !== undefined &&
      (obj.on_success_callback = message.OnSuccessCallback);
    message.OnFailCallback !== undefined &&
      (obj.on_fail_callback = message.OnFailCallback);
    return obj;
  },
};

function createBaseTVideoCallCapability_TTurnOnVideoDirective(): TVideoCallCapability_TTurnOnVideoDirective {
  return { Name: "", TelegramMuteTurnOnVideoData: undefined };
}

export const TVideoCallCapability_TTurnOnVideoDirective = {
  encode(
    message: TVideoCallCapability_TTurnOnVideoDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramMuteTurnOnVideoData !== undefined) {
      TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData.encode(
        message.TelegramMuteTurnOnVideoData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TTurnOnVideoDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TTurnOnVideoDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramMuteTurnOnVideoData =
            TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TTurnOnVideoDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramMuteTurnOnVideoData: isSet(object.telegram_turn_on_video_data)
        ? TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData.fromJSON(
            object.telegram_turn_on_video_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TTurnOnVideoDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramMuteTurnOnVideoData !== undefined &&
      (obj.telegram_turn_on_video_data = message.TelegramMuteTurnOnVideoData
        ? TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData.toJSON(
            message.TelegramMuteTurnOnVideoData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData(): TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData {
  return {
    CallOwnerData: undefined,
    OnSuccessCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData =
  {
    encode(
      message: TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.CallOwnerData !== undefined) {
        TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
          message.CallOwnerData,
          writer.uint32(10).fork()
        ).ldelim();
      }
      if (message.OnSuccessCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnSuccessCallback),
          writer.uint32(18).fork()
        ).ldelim();
      }
      if (message.OnFailCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnFailCallback),
          writer.uint32(26).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.CallOwnerData =
              TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
                reader,
                reader.uint32()
              );
            break;
          case 2:
            message.OnSuccessCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
            );
            break;
          case 3:
            message.OnFailCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
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
    ): TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData {
      return {
        CallOwnerData: isSet(object.call_owner_data)
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
              object.call_owner_data
            )
          : undefined,
        OnSuccessCallback: isObject(object.on_success_callback)
          ? object.on_success_callback
          : undefined,
        OnFailCallback: isObject(object.on_fail_callback)
          ? object.on_fail_callback
          : undefined,
      };
    },

    toJSON(
      message: TVideoCallCapability_TTurnOnVideoDirective_TTelegramTurnOnVideoData
    ): unknown {
      const obj: any = {};
      message.CallOwnerData !== undefined &&
        (obj.call_owner_data = message.CallOwnerData
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
              message.CallOwnerData
            )
          : undefined);
      message.OnSuccessCallback !== undefined &&
        (obj.on_success_callback = message.OnSuccessCallback);
      message.OnFailCallback !== undefined &&
        (obj.on_fail_callback = message.OnFailCallback);
      return obj;
    },
  };

function createBaseTVideoCallCapability_TTurnOffVideoDirective(): TVideoCallCapability_TTurnOffVideoDirective {
  return { Name: "", TelegramMuteTurnOffVideoData: undefined };
}

export const TVideoCallCapability_TTurnOffVideoDirective = {
  encode(
    message: TVideoCallCapability_TTurnOffVideoDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TelegramMuteTurnOffVideoData !== undefined) {
      TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData.encode(
        message.TelegramMuteTurnOffVideoData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallCapability_TTurnOffVideoDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallCapability_TTurnOffVideoDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TelegramMuteTurnOffVideoData =
            TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData.decode(
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

  fromJSON(object: any): TVideoCallCapability_TTurnOffVideoDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TelegramMuteTurnOffVideoData: isSet(object.telegram_turn_off_video_data)
        ? TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData.fromJSON(
            object.telegram_turn_off_video_data
          )
        : undefined,
    };
  },

  toJSON(message: TVideoCallCapability_TTurnOffVideoDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TelegramMuteTurnOffVideoData !== undefined &&
      (obj.telegram_turn_off_video_data = message.TelegramMuteTurnOffVideoData
        ? TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData.toJSON(
            message.TelegramMuteTurnOffVideoData
          )
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData(): TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData {
  return {
    CallOwnerData: undefined,
    OnSuccessCallback: undefined,
    OnFailCallback: undefined,
  };
}

export const TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData =
  {
    encode(
      message: TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.CallOwnerData !== undefined) {
        TVideoCallCapability_TTelegramVideoCallOwnerData.encode(
          message.CallOwnerData,
          writer.uint32(10).fork()
        ).ldelim();
      }
      if (message.OnSuccessCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnSuccessCallback),
          writer.uint32(18).fork()
        ).ldelim();
      }
      if (message.OnFailCallback !== undefined) {
        Struct.encode(
          Struct.wrap(message.OnFailCallback),
          writer.uint32(26).fork()
        ).ldelim();
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.CallOwnerData =
              TVideoCallCapability_TTelegramVideoCallOwnerData.decode(
                reader,
                reader.uint32()
              );
            break;
          case 2:
            message.OnSuccessCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
            );
            break;
          case 3:
            message.OnFailCallback = Struct.unwrap(
              Struct.decode(reader, reader.uint32())
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
    ): TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData {
      return {
        CallOwnerData: isSet(object.call_owner_data)
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.fromJSON(
              object.call_owner_data
            )
          : undefined,
        OnSuccessCallback: isObject(object.on_success_callback)
          ? object.on_success_callback
          : undefined,
        OnFailCallback: isObject(object.on_fail_callback)
          ? object.on_fail_callback
          : undefined,
      };
    },

    toJSON(
      message: TVideoCallCapability_TTurnOffVideoDirective_TTelegramTurnOffVideoData
    ): unknown {
      const obj: any = {};
      message.CallOwnerData !== undefined &&
        (obj.call_owner_data = message.CallOwnerData
          ? TVideoCallCapability_TTelegramVideoCallOwnerData.toJSON(
              message.CallOwnerData
            )
          : undefined);
      message.OnSuccessCallback !== undefined &&
        (obj.on_success_callback = message.OnSuccessCallback);
      message.OnFailCallback !== undefined &&
        (obj.on_fail_callback = message.OnFailCallback);
      return obj;
    },
  };

function createBaseTLevelCapability(): TLevelCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TLevelCapability = {
  encode(message: TLevelCapability, writer: Writer = Writer.create()): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TLevelCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TLevelCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TLevelCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TLevelCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TLevelCapability_TState.decode(
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

  fromJSON(object: any): TLevelCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TLevelCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TLevelCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TLevelCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TLevelCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TLevelCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTLevelCapability_TParameters(): TLevelCapability_TParameters {
  return { Instance: 0, Range: undefined, Unit: 0 };
}

export const TLevelCapability_TParameters = {
  encode(
    message: TLevelCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== 0) {
      writer.uint32(8).int32(message.Instance);
    }
    if (message.Range !== undefined) {
      TRange.encode(message.Range, writer.uint32(18).fork()).ldelim();
    }
    if (message.Unit !== 0) {
      writer.uint32(24).int32(message.Unit);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.int32() as any;
          break;
        case 2:
          message.Range = TRange.decode(reader, reader.uint32());
          break;
        case 3:
          message.Unit = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TParameters {
    return {
      Instance: isSet(object.instance)
        ? tLevelCapability_EInstanceFromJSON(object.instance)
        : 0,
      Range: isSet(object.range) ? TRange.fromJSON(object.range) : undefined,
      Unit: isSet(object.unit) ? eUnitFromJSON(object.unit) : 0,
    };
  },

  toJSON(message: TLevelCapability_TParameters): unknown {
    const obj: any = {};
    message.Instance !== undefined &&
      (obj.instance = tLevelCapability_EInstanceToJSON(message.Instance));
    message.Range !== undefined &&
      (obj.range = message.Range ? TRange.toJSON(message.Range) : undefined);
    message.Unit !== undefined && (obj.unit = eUnitToJSON(message.Unit));
    return obj;
  },
};

function createBaseTLevelCapability_TState(): TLevelCapability_TState {
  return { Level: 0 };
}

export const TLevelCapability_TState = {
  encode(
    message: TLevelCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Level !== 0) {
      writer.uint32(9).double(message.Level);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TLevelCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Level = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TState {
    return {
      Level: isSet(object.level) ? Number(object.level) : 0,
    };
  },

  toJSON(message: TLevelCapability_TState): unknown {
    const obj: any = {};
    message.Level !== undefined && (obj.level = message.Level);
    return obj;
  },
};

function createBaseTLevelCapability_TSetAbsoluteLevelDirective(): TLevelCapability_TSetAbsoluteLevelDirective {
  return { Name: "", TargetLevel: 0, TransitionTime: 0 };
}

export const TLevelCapability_TSetAbsoluteLevelDirective = {
  encode(
    message: TLevelCapability_TSetAbsoluteLevelDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TargetLevel !== 0) {
      writer.uint32(9).double(message.TargetLevel);
    }
    if (message.TransitionTime !== 0) {
      writer.uint32(16).uint32(message.TransitionTime);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TSetAbsoluteLevelDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TSetAbsoluteLevelDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TargetLevel = reader.double();
          break;
        case 2:
          message.TransitionTime = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TSetAbsoluteLevelDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TargetLevel: isSet(object.target_level) ? Number(object.target_level) : 0,
      TransitionTime: isSet(object.transition_time)
        ? Number(object.transition_time)
        : 0,
    };
  },

  toJSON(message: TLevelCapability_TSetAbsoluteLevelDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TargetLevel !== undefined &&
      (obj.target_level = message.TargetLevel);
    message.TransitionTime !== undefined &&
      (obj.transition_time = Math.round(message.TransitionTime));
    return obj;
  },
};

function createBaseTLevelCapability_TSetRelativeLevelDirective(): TLevelCapability_TSetRelativeLevelDirective {
  return { Name: "", RelativeLevel: 0, TransitionTime: 0 };
}

export const TLevelCapability_TSetRelativeLevelDirective = {
  encode(
    message: TLevelCapability_TSetRelativeLevelDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.RelativeLevel !== 0) {
      writer.uint32(9).double(message.RelativeLevel);
    }
    if (message.TransitionTime !== 0) {
      writer.uint32(16).uint32(message.TransitionTime);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TSetRelativeLevelDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TSetRelativeLevelDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.RelativeLevel = reader.double();
          break;
        case 2:
          message.TransitionTime = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TSetRelativeLevelDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      RelativeLevel: isSet(object.relative_level)
        ? Number(object.relative_level)
        : 0,
      TransitionTime: isSet(object.transition_time)
        ? Number(object.transition_time)
        : 0,
    };
  },

  toJSON(message: TLevelCapability_TSetRelativeLevelDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.RelativeLevel !== undefined &&
      (obj.relative_level = message.RelativeLevel);
    message.TransitionTime !== undefined &&
      (obj.transition_time = Math.round(message.TransitionTime));
    return obj;
  },
};

function createBaseTLevelCapability_TStartMoveLevelDirective(): TLevelCapability_TStartMoveLevelDirective {
  return { Name: "", ModeMode: 0, Rate: 0 };
}

export const TLevelCapability_TStartMoveLevelDirective = {
  encode(
    message: TLevelCapability_TStartMoveLevelDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.ModeMode !== 0) {
      writer.uint32(8).int32(message.ModeMode);
    }
    if (message.Rate !== 0) {
      writer.uint32(17).double(message.Rate);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TStartMoveLevelDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TStartMoveLevelDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.ModeMode = reader.int32() as any;
          break;
        case 2:
          message.Rate = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TStartMoveLevelDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      ModeMode: isSet(object.move_mode)
        ? tLevelCapability_EMoveModeFromJSON(object.move_mode)
        : 0,
      Rate: isSet(object.rate) ? Number(object.rate) : 0,
    };
  },

  toJSON(message: TLevelCapability_TStartMoveLevelDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.ModeMode !== undefined &&
      (obj.move_mode = tLevelCapability_EMoveModeToJSON(message.ModeMode));
    message.Rate !== undefined && (obj.rate = message.Rate);
    return obj;
  },
};

function createBaseTLevelCapability_TStopMoveLevelDirective(): TLevelCapability_TStopMoveLevelDirective {
  return { Name: "" };
}

export const TLevelCapability_TStopMoveLevelDirective = {
  encode(
    message: TLevelCapability_TStopMoveLevelDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TStopMoveLevelDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TStopMoveLevelDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TStopMoveLevelDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
    };
  },

  toJSON(message: TLevelCapability_TStopMoveLevelDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    return obj;
  },
};

function createBaseTLevelCapability_TUpdateStateEvent(): TLevelCapability_TUpdateStateEvent {
  return { Capability: undefined };
}

export const TLevelCapability_TUpdateStateEvent = {
  encode(
    message: TLevelCapability_TUpdateStateEvent,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Capability !== undefined) {
      TLevelCapability.encode(
        message.Capability,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TUpdateStateEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TUpdateStateEvent();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Capability = TLevelCapability.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TUpdateStateEvent {
    return {
      Capability: isSet(object.capability)
        ? TLevelCapability.fromJSON(object.capability)
        : undefined,
    };
  },

  toJSON(message: TLevelCapability_TUpdateStateEvent): unknown {
    const obj: any = {};
    message.Capability !== undefined &&
      (obj.capability = message.Capability
        ? TLevelCapability.toJSON(message.Capability)
        : undefined);
    return obj;
  },
};

function createBaseTLevelCapability_TCondition(): TLevelCapability_TCondition {
  return {
    Instance: 0,
    LowerBound: undefined,
    UpperBound: undefined,
    Hysteresis: 0,
  };
}

export const TLevelCapability_TCondition = {
  encode(
    message: TLevelCapability_TCondition,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Instance !== 0) {
      writer.uint32(8).int32(message.Instance);
    }
    if (message.LowerBound !== undefined) {
      DoubleValue.encode(
        { value: message.LowerBound! },
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.UpperBound !== undefined) {
      DoubleValue.encode(
        { value: message.UpperBound! },
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Hysteresis !== 0) {
      writer.uint32(33).double(message.Hysteresis);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLevelCapability_TCondition {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLevelCapability_TCondition();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Instance = reader.int32() as any;
          break;
        case 2:
          message.LowerBound = DoubleValue.decode(
            reader,
            reader.uint32()
          ).value;
          break;
        case 3:
          message.UpperBound = DoubleValue.decode(
            reader,
            reader.uint32()
          ).value;
          break;
        case 4:
          message.Hysteresis = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLevelCapability_TCondition {
    return {
      Instance: isSet(object.instance)
        ? tLevelCapability_EInstanceFromJSON(object.instance)
        : 0,
      LowerBound: isSet(object.lower_bound)
        ? Number(object.lower_bound)
        : undefined,
      UpperBound: isSet(object.upper_bound)
        ? Number(object.upper_bound)
        : undefined,
      Hysteresis: isSet(object.hysteresis) ? Number(object.hysteresis) : 0,
    };
  },

  toJSON(message: TLevelCapability_TCondition): unknown {
    const obj: any = {};
    message.Instance !== undefined &&
      (obj.instance = tLevelCapability_EInstanceToJSON(message.Instance));
    message.LowerBound !== undefined && (obj.lower_bound = message.LowerBound);
    message.UpperBound !== undefined && (obj.upper_bound = message.UpperBound);
    message.Hysteresis !== undefined && (obj.hysteresis = message.Hysteresis);
    return obj;
  },
};

function createBaseTColorCapability(): TColorCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TColorCapability = {
  encode(message: TColorCapability, writer: Writer = Writer.create()): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TColorCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TColorCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TColorCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTColorCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TColorCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TColorCapability_TState.decode(
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

  fromJSON(object: any): TColorCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TColorCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TColorCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TColorCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TColorCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TColorCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTColorCapability_TParameters(): TColorCapability_TParameters {
  return { ColorSceneParameters: undefined, TemperatureKParameters: undefined };
}

export const TColorCapability_TParameters = {
  encode(
    message: TColorCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ColorSceneParameters !== undefined) {
      TColorCapability_TParameters_TColorSceneParameters.encode(
        message.ColorSceneParameters,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.TemperatureKParameters !== undefined) {
      TColorCapability_TParameters_TTemperatureKParameters.encode(
        message.TemperatureKParameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TColorCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTColorCapability_TParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ColorSceneParameters =
            TColorCapability_TParameters_TColorSceneParameters.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.TemperatureKParameters =
            TColorCapability_TParameters_TTemperatureKParameters.decode(
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

  fromJSON(object: any): TColorCapability_TParameters {
    return {
      ColorSceneParameters: isSet(object.color_scene_parameters)
        ? TColorCapability_TParameters_TColorSceneParameters.fromJSON(
            object.color_scene_parameters
          )
        : undefined,
      TemperatureKParameters: isSet(object.temperature_k_parameters)
        ? TColorCapability_TParameters_TTemperatureKParameters.fromJSON(
            object.temperature_k_parameters
          )
        : undefined,
    };
  },

  toJSON(message: TColorCapability_TParameters): unknown {
    const obj: any = {};
    message.ColorSceneParameters !== undefined &&
      (obj.color_scene_parameters = message.ColorSceneParameters
        ? TColorCapability_TParameters_TColorSceneParameters.toJSON(
            message.ColorSceneParameters
          )
        : undefined);
    message.TemperatureKParameters !== undefined &&
      (obj.temperature_k_parameters = message.TemperatureKParameters
        ? TColorCapability_TParameters_TTemperatureKParameters.toJSON(
            message.TemperatureKParameters
          )
        : undefined);
    return obj;
  },
};

function createBaseTColorCapability_TParameters_TTemperatureKParameters(): TColorCapability_TParameters_TTemperatureKParameters {
  return { Range: undefined };
}

export const TColorCapability_TParameters_TTemperatureKParameters = {
  encode(
    message: TColorCapability_TParameters_TTemperatureKParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Range !== undefined) {
      TPositiveIntegerRange.encode(
        message.Range,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TColorCapability_TParameters_TTemperatureKParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTColorCapability_TParameters_TTemperatureKParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Range = TPositiveIntegerRange.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TColorCapability_TParameters_TTemperatureKParameters {
    return {
      Range: isSet(object.range)
        ? TPositiveIntegerRange.fromJSON(object.range)
        : undefined,
    };
  },

  toJSON(
    message: TColorCapability_TParameters_TTemperatureKParameters
  ): unknown {
    const obj: any = {};
    message.Range !== undefined &&
      (obj.range = message.Range
        ? TPositiveIntegerRange.toJSON(message.Range)
        : undefined);
    return obj;
  },
};

function createBaseTColorCapability_TParameters_TColorSceneParameters(): TColorCapability_TParameters_TColorSceneParameters {
  return { SupportedScenes: [] };
}

export const TColorCapability_TParameters_TColorSceneParameters = {
  encode(
    message: TColorCapability_TParameters_TColorSceneParameters,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.SupportedScenes) {
      writer.int32(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TColorCapability_TParameters_TColorSceneParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTColorCapability_TParameters_TColorSceneParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.SupportedScenes.push(reader.int32() as any);
            }
          } else {
            message.SupportedScenes.push(reader.int32() as any);
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TColorCapability_TParameters_TColorSceneParameters {
    return {
      SupportedScenes: Array.isArray(object?.supported_scenes)
        ? object.supported_scenes.map((e: any) =>
            tColorCapability_EColorSceneFromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TColorCapability_TParameters_TColorSceneParameters): unknown {
    const obj: any = {};
    if (message.SupportedScenes) {
      obj.supported_scenes = message.SupportedScenes.map((e) =>
        tColorCapability_EColorSceneToJSON(e)
      );
    } else {
      obj.supported_scenes = [];
    }
    return obj;
  },
};

function createBaseTColorCapability_TState(): TColorCapability_TState {
  return { ColorScene: undefined, TemperatureK: undefined };
}

export const TColorCapability_TState = {
  encode(
    message: TColorCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ColorScene !== undefined) {
      writer.uint32(8).int32(message.ColorScene);
    }
    if (message.TemperatureK !== undefined) {
      writer.uint32(16).uint64(message.TemperatureK);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TColorCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTColorCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ColorScene = reader.int32() as any;
          break;
        case 2:
          message.TemperatureK = longToNumber(reader.uint64() as Long);
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TColorCapability_TState {
    return {
      ColorScene: isSet(object.color_scene)
        ? tColorCapability_EColorSceneFromJSON(object.color_scene)
        : undefined,
      TemperatureK: isSet(object.temperature_k)
        ? Number(object.temperature_k)
        : undefined,
    };
  },

  toJSON(message: TColorCapability_TState): unknown {
    const obj: any = {};
    message.ColorScene !== undefined &&
      (obj.color_scene =
        message.ColorScene !== undefined
          ? tColorCapability_EColorSceneToJSON(message.ColorScene)
          : undefined);
    message.TemperatureK !== undefined &&
      (obj.temperature_k = Math.round(message.TemperatureK));
    return obj;
  },
};

function createBaseTColorCapability_TSetColorSceneDirective(): TColorCapability_TSetColorSceneDirective {
  return { Name: "", ColorScene: 0 };
}

export const TColorCapability_TSetColorSceneDirective = {
  encode(
    message: TColorCapability_TSetColorSceneDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.ColorScene !== 0) {
      writer.uint32(8).int32(message.ColorScene);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TColorCapability_TSetColorSceneDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTColorCapability_TSetColorSceneDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.ColorScene = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TColorCapability_TSetColorSceneDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      ColorScene: isSet(object.color_scene)
        ? tColorCapability_EColorSceneFromJSON(object.color_scene)
        : 0,
    };
  },

  toJSON(message: TColorCapability_TSetColorSceneDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.ColorScene !== undefined &&
      (obj.color_scene = tColorCapability_EColorSceneToJSON(
        message.ColorScene
      ));
    return obj;
  },
};

function createBaseTColorCapability_TSetTemperatureKDirective(): TColorCapability_TSetTemperatureKDirective {
  return { Name: "", TargetValue: 0 };
}

export const TColorCapability_TSetTemperatureKDirective = {
  encode(
    message: TColorCapability_TSetTemperatureKDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.TargetValue !== 0) {
      writer.uint32(8).uint64(message.TargetValue);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TColorCapability_TSetTemperatureKDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTColorCapability_TSetTemperatureKDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.TargetValue = longToNumber(reader.uint64() as Long);
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TColorCapability_TSetTemperatureKDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      TargetValue: isSet(object.target_value) ? Number(object.target_value) : 0,
    };
  },

  toJSON(message: TColorCapability_TSetTemperatureKDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.TargetValue !== undefined &&
      (obj.target_value = Math.round(message.TargetValue));
    return obj;
  },
};

function createBaseTColorCapability_TUpdateStateEvent(): TColorCapability_TUpdateStateEvent {
  return { Capability: undefined };
}

export const TColorCapability_TUpdateStateEvent = {
  encode(
    message: TColorCapability_TUpdateStateEvent,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Capability !== undefined) {
      TColorCapability.encode(
        message.Capability,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TColorCapability_TUpdateStateEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTColorCapability_TUpdateStateEvent();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Capability = TColorCapability.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TColorCapability_TUpdateStateEvent {
    return {
      Capability: isSet(object.capability)
        ? TColorCapability.fromJSON(object.capability)
        : undefined,
    };
  },

  toJSON(message: TColorCapability_TUpdateStateEvent): unknown {
    const obj: any = {};
    message.Capability !== undefined &&
      (obj.capability = message.Capability
        ? TColorCapability.toJSON(message.Capability)
        : undefined);
    return obj;
  },
};

function createBaseTWebOSCapability(): TWebOSCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TWebOSCapability = {
  encode(message: TWebOSCapability, writer: Writer = Writer.create()): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TWebOSCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TWebOSCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWebOSCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWebOSCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TWebOSCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TWebOSCapability_TState.decode(
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

  fromJSON(object: any): TWebOSCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TWebOSCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TWebOSCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TWebOSCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TWebOSCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TWebOSCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTWebOSCapability_TState(): TWebOSCapability_TState {
  return { ForegroundAppId: "" };
}

export const TWebOSCapability_TState = {
  encode(
    message: TWebOSCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ForegroundAppId !== "") {
      writer.uint32(10).string(message.ForegroundAppId);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWebOSCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWebOSCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ForegroundAppId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWebOSCapability_TState {
    return {
      ForegroundAppId: isSet(object.foreground_app_id)
        ? String(object.foreground_app_id)
        : "",
    };
  },

  toJSON(message: TWebOSCapability_TState): unknown {
    const obj: any = {};
    message.ForegroundAppId !== undefined &&
      (obj.foreground_app_id = message.ForegroundAppId);
    return obj;
  },
};

function createBaseTWebOSCapability_TParameters(): TWebOSCapability_TParameters {
  return { AvailableApps: [] };
}

export const TWebOSCapability_TParameters = {
  encode(
    message: TWebOSCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.AvailableApps) {
      TWebOSCapability_TParameters_TAppInfo.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWebOSCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWebOSCapability_TParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AvailableApps.push(
            TWebOSCapability_TParameters_TAppInfo.decode(
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

  fromJSON(object: any): TWebOSCapability_TParameters {
    return {
      AvailableApps: Array.isArray(object?.available_apps)
        ? object.available_apps.map((e: any) =>
            TWebOSCapability_TParameters_TAppInfo.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TWebOSCapability_TParameters): unknown {
    const obj: any = {};
    if (message.AvailableApps) {
      obj.available_apps = message.AvailableApps.map((e) =>
        e ? TWebOSCapability_TParameters_TAppInfo.toJSON(e) : undefined
      );
    } else {
      obj.available_apps = [];
    }
    return obj;
  },
};

function createBaseTWebOSCapability_TParameters_TAppInfo(): TWebOSCapability_TParameters_TAppInfo {
  return { AppId: "" };
}

export const TWebOSCapability_TParameters_TAppInfo = {
  encode(
    message: TWebOSCapability_TParameters_TAppInfo,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.AppId !== "") {
      writer.uint32(10).string(message.AppId);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWebOSCapability_TParameters_TAppInfo {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWebOSCapability_TParameters_TAppInfo();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AppId = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWebOSCapability_TParameters_TAppInfo {
    return {
      AppId: isSet(object.app_id) ? String(object.app_id) : "",
    };
  },

  toJSON(message: TWebOSCapability_TParameters_TAppInfo): unknown {
    const obj: any = {};
    message.AppId !== undefined && (obj.app_id = message.AppId);
    return obj;
  },
};

function createBaseTWebOSCapability_TWebOSLaunchAppDirective(): TWebOSCapability_TWebOSLaunchAppDirective {
  return { Name: "", AppId: "", ParamsJson: new Uint8Array() };
}

export const TWebOSCapability_TWebOSLaunchAppDirective = {
  encode(
    message: TWebOSCapability_TWebOSLaunchAppDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.AppId !== "") {
      writer.uint32(10).string(message.AppId);
    }
    if (message.ParamsJson.length !== 0) {
      writer.uint32(18).bytes(message.ParamsJson);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWebOSCapability_TWebOSLaunchAppDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWebOSCapability_TWebOSLaunchAppDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.AppId = reader.string();
          break;
        case 2:
          message.ParamsJson = reader.bytes();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWebOSCapability_TWebOSLaunchAppDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      AppId: isSet(object.app_id) ? String(object.app_id) : "",
      ParamsJson: isSet(object.params_json)
        ? bytesFromBase64(object.params_json)
        : new Uint8Array(),
    };
  },

  toJSON(message: TWebOSCapability_TWebOSLaunchAppDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.AppId !== undefined && (obj.app_id = message.AppId);
    message.ParamsJson !== undefined &&
      (obj.params_json = base64FromBytes(
        message.ParamsJson !== undefined ? message.ParamsJson : new Uint8Array()
      ));
    return obj;
  },
};

function createBaseTWebOSCapability_TWebOSShowGalleryDirective(): TWebOSCapability_TWebOSShowGalleryDirective {
  return { Name: "", ItemsJson: [] };
}

export const TWebOSCapability_TWebOSShowGalleryDirective = {
  encode(
    message: TWebOSCapability_TWebOSShowGalleryDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    for (const v of message.ItemsJson) {
      writer.uint32(10).bytes(v!);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWebOSCapability_TWebOSShowGalleryDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWebOSCapability_TWebOSShowGalleryDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.ItemsJson.push(reader.bytes());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWebOSCapability_TWebOSShowGalleryDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      ItemsJson: Array.isArray(object?.items_json)
        ? object.items_json.map((e: any) => bytesFromBase64(e))
        : [],
    };
  },

  toJSON(message: TWebOSCapability_TWebOSShowGalleryDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.ItemsJson) {
      obj.items_json = message.ItemsJson.map((e) =>
        base64FromBytes(e !== undefined ? e : new Uint8Array())
      );
    } else {
      obj.items_json = [];
    }
    return obj;
  },
};

function createBaseTButtonCapability(): TButtonCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TButtonCapability = {
  encode(message: TButtonCapability, writer: Writer = Writer.create()): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TButtonCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TButtonCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TButtonCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TButtonCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TButtonCapability_TState.decode(
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

  fromJSON(object: any): TButtonCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TButtonCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TButtonCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TButtonCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TButtonCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TButtonCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTButtonCapability_TParameters(): TButtonCapability_TParameters {
  return {};
}

export const TButtonCapability_TParameters = {
  encode(
    _: TButtonCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TParameters();
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

  fromJSON(_: any): TButtonCapability_TParameters {
    return {};
  },

  toJSON(_: TButtonCapability_TParameters): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTButtonCapability_TState(): TButtonCapability_TState {
  return {};
}

export const TButtonCapability_TState = {
  encode(
    _: TButtonCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TState();
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

  fromJSON(_: any): TButtonCapability_TState {
    return {};
  },

  toJSON(_: TButtonCapability_TState): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTButtonCapability_TButtonClickEvent(): TButtonCapability_TButtonClickEvent {
  return {};
}

export const TButtonCapability_TButtonClickEvent = {
  encode(
    _: TButtonCapability_TButtonClickEvent,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TButtonClickEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TButtonClickEvent();
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

  fromJSON(_: any): TButtonCapability_TButtonClickEvent {
    return {};
  },

  toJSON(_: TButtonCapability_TButtonClickEvent): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTButtonCapability_TButtonDoubleClickEvent(): TButtonCapability_TButtonDoubleClickEvent {
  return {};
}

export const TButtonCapability_TButtonDoubleClickEvent = {
  encode(
    _: TButtonCapability_TButtonDoubleClickEvent,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TButtonDoubleClickEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TButtonDoubleClickEvent();
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

  fromJSON(_: any): TButtonCapability_TButtonDoubleClickEvent {
    return {};
  },

  toJSON(_: TButtonCapability_TButtonDoubleClickEvent): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTButtonCapability_TButtonLongPressEvent(): TButtonCapability_TButtonLongPressEvent {
  return {};
}

export const TButtonCapability_TButtonLongPressEvent = {
  encode(
    _: TButtonCapability_TButtonLongPressEvent,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TButtonLongPressEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TButtonLongPressEvent();
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

  fromJSON(_: any): TButtonCapability_TButtonLongPressEvent {
    return {};
  },

  toJSON(_: TButtonCapability_TButtonLongPressEvent): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTButtonCapability_TButtonLongReleaseEvent(): TButtonCapability_TButtonLongReleaseEvent {
  return {};
}

export const TButtonCapability_TButtonLongReleaseEvent = {
  encode(
    _: TButtonCapability_TButtonLongReleaseEvent,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TButtonLongReleaseEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TButtonLongReleaseEvent();
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

  fromJSON(_: any): TButtonCapability_TButtonLongReleaseEvent {
    return {};
  },

  toJSON(_: TButtonCapability_TButtonLongReleaseEvent): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTButtonCapability_TCondition(): TButtonCapability_TCondition {
  return { Events: [] };
}

export const TButtonCapability_TCondition = {
  encode(
    message: TButtonCapability_TCondition,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.Events) {
      writer.int32(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TButtonCapability_TCondition {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTButtonCapability_TCondition();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.Events.push(reader.int32() as any);
            }
          } else {
            message.Events.push(reader.int32() as any);
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TButtonCapability_TCondition {
    return {
      Events: Array.isArray(object?.events)
        ? object.events.map((e: any) => tCapability_EEventTypeFromJSON(e))
        : [],
    };
  },

  toJSON(message: TButtonCapability_TCondition): unknown {
    const obj: any = {};
    if (message.Events) {
      obj.events = message.Events.map((e) => tCapability_EEventTypeToJSON(e));
    } else {
      obj.events = [];
    }
    return obj;
  },
};

function createBaseTEqualizerCapability(): TEqualizerCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TEqualizerCapability = {
  encode(
    message: TEqualizerCapability,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TEqualizerCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TEqualizerCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TEqualizerCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEqualizerCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TEqualizerCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TEqualizerCapability_TState.decode(
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

  fromJSON(object: any): TEqualizerCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TEqualizerCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TEqualizerCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TEqualizerCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TEqualizerCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TEqualizerCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTEqualizerCapability_TParameters(): TEqualizerCapability_TParameters {
  return {
    BandsLimits: undefined,
    SupportedPresetModes: [],
    Fixed: undefined,
    Adjustable: undefined,
  };
}

export const TEqualizerCapability_TParameters = {
  encode(
    message: TEqualizerCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.BandsLimits !== undefined) {
      TEqualizerCapability_TParameters_TBandsLimits.encode(
        message.BandsLimits,
        writer.uint32(10).fork()
      ).ldelim();
    }
    writer.uint32(18).fork();
    for (const v of message.SupportedPresetModes) {
      writer.int32(v);
    }
    writer.ldelim();
    if (message.Fixed !== undefined) {
      TEqualizerCapability_TParameters_TFixedBandsConfiguration.encode(
        message.Fixed,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Adjustable !== undefined) {
      TEqualizerCapability_TParameters_TAdjustableBandsConfiguration.encode(
        message.Adjustable,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEqualizerCapability_TParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.BandsLimits =
            TEqualizerCapability_TParameters_TBandsLimits.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.SupportedPresetModes.push(reader.int32() as any);
            }
          } else {
            message.SupportedPresetModes.push(reader.int32() as any);
          }
          break;
        case 3:
          message.Fixed =
            TEqualizerCapability_TParameters_TFixedBandsConfiguration.decode(
              reader,
              reader.uint32()
            );
          break;
        case 4:
          message.Adjustable =
            TEqualizerCapability_TParameters_TAdjustableBandsConfiguration.decode(
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

  fromJSON(object: any): TEqualizerCapability_TParameters {
    return {
      BandsLimits: isSet(object.bands_limits)
        ? TEqualizerCapability_TParameters_TBandsLimits.fromJSON(
            object.bands_limits
          )
        : undefined,
      SupportedPresetModes: Array.isArray(object?.supported_preset_modes)
        ? object.supported_preset_modes.map((e: any) =>
            tEqualizerCapability_EPresetModeFromJSON(e)
          )
        : [],
      Fixed: isSet(object.fixed)
        ? TEqualizerCapability_TParameters_TFixedBandsConfiguration.fromJSON(
            object.fixed
          )
        : undefined,
      Adjustable: isSet(object.adjustable)
        ? TEqualizerCapability_TParameters_TAdjustableBandsConfiguration.fromJSON(
            object.adjustable
          )
        : undefined,
    };
  },

  toJSON(message: TEqualizerCapability_TParameters): unknown {
    const obj: any = {};
    message.BandsLimits !== undefined &&
      (obj.bands_limits = message.BandsLimits
        ? TEqualizerCapability_TParameters_TBandsLimits.toJSON(
            message.BandsLimits
          )
        : undefined);
    if (message.SupportedPresetModes) {
      obj.supported_preset_modes = message.SupportedPresetModes.map((e) =>
        tEqualizerCapability_EPresetModeToJSON(e)
      );
    } else {
      obj.supported_preset_modes = [];
    }
    message.Fixed !== undefined &&
      (obj.fixed = message.Fixed
        ? TEqualizerCapability_TParameters_TFixedBandsConfiguration.toJSON(
            message.Fixed
          )
        : undefined);
    message.Adjustable !== undefined &&
      (obj.adjustable = message.Adjustable
        ? TEqualizerCapability_TParameters_TAdjustableBandsConfiguration.toJSON(
            message.Adjustable
          )
        : undefined);
    return obj;
  },
};

function createBaseTEqualizerCapability_TParameters_TBandsLimits(): TEqualizerCapability_TParameters_TBandsLimits {
  return { MinBandsCount: 0, MaxBandsCount: 0, MinBandGain: 0, MaxBandGain: 0 };
}

export const TEqualizerCapability_TParameters_TBandsLimits = {
  encode(
    message: TEqualizerCapability_TParameters_TBandsLimits,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.MinBandsCount !== 0) {
      writer.uint32(8).uint32(message.MinBandsCount);
    }
    if (message.MaxBandsCount !== 0) {
      writer.uint32(16).uint32(message.MaxBandsCount);
    }
    if (message.MinBandGain !== 0) {
      writer.uint32(25).double(message.MinBandGain);
    }
    if (message.MaxBandGain !== 0) {
      writer.uint32(33).double(message.MaxBandGain);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TParameters_TBandsLimits {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEqualizerCapability_TParameters_TBandsLimits();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.MinBandsCount = reader.uint32();
          break;
        case 2:
          message.MaxBandsCount = reader.uint32();
          break;
        case 3:
          message.MinBandGain = reader.double();
          break;
        case 4:
          message.MaxBandGain = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEqualizerCapability_TParameters_TBandsLimits {
    return {
      MinBandsCount: isSet(object.min_bands_count)
        ? Number(object.min_bands_count)
        : 0,
      MaxBandsCount: isSet(object.max_bands_count)
        ? Number(object.max_bands_count)
        : 0,
      MinBandGain: isSet(object.min_band_gain)
        ? Number(object.min_band_gain)
        : 0,
      MaxBandGain: isSet(object.max_band_gain)
        ? Number(object.max_band_gain)
        : 0,
    };
  },

  toJSON(message: TEqualizerCapability_TParameters_TBandsLimits): unknown {
    const obj: any = {};
    message.MinBandsCount !== undefined &&
      (obj.min_bands_count = Math.round(message.MinBandsCount));
    message.MaxBandsCount !== undefined &&
      (obj.max_bands_count = Math.round(message.MaxBandsCount));
    message.MinBandGain !== undefined &&
      (obj.min_band_gain = message.MinBandGain);
    message.MaxBandGain !== undefined &&
      (obj.max_band_gain = message.MaxBandGain);
    return obj;
  },
};

function createBaseTEqualizerCapability_TParameters_TAdjustableBandsConfiguration(): TEqualizerCapability_TParameters_TAdjustableBandsConfiguration {
  return {};
}

export const TEqualizerCapability_TParameters_TAdjustableBandsConfiguration = {
  encode(
    _: TEqualizerCapability_TParameters_TAdjustableBandsConfiguration,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TParameters_TAdjustableBandsConfiguration {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTEqualizerCapability_TParameters_TAdjustableBandsConfiguration();
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
  ): TEqualizerCapability_TParameters_TAdjustableBandsConfiguration {
    return {};
  },

  toJSON(
    _: TEqualizerCapability_TParameters_TAdjustableBandsConfiguration
  ): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTEqualizerCapability_TParameters_TFixedBandsConfiguration(): TEqualizerCapability_TParameters_TFixedBandsConfiguration {
  return { FixedBands: [] };
}

export const TEqualizerCapability_TParameters_TFixedBandsConfiguration = {
  encode(
    message: TEqualizerCapability_TParameters_TFixedBandsConfiguration,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.FixedBands) {
      TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TParameters_TFixedBandsConfiguration {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTEqualizerCapability_TParameters_TFixedBandsConfiguration();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FixedBands.push(
            TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand.decode(
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

  fromJSON(
    object: any
  ): TEqualizerCapability_TParameters_TFixedBandsConfiguration {
    return {
      FixedBands: Array.isArray(object?.fixed_bands)
        ? object.fixed_bands.map((e: any) =>
            TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand.fromJSON(
              e
            )
          )
        : [],
    };
  },

  toJSON(
    message: TEqualizerCapability_TParameters_TFixedBandsConfiguration
  ): unknown {
    const obj: any = {};
    if (message.FixedBands) {
      obj.fixed_bands = message.FixedBands.map((e) =>
        e
          ? TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand.toJSON(
              e
            )
          : undefined
      );
    } else {
      obj.fixed_bands = [];
    }
    return obj;
  },
};

function createBaseTEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand(): TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand {
  return { Frequency: 0, Width: 0 };
}

export const TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand =
  {
    encode(
      message: TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Frequency !== 0) {
        writer.uint32(9).double(message.Frequency);
      }
      if (message.Width !== 0) {
        writer.uint32(17).double(message.Width);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Frequency = reader.double();
            break;
          case 2:
            message.Width = reader.double();
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
    ): TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand {
      return {
        Frequency: isSet(object.frequency) ? Number(object.frequency) : 0,
        Width: isSet(object.width) ? Number(object.width) : 0,
      };
    },

    toJSON(
      message: TEqualizerCapability_TParameters_TFixedBandsConfiguration_TFixedBand
    ): unknown {
      const obj: any = {};
      message.Frequency !== undefined && (obj.frequency = message.Frequency);
      message.Width !== undefined && (obj.width = message.Width);
      return obj;
    },
  };

function createBaseTEqualizerCapability_TState(): TEqualizerCapability_TState {
  return { Bands: [], PresetMode: 0 };
}

export const TEqualizerCapability_TState = {
  encode(
    message: TEqualizerCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.Bands) {
      TEqualizerCapability_TBandState.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.PresetMode !== 0) {
      writer.uint32(16).int32(message.PresetMode);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEqualizerCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Bands.push(
            TEqualizerCapability_TBandState.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.PresetMode = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEqualizerCapability_TState {
    return {
      Bands: Array.isArray(object?.bands)
        ? object.bands.map((e: any) =>
            TEqualizerCapability_TBandState.fromJSON(e)
          )
        : [],
      PresetMode: isSet(object.preset_mode)
        ? tEqualizerCapability_EPresetModeFromJSON(object.preset_mode)
        : 0,
    };
  },

  toJSON(message: TEqualizerCapability_TState): unknown {
    const obj: any = {};
    if (message.Bands) {
      obj.bands = message.Bands.map((e) =>
        e ? TEqualizerCapability_TBandState.toJSON(e) : undefined
      );
    } else {
      obj.bands = [];
    }
    message.PresetMode !== undefined &&
      (obj.preset_mode = tEqualizerCapability_EPresetModeToJSON(
        message.PresetMode
      ));
    return obj;
  },
};

function createBaseTEqualizerCapability_TBandState(): TEqualizerCapability_TBandState {
  return { Frequency: 0, Gain: 0, Width: 0 };
}

export const TEqualizerCapability_TBandState = {
  encode(
    message: TEqualizerCapability_TBandState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Frequency !== 0) {
      writer.uint32(9).double(message.Frequency);
    }
    if (message.Gain !== 0) {
      writer.uint32(17).double(message.Gain);
    }
    if (message.Width !== 0) {
      writer.uint32(25).double(message.Width);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TBandState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEqualizerCapability_TBandState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Frequency = reader.double();
          break;
        case 2:
          message.Gain = reader.double();
          break;
        case 3:
          message.Width = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEqualizerCapability_TBandState {
    return {
      Frequency: isSet(object.frequency) ? Number(object.frequency) : 0,
      Gain: isSet(object.gain) ? Number(object.gain) : 0,
      Width: isSet(object.width) ? Number(object.width) : 0,
    };
  },

  toJSON(message: TEqualizerCapability_TBandState): unknown {
    const obj: any = {};
    message.Frequency !== undefined && (obj.frequency = message.Frequency);
    message.Gain !== undefined && (obj.gain = message.Gain);
    message.Width !== undefined && (obj.width = message.Width);
    return obj;
  },
};

function createBaseTEqualizerCapability_TSetAdjustableEqualizerBandsDirective(): TEqualizerCapability_TSetAdjustableEqualizerBandsDirective {
  return { Name: "", Bands: [] };
}

export const TEqualizerCapability_TSetAdjustableEqualizerBandsDirective = {
  encode(
    message: TEqualizerCapability_TSetAdjustableEqualizerBandsDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    for (const v of message.Bands) {
      TEqualizerCapability_TBandState.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TSetAdjustableEqualizerBandsDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTEqualizerCapability_TSetAdjustableEqualizerBandsDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.Bands.push(
            TEqualizerCapability_TBandState.decode(reader, reader.uint32())
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
  ): TEqualizerCapability_TSetAdjustableEqualizerBandsDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Bands: Array.isArray(object?.bands)
        ? object.bands.map((e: any) =>
            TEqualizerCapability_TBandState.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(
    message: TEqualizerCapability_TSetAdjustableEqualizerBandsDirective
  ): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Bands) {
      obj.bands = message.Bands.map((e) =>
        e ? TEqualizerCapability_TBandState.toJSON(e) : undefined
      );
    } else {
      obj.bands = [];
    }
    return obj;
  },
};

function createBaseTEqualizerCapability_TSetFixedEqualizerBandsDirective(): TEqualizerCapability_TSetFixedEqualizerBandsDirective {
  return { Name: "", Gains: [] };
}

export const TEqualizerCapability_TSetFixedEqualizerBandsDirective = {
  encode(
    message: TEqualizerCapability_TSetFixedEqualizerBandsDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    writer.uint32(10).fork();
    for (const v of message.Gains) {
      writer.double(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEqualizerCapability_TSetFixedEqualizerBandsDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTEqualizerCapability_TSetFixedEqualizerBandsDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.Gains.push(reader.double());
            }
          } else {
            message.Gains.push(reader.double());
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TEqualizerCapability_TSetFixedEqualizerBandsDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Gains: Array.isArray(object?.gains)
        ? object.gains.map((e: any) => Number(e))
        : [],
    };
  },

  toJSON(
    message: TEqualizerCapability_TSetFixedEqualizerBandsDirective
  ): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Gains) {
      obj.gains = message.Gains.map((e) => e);
    } else {
      obj.gains = [];
    }
    return obj;
  },
};

function createBaseTAnimationCapability(): TAnimationCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TAnimationCapability = {
  encode(
    message: TAnimationCapability,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TAnimationCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TAnimationCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TAnimationCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TAnimationCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TAnimationCapability_TState.decode(
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

  fromJSON(object: any): TAnimationCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TAnimationCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TAnimationCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TAnimationCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TAnimationCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TAnimationCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTAnimationCapability_TParameters(): TAnimationCapability_TParameters {
  return { SupportedFormats: [], Screens: [] };
}

export const TAnimationCapability_TParameters = {
  encode(
    message: TAnimationCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.SupportedFormats) {
      writer.int32(v);
    }
    writer.ldelim();
    for (const v of message.Screens) {
      TAnimationCapability_TScreen.encode(
        v!,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TParameters();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.SupportedFormats.push(reader.int32() as any);
            }
          } else {
            message.SupportedFormats.push(reader.int32() as any);
          }
          break;
        case 2:
          message.Screens.push(
            TAnimationCapability_TScreen.decode(reader, reader.uint32())
          );
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TParameters {
    return {
      SupportedFormats: Array.isArray(object?.supported_formats)
        ? object.supported_formats.map((e: any) =>
            tAnimationCapability_EFormatFromJSON(e)
          )
        : [],
      Screens: Array.isArray(object?.screens)
        ? object.screens.map((e: any) =>
            TAnimationCapability_TScreen.fromJSON(e)
          )
        : [],
    };
  },

  toJSON(message: TAnimationCapability_TParameters): unknown {
    const obj: any = {};
    if (message.SupportedFormats) {
      obj.supported_formats = message.SupportedFormats.map((e) =>
        tAnimationCapability_EFormatToJSON(e)
      );
    } else {
      obj.supported_formats = [];
    }
    if (message.Screens) {
      obj.screens = message.Screens.map((e) =>
        e ? TAnimationCapability_TScreen.toJSON(e) : undefined
      );
    } else {
      obj.screens = [];
    }
    return obj;
  },
};

function createBaseTAnimationCapability_TScreen(): TAnimationCapability_TScreen {
  return { Guid: "", ScreenType: 0 };
}

export const TAnimationCapability_TScreen = {
  encode(
    message: TAnimationCapability_TScreen,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Guid !== "") {
      writer.uint32(10).string(message.Guid);
    }
    if (message.ScreenType !== 0) {
      writer.uint32(16).int32(message.ScreenType);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TScreen {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TScreen();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Guid = reader.string();
          break;
        case 2:
          message.ScreenType = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TScreen {
    return {
      Guid: isSet(object.guid) ? String(object.guid) : "",
      ScreenType: isSet(object.screen_type)
        ? tAnimationCapability_TScreen_EScreenTypeFromJSON(object.screen_type)
        : 0,
    };
  },

  toJSON(message: TAnimationCapability_TScreen): unknown {
    const obj: any = {};
    message.Guid !== undefined && (obj.guid = message.Guid);
    message.ScreenType !== undefined &&
      (obj.screen_type = tAnimationCapability_TScreen_EScreenTypeToJSON(
        message.ScreenType
      ));
    return obj;
  },
};

function createBaseTAnimationCapability_TAnimation(): TAnimationCapability_TAnimation {
  return { AnimationType: 0 };
}

export const TAnimationCapability_TAnimation = {
  encode(
    message: TAnimationCapability_TAnimation,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.AnimationType !== 0) {
      writer.uint32(8).int32(message.AnimationType);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TAnimation {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TAnimation();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AnimationType = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TAnimation {
    return {
      AnimationType: isSet(object.animation_type)
        ? tAnimationCapability_TAnimation_EAnimationTypeFromJSON(
            object.animation_type
          )
        : 0,
    };
  },

  toJSON(message: TAnimationCapability_TAnimation): unknown {
    const obj: any = {};
    message.AnimationType !== undefined &&
      (obj.animation_type =
        tAnimationCapability_TAnimation_EAnimationTypeToJSON(
          message.AnimationType
        ));
    return obj;
  },
};

function createBaseTAnimationCapability_TState(): TAnimationCapability_TState {
  return { ScreenStatesMap: {} };
}

export const TAnimationCapability_TState = {
  encode(
    message: TAnimationCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    Object.entries(message.ScreenStatesMap).forEach(([key, value]) => {
      TAnimationCapability_TState_ScreenStatesMapEntry.encode(
        { key: key as any, value },
        writer.uint32(18).fork()
      ).ldelim();
    });
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          const entry2 =
            TAnimationCapability_TState_ScreenStatesMapEntry.decode(
              reader,
              reader.uint32()
            );
          if (entry2.value !== undefined) {
            message.ScreenStatesMap[entry2.key] = entry2.value;
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TState {
    return {
      ScreenStatesMap: isObject(object.screen_states_map)
        ? Object.entries(object.screen_states_map).reduce<{
            [key: string]: TAnimationCapability_TState_TScreenState;
          }>((acc, [key, value]) => {
            acc[key] = TAnimationCapability_TState_TScreenState.fromJSON(value);
            return acc;
          }, {})
        : {},
    };
  },

  toJSON(message: TAnimationCapability_TState): unknown {
    const obj: any = {};
    obj.screen_states_map = {};
    if (message.ScreenStatesMap) {
      Object.entries(message.ScreenStatesMap).forEach(([k, v]) => {
        obj.screen_states_map[k] =
          TAnimationCapability_TState_TScreenState.toJSON(v);
      });
    }
    return obj;
  },
};

function createBaseTAnimationCapability_TState_TScreenState(): TAnimationCapability_TState_TScreenState {
  return { Enabled: false, Animation: undefined };
}

export const TAnimationCapability_TState_TScreenState = {
  encode(
    message: TAnimationCapability_TState_TScreenState,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Enabled === true) {
      writer.uint32(16).bool(message.Enabled);
    }
    if (message.Animation !== undefined) {
      TAnimationCapability_TAnimation.encode(
        message.Animation,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TState_TScreenState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TState_TScreenState();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          message.Enabled = reader.bool();
          break;
        case 3:
          message.Animation = TAnimationCapability_TAnimation.decode(
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

  fromJSON(object: any): TAnimationCapability_TState_TScreenState {
    return {
      Enabled: isSet(object.enabled) ? Boolean(object.enabled) : false,
      Animation: isSet(object.animation)
        ? TAnimationCapability_TAnimation.fromJSON(object.animation)
        : undefined,
    };
  },

  toJSON(message: TAnimationCapability_TState_TScreenState): unknown {
    const obj: any = {};
    message.Enabled !== undefined && (obj.enabled = message.Enabled);
    message.Animation !== undefined &&
      (obj.animation = message.Animation
        ? TAnimationCapability_TAnimation.toJSON(message.Animation)
        : undefined);
    return obj;
  },
};

function createBaseTAnimationCapability_TState_ScreenStatesMapEntry(): TAnimationCapability_TState_ScreenStatesMapEntry {
  return { key: "", value: undefined };
}

export const TAnimationCapability_TState_ScreenStatesMapEntry = {
  encode(
    message: TAnimationCapability_TState_ScreenStatesMapEntry,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.key !== "") {
      writer.uint32(10).string(message.key);
    }
    if (message.value !== undefined) {
      TAnimationCapability_TState_TScreenState.encode(
        message.value,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TState_ScreenStatesMapEntry {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTAnimationCapability_TState_ScreenStatesMapEntry();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.key = reader.string();
          break;
        case 2:
          message.value = TAnimationCapability_TState_TScreenState.decode(
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

  fromJSON(object: any): TAnimationCapability_TState_ScreenStatesMapEntry {
    return {
      key: isSet(object.key) ? String(object.key) : "",
      value: isSet(object.value)
        ? TAnimationCapability_TState_TScreenState.fromJSON(object.value)
        : undefined,
    };
  },

  toJSON(message: TAnimationCapability_TState_ScreenStatesMapEntry): unknown {
    const obj: any = {};
    message.key !== undefined && (obj.key = message.key);
    message.value !== undefined &&
      (obj.value = message.value
        ? TAnimationCapability_TState_TScreenState.toJSON(message.value)
        : undefined);
    return obj;
  },
};

function createBaseTAnimationCapability_TDrawAnimationDirective(): TAnimationCapability_TDrawAnimationDirective {
  return {
    Name: "",
    Animations: [],
    AnimationStopPolicy: 0,
    SpeakingAnimationPolicy: 0,
  };
}

export const TAnimationCapability_TDrawAnimationDirective = {
  encode(
    message: TAnimationCapability_TDrawAnimationDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    for (const v of message.Animations) {
      TAnimationCapability_TDrawAnimationDirective_TAnimation.encode(
        v!,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.AnimationStopPolicy !== 0) {
      writer.uint32(16).int32(message.AnimationStopPolicy);
    }
    if (message.SpeakingAnimationPolicy !== 0) {
      writer.uint32(24).int32(message.SpeakingAnimationPolicy);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TDrawAnimationDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TDrawAnimationDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 1:
          message.Animations.push(
            TAnimationCapability_TDrawAnimationDirective_TAnimation.decode(
              reader,
              reader.uint32()
            )
          );
          break;
        case 2:
          message.AnimationStopPolicy = reader.int32() as any;
          break;
        case 3:
          message.SpeakingAnimationPolicy = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TDrawAnimationDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Animations: Array.isArray(object?.animations)
        ? object.animations.map((e: any) =>
            TAnimationCapability_TDrawAnimationDirective_TAnimation.fromJSON(e)
          )
        : [],
      AnimationStopPolicy: isSet(object.animation_stop_policy)
        ? tAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicyFromJSON(
            object.animation_stop_policy
          )
        : 0,
      SpeakingAnimationPolicy: isSet(object.speaking_animation_policy)
        ? tAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicyFromJSON(
            object.speaking_animation_policy
          )
        : 0,
    };
  },

  toJSON(message: TAnimationCapability_TDrawAnimationDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Animations) {
      obj.animations = message.Animations.map((e) =>
        e
          ? TAnimationCapability_TDrawAnimationDirective_TAnimation.toJSON(e)
          : undefined
      );
    } else {
      obj.animations = [];
    }
    message.AnimationStopPolicy !== undefined &&
      (obj.animation_stop_policy =
        tAnimationCapability_TDrawAnimationDirective_EAnimationStopPolicyToJSON(
          message.AnimationStopPolicy
        ));
    message.SpeakingAnimationPolicy !== undefined &&
      (obj.speaking_animation_policy =
        tAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicyToJSON(
          message.SpeakingAnimationPolicy
        ));
    return obj;
  },
};

function createBaseTAnimationCapability_TDrawAnimationDirective_TAnimation(): TAnimationCapability_TDrawAnimationDirective_TAnimation {
  return { BinaryAnimation: undefined, S3Directory: undefined };
}

export const TAnimationCapability_TDrawAnimationDirective_TAnimation = {
  encode(
    message: TAnimationCapability_TDrawAnimationDirective_TAnimation,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.BinaryAnimation !== undefined) {
      TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation.encode(
        message.BinaryAnimation,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.S3Directory !== undefined) {
      TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory.encode(
        message.S3Directory,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TDrawAnimationDirective_TAnimation {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTAnimationCapability_TDrawAnimationDirective_TAnimation();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.BinaryAnimation =
            TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation.decode(
              reader,
              reader.uint32()
            );
          break;
        case 2:
          message.S3Directory =
            TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory.decode(
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
  ): TAnimationCapability_TDrawAnimationDirective_TAnimation {
    return {
      BinaryAnimation: isSet(object.binary_animation)
        ? TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation.fromJSON(
            object.binary_animation
          )
        : undefined,
      S3Directory: isSet(object.s3_directory)
        ? TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory.fromJSON(
            object.s3_directory
          )
        : undefined,
    };
  },

  toJSON(
    message: TAnimationCapability_TDrawAnimationDirective_TAnimation
  ): unknown {
    const obj: any = {};
    message.BinaryAnimation !== undefined &&
      (obj.binary_animation = message.BinaryAnimation
        ? TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation.toJSON(
            message.BinaryAnimation
          )
        : undefined);
    message.S3Directory !== undefined &&
      (obj.s3_directory = message.S3Directory
        ? TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory.toJSON(
            message.S3Directory
          )
        : undefined);
    return obj;
  },
};

function createBaseTAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory(): TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory {
  return { Bucket: "", Path: "" };
}

export const TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory =
  {
    encode(
      message: TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Bucket !== "") {
        writer.uint32(10).string(message.Bucket);
      }
      if (message.Path !== "") {
        writer.uint32(18).string(message.Path);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Bucket = reader.string();
            break;
          case 2:
            message.Path = reader.string();
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
    ): TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory {
      return {
        Bucket: isSet(object.bucket) ? String(object.bucket) : "",
        Path: isSet(object.path) ? String(object.path) : "",
      };
    },

    toJSON(
      message: TAnimationCapability_TDrawAnimationDirective_TAnimation_TS3Directory
    ): unknown {
      const obj: any = {};
      message.Bucket !== undefined && (obj.bucket = message.Bucket);
      message.Path !== undefined && (obj.path = message.Path);
      return obj;
    },
  };

function createBaseTAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation(): TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation {
  return { Compression: 0, Base64EncodedValue: "" };
}

export const TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation =
  {
    encode(
      message: TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation,
      writer: Writer = Writer.create()
    ): Writer {
      if (message.Compression !== 0) {
        writer.uint32(8).int32(message.Compression);
      }
      if (message.Base64EncodedValue !== "") {
        writer.uint32(18).string(message.Base64EncodedValue);
      }
      return writer;
    },

    decode(
      input: Reader | Uint8Array,
      length?: number
    ): TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation {
      const reader = input instanceof Reader ? input : new Reader(input);
      let end = length === undefined ? reader.len : reader.pos + length;
      const message =
        createBaseTAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation();
      while (reader.pos < end) {
        const tag = reader.uint32();
        switch (tag >>> 3) {
          case 1:
            message.Compression = reader.int32() as any;
            break;
          case 2:
            message.Base64EncodedValue = reader.string();
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
    ): TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation {
      return {
        Compression: isSet(object.compression_type)
          ? tAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionTypeFromJSON(
              object.compression_type
            )
          : 0,
        Base64EncodedValue: isSet(object.base64_encoded_value)
          ? String(object.base64_encoded_value)
          : "",
      };
    },

    toJSON(
      message: TAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation
    ): unknown {
      const obj: any = {};
      message.Compression !== undefined &&
        (obj.compression_type =
          tAnimationCapability_TDrawAnimationDirective_TAnimation_TBinaryAnimation_ECompressionTypeToJSON(
            message.Compression
          ));
      message.Base64EncodedValue !== undefined &&
        (obj.base64_encoded_value = message.Base64EncodedValue);
      return obj;
    },
  };

function createBaseTAnimationCapability_TEnableScreenDirective(): TAnimationCapability_TEnableScreenDirective {
  return { Name: "", Guid: "" };
}

export const TAnimationCapability_TEnableScreenDirective = {
  encode(
    message: TAnimationCapability_TEnableScreenDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.Guid !== "") {
      writer.uint32(18).string(message.Guid);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TEnableScreenDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TEnableScreenDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 2:
          message.Guid = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TEnableScreenDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Guid: isSet(object.guid) ? String(object.guid) : "",
    };
  },

  toJSON(message: TAnimationCapability_TEnableScreenDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Guid !== undefined && (obj.guid = message.Guid);
    return obj;
  },
};

function createBaseTAnimationCapability_TDisableScreenDirective(): TAnimationCapability_TDisableScreenDirective {
  return { Name: "", Guid: "" };
}

export const TAnimationCapability_TDisableScreenDirective = {
  encode(
    message: TAnimationCapability_TDisableScreenDirective,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(8002).string(message.Name);
    }
    if (message.Guid !== "") {
      writer.uint32(18).string(message.Guid);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAnimationCapability_TDisableScreenDirective {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAnimationCapability_TDisableScreenDirective();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1000:
          message.Name = reader.string();
          break;
        case 2:
          message.Guid = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAnimationCapability_TDisableScreenDirective {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Guid: isSet(object.guid) ? String(object.guid) : "",
    };
  },

  toJSON(message: TAnimationCapability_TDisableScreenDirective): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Guid !== undefined && (obj.guid = message.Guid);
    return obj;
  },
};

function createBaseTMotionCapability(): TMotionCapability {
  return { Meta: undefined, Parameters: undefined, State: undefined };
}

export const TMotionCapability = {
  encode(message: TMotionCapability, writer: Writer = Writer.create()): Writer {
    if (message.Meta !== undefined) {
      TCapability_TMeta.encode(message.Meta, writer.uint32(10).fork()).ldelim();
    }
    if (message.Parameters !== undefined) {
      TMotionCapability_TParameters.encode(
        message.Parameters,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.State !== undefined) {
      TMotionCapability_TState.encode(
        message.State,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TMotionCapability {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMotionCapability();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Meta = TCapability_TMeta.decode(reader, reader.uint32());
          break;
        case 2:
          message.Parameters = TMotionCapability_TParameters.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.State = TMotionCapability_TState.decode(
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

  fromJSON(object: any): TMotionCapability {
    return {
      Meta: isSet(object.meta)
        ? TCapability_TMeta.fromJSON(object.meta)
        : undefined,
      Parameters: isSet(object.parameters)
        ? TMotionCapability_TParameters.fromJSON(object.parameters)
        : undefined,
      State: isSet(object.state)
        ? TMotionCapability_TState.fromJSON(object.state)
        : undefined,
    };
  },

  toJSON(message: TMotionCapability): unknown {
    const obj: any = {};
    message.Meta !== undefined &&
      (obj.meta = message.Meta
        ? TCapability_TMeta.toJSON(message.Meta)
        : undefined);
    message.Parameters !== undefined &&
      (obj.parameters = message.Parameters
        ? TMotionCapability_TParameters.toJSON(message.Parameters)
        : undefined);
    message.State !== undefined &&
      (obj.state = message.State
        ? TMotionCapability_TState.toJSON(message.State)
        : undefined);
    return obj;
  },
};

function createBaseTMotionCapability_TParameters(): TMotionCapability_TParameters {
  return {};
}

export const TMotionCapability_TParameters = {
  encode(
    _: TMotionCapability_TParameters,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMotionCapability_TParameters {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMotionCapability_TParameters();
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

  fromJSON(_: any): TMotionCapability_TParameters {
    return {};
  },

  toJSON(_: TMotionCapability_TParameters): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMotionCapability_TState(): TMotionCapability_TState {
  return {};
}

export const TMotionCapability_TState = {
  encode(
    _: TMotionCapability_TState,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMotionCapability_TState {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMotionCapability_TState();
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

  fromJSON(_: any): TMotionCapability_TState {
    return {};
  },

  toJSON(_: TMotionCapability_TState): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMotionCapability_TMotionDetectedEvent(): TMotionCapability_TMotionDetectedEvent {
  return {};
}

export const TMotionCapability_TMotionDetectedEvent = {
  encode(
    _: TMotionCapability_TMotionDetectedEvent,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMotionCapability_TMotionDetectedEvent {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMotionCapability_TMotionDetectedEvent();
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

  fromJSON(_: any): TMotionCapability_TMotionDetectedEvent {
    return {};
  },

  toJSON(_: TMotionCapability_TMotionDetectedEvent): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMotionCapability_TCondition(): TMotionCapability_TCondition {
  return { Events: [] };
}

export const TMotionCapability_TCondition = {
  encode(
    message: TMotionCapability_TCondition,
    writer: Writer = Writer.create()
  ): Writer {
    writer.uint32(10).fork();
    for (const v of message.Events) {
      writer.int32(v);
    }
    writer.ldelim();
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMotionCapability_TCondition {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMotionCapability_TCondition();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          if ((tag & 7) === 2) {
            const end2 = reader.uint32() + reader.pos;
            while (reader.pos < end2) {
              message.Events.push(reader.int32() as any);
            }
          } else {
            message.Events.push(reader.int32() as any);
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMotionCapability_TCondition {
    return {
      Events: Array.isArray(object?.events)
        ? object.events.map((e: any) => tCapability_EEventTypeFromJSON(e))
        : [],
    };
  },

  toJSON(message: TMotionCapability_TCondition): unknown {
    const obj: any = {};
    if (message.Events) {
      obj.events = message.Events.map((e) => tCapability_EEventTypeToJSON(e));
    } else {
      obj.events = [];
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

function isObject(value: any): boolean {
  return typeof value === "object" && value !== null;
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
