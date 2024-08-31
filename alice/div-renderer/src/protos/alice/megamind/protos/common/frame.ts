/* eslint-disable */
import { util, configure, Writer, Reader } from "protobufjs/minimal";
import * as Long from "long";
import {
  ELang,
  eLangFromJSON,
  eLangToJSON,
} from "../../../../alice/protos/data/language/language";
import { TAnalyticsTrackingModule } from "../../../../alice/megamind/protos/common/atm";
import { TOrigin } from "../../../../alice/megamind/protos/common/origin";
import { TFrameRequestParams } from "../../../../alice/megamind/protos/common/frame_request_params";
import { TRequestParams } from "../../../../alice/megamind/protos/common/request_params";
import { TVideoGalleryItemMeta } from "../../../../alice/protos/data/entity_meta/video_nlu_meta";
import { TNewsProvider } from "../../../../alice/protos/data/news_provider";
import { TTopic } from "../../../../alice/protos/data/scenario/music/topic";
import {
  TDayPart_EValue,
  TAge_EValue,
  tDayPart_EValueFromJSON,
  tDayPart_EValueToJSON,
  tAge_EValueFromJSON,
  tAge_EValueToJSON,
} from "../../../../alice/protos/data/scenario/alice_show/selectors";
import {
  ENotificationType,
  eNotificationTypeFromJSON,
  eNotificationTypeToJSON,
} from "../../../../alice/protos/data/scenario/order/order";
import { TIotDiscoveryCapability_TNetworks } from "../../../../alice/protos/endpoint/capability";
import { TLatLon } from "../../../../alice/protos/data/lat_lon";
import {
  TStartIotDiscoveryRequest,
  TFinishIotDiscoveryRequest,
  TForgetIotEndpointsRequest,
  TEndpointStateUpdatesRequest,
  TIoTYandexIOActionRequest,
  TIoTDeviceActionsBatch,
  TIoTCapabilityAction,
  TIoTDeviceActionRequest,
} from "../../../../alice/megamind/protos/common/iot";
import {
  TEndpointCapabilityEvents,
  TEndpointEventsBatch,
} from "../../../../alice/protos/endpoint/events/events";
import { TTandemTemplate } from "../../../../alice/protos/data/tv_feature_boarding/template";
import { TContentId } from "../../../../alice/protos/data/scenario/music/content_id";
import { TEnrollmentStatus } from "../../../../alice/protos/data/scenario/iot/enrollment_status";
import {
  TWidgetPosition,
  TCentaurWidgetData,
  TCentaurWidgetConfigData,
} from "../../../../alice/protos/data/scenario/centaur/my_screen/widgets";
import { TCatalogTags } from "../../../../alice/protos/data/tv/tags/catalog_tag";
import { TTeaserSettingsData } from "../../../../alice/protos/data/scenario/centaur/teasers/teaser_settings";
import { TFmRadioInfo } from "../../../../alice/protos/data/fm_radio_info";
import { TUpdateContactsRequest } from "../../../../alice/protos/data/contacts";
import {
  TProviderContactData,
  TProviderContactList,
} from "../../../../alice/protos/data/scenario/video_call/video_call";

export const protobufPackage = "NAlice";

export interface TNluPhrase {
  Language: ELang;
  Phrase: string;
}

export interface TNluHint {
  Instances: TNluPhrase[];
  Video?: TVideoGalleryItemMeta | undefined;
  /**
   * We try to not trigger the item when one of the phrases in Negatives
   * is a better match for the request than any phrase in Instances.
   * The item should not be triggered if request matches some phrase from Negatives exactly
   */
  Negatives: TNluPhrase[];
}

export interface TFrameNluHint {
  /** Name of the existing frame to be matched, or a stub. */
  FrameName: string;
  /** Phrases in particular languages that correspond to the button. */
  Instances: TNluPhrase[];
  /**
   * Phrases that do NOT correspond to the button, and shouldn't be
   * recognized as activating it.
   *
   * TODO: clarify with the0@
   */
  Negatives: TNluPhrase[];
}

export interface TClientEntity {
  Name: string;
  /** Map entity value to matching phrases/grammars. */
  Items: { [key: string]: TNluHint };
}

export interface TClientEntity_ItemsEntry {
  key: string;
  value?: TNluHint;
}

export interface TClientEntityList {
  Entities: TClientEntity[];
}

export interface TSlotValue {
  Type: string;
  /** todo: add other types */
  String: string | undefined;
}

export interface TStringSlot {
  StringValue: string | undefined;
  SpecialPlaylistValue: string | undefined;
  NewsTopicValue: string | undefined;
  SpecialAnswerInfoValue: string | undefined;
  VideoContentTypeValue: string | undefined;
  VideoActionValue: string | undefined;
  /**
   * reserved 7;
   * legacy value, use TActionRequestSlot for new frames
   *
   * @deprecated
   */
  ActionRequestValue: string | undefined;
  EpochValue: string | undefined;
  VideoSelectionActionValue: string | undefined;
}

export interface TUInt32Slot {
  UInt32Value: number | undefined;
}

export interface TNumSlot {
  NumValue: number | undefined;
}

export interface TSysNumSlot {
  NumValue: number | undefined;
}

export interface TDoubleSlot {
  DoubleValue: number | undefined;
}

export interface TBoolSlot {
  BoolValue: boolean | undefined;
}

export interface TNotificationSlot {
  SubscriptionValue: string | undefined;
}

/** SpecialPlaylist, SpecialAnswerInfo and ActionRequest are left in TStringSlot for compatibility */
export interface TMusicSlot {
  GenreValue: string | undefined;
  MoodValue: string | undefined;
  ActivityValue: string | undefined;
  VocalValue: string | undefined;
  NoveltyValue: string | undefined;
  PersonalityValue: string | undefined;
  /**
   * legacy value, use TOrderSlot for new frames
   *
   * @deprecated
   */
  OrderValue: string | undefined;
  /**
   * legacy value, use TRepeatSlot for new frames
   *
   * @deprecated
   */
  RepeatValue: string | undefined;
  RoomValue: string | undefined;
  StreamValue: string | undefined;
  GenerativeStationValue: string | undefined;
  /**
   * legacy value, use TNeedSimilarSlot for new frames
   *
   * @deprecated
   */
  NeedSimilarValue: string | undefined;
  OffsetValue: string | undefined;
}

export interface TMusicContentTypeSlot {
  EnumValue: TMusicContentTypeSlot_EValue | undefined;
}

export enum TMusicContentTypeSlot_EValue {
  Unknown = 0,
  Podcast = 1,
  Audiobook = 2,
  FairyTale = 3,
  UNRECOGNIZED = -1,
}

export function tMusicContentTypeSlot_EValueFromJSON(
  object: any
): TMusicContentTypeSlot_EValue {
  switch (object) {
    case 0:
    case "Unknown":
      return TMusicContentTypeSlot_EValue.Unknown;
    case 1:
    case "Podcast":
      return TMusicContentTypeSlot_EValue.Podcast;
    case 2:
    case "Audiobook":
      return TMusicContentTypeSlot_EValue.Audiobook;
    case 3:
    case "FairyTale":
      return TMusicContentTypeSlot_EValue.FairyTale;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TMusicContentTypeSlot_EValue.UNRECOGNIZED;
  }
}

export function tMusicContentTypeSlot_EValueToJSON(
  object: TMusicContentTypeSlot_EValue
): string {
  switch (object) {
    case TMusicContentTypeSlot_EValue.Unknown:
      return "Unknown";
    case TMusicContentTypeSlot_EValue.Podcast:
      return "Podcast";
    case TMusicContentTypeSlot_EValue.Audiobook:
      return "Audiobook";
    case TMusicContentTypeSlot_EValue.FairyTale:
      return "FairyTale";
    default:
      return "UNKNOWN";
  }
}

export interface TVideoSlot {
  NewValue: string | undefined;
}

export interface TLanguageSlot {
  LanguageValue: string | undefined;
}

export interface TDateTimeSlot {
  DateTimeValue: string | undefined;
}

export interface TUnitsTimeSlot {
  StringValue: string | undefined;
  UnitsTimeValue: string | undefined;
}

export interface TNewsProviderSlot {
  NewsProviderValue?: TNewsProvider | undefined;
}

export interface TTopicSlot {
  TopicValue?: TTopic | undefined;
}

export interface TDayPartSlot {
  DayPartValue: TDayPart_EValue | undefined;
}

export interface TAgeSlot {
  AgeValue: TAge_EValue | undefined;
}

export interface TOrderNotificationTypeSlot {
  OrderNotificationTypeValue: ENotificationType | undefined;
}

export interface TAppData {
  AppDataValue: string | undefined;
}

export interface THardcodedResponseName {
  HardcodedResponseNameValue: string | undefined;
}

export interface TIotNetworksSlot {
  NetworksValue?: TIotDiscoveryCapability_TNetworks | undefined;
}

export interface TWhereSlot {
  WhereValue: string | undefined;
  SpecialLocationValue: string | undefined;
  LatLonValue?: TLatLon | undefined;
}

export interface TLocationSlot {
  UserIotRoomValue: string | undefined;
  DeviceIotRoomValue: string | undefined;
  UserIotGroupValue: string | undefined;
  DeviceIotGroupValue: string | undefined;
  UserIotDeviceValue: string | undefined;
  DeviceIotDeviceValue: string | undefined;
  UserIotMultiroomAllDevicesValue: string | undefined;
  DeviceIotMultiroomAllDevicesValue: string | undefined;
}

export interface TStartIotDiscoveryRequestSlot {
  RequestValue?: TStartIotDiscoveryRequest | undefined;
}

export interface TFinishIotDiscoveryRequestSlot {
  RequestValue?: TFinishIotDiscoveryRequest | undefined;
}

export interface TForgetIotEndpointsRequestSlot {
  RequestValue?: TForgetIotEndpointsRequest | undefined;
}

export interface TEndpointStateUpdatesRequestSlot {
  RequestValue?: TEndpointStateUpdatesRequest | undefined;
}

/** deprecated */
export interface TEndpointCapabilityEventsSlot {
  EventsValue?: TEndpointCapabilityEvents | undefined;
}

export interface TEndpointEventsBatchSlot {
  BatchValue?: TEndpointEventsBatch | undefined;
}

/** this is only relevant while https://st.yandex-team.ru/ZION-93 is not available and should be removed later */
export interface TIotYandexIOActionRequestSlot {
  RequestValue?: TIoTYandexIOActionRequest | undefined;
}

export interface TTvChosenTemplateSlot {
  TandemTemplate?: TTandemTemplate | undefined;
}

export interface TIoTDeviceActionsBatchSlot {
  BatchValue?: TIoTDeviceActionsBatch | undefined;
}

export interface TContentIdSlot {
  ContentIdValue?: TContentId | undefined;
}

export interface TFixlistInfoSlot {
  FixlistInfoValue: string | undefined;
}

export interface TTargetTypeSlot {
  TargetTypeValue: TTargetTypeSlot_EValue | undefined;
}

export enum TTargetTypeSlot_EValue {
  unknown = 0,
  track = 1,
  album = 2,
  artist = 3,
  playlist = 4,
  radio = 5,
  UNRECOGNIZED = -1,
}

export function tTargetTypeSlot_EValueFromJSON(
  object: any
): TTargetTypeSlot_EValue {
  switch (object) {
    case 0:
    case "unknown":
      return TTargetTypeSlot_EValue.unknown;
    case 1:
    case "track":
      return TTargetTypeSlot_EValue.track;
    case 2:
    case "album":
      return TTargetTypeSlot_EValue.album;
    case 3:
    case "artist":
      return TTargetTypeSlot_EValue.artist;
    case 4:
    case "playlist":
      return TTargetTypeSlot_EValue.playlist;
    case 5:
    case "radio":
      return TTargetTypeSlot_EValue.radio;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TTargetTypeSlot_EValue.UNRECOGNIZED;
  }
}

export function tTargetTypeSlot_EValueToJSON(
  object: TTargetTypeSlot_EValue
): string {
  switch (object) {
    case TTargetTypeSlot_EValue.unknown:
      return "unknown";
    case TTargetTypeSlot_EValue.track:
      return "track";
    case TTargetTypeSlot_EValue.album:
      return "album";
    case TTargetTypeSlot_EValue.artist:
      return "artist";
    case TTargetTypeSlot_EValue.playlist:
      return "playlist";
    case TTargetTypeSlot_EValue.radio:
      return "radio";
    default:
      return "UNKNOWN";
  }
}

export interface TActionRequestSlot {
  ActionRequestValue: TActionRequestSlot_EValue | undefined;
}

export enum TActionRequestSlot_EValue {
  unknown = 0,
  autoplay = 1,
  like = 2,
  dislike = 3,
  UNRECOGNIZED = -1,
}

export function tActionRequestSlot_EValueFromJSON(
  object: any
): TActionRequestSlot_EValue {
  switch (object) {
    case 0:
    case "unknown":
      return TActionRequestSlot_EValue.unknown;
    case 1:
    case "autoplay":
      return TActionRequestSlot_EValue.autoplay;
    case 2:
    case "like":
      return TActionRequestSlot_EValue.like;
    case 3:
    case "dislike":
      return TActionRequestSlot_EValue.dislike;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TActionRequestSlot_EValue.UNRECOGNIZED;
  }
}

export function tActionRequestSlot_EValueToJSON(
  object: TActionRequestSlot_EValue
): string {
  switch (object) {
    case TActionRequestSlot_EValue.unknown:
      return "unknown";
    case TActionRequestSlot_EValue.autoplay:
      return "autoplay";
    case TActionRequestSlot_EValue.like:
      return "like";
    case TActionRequestSlot_EValue.dislike:
      return "dislike";
    default:
      return "UNKNOWN";
  }
}

export interface TNeedSimilarSlot {
  NeedSimilarValue: TNeedSimilarSlot_EValue | undefined;
}

export enum TNeedSimilarSlot_EValue {
  unknown = 0,
  need_similar = 1,
  UNRECOGNIZED = -1,
}

export function tNeedSimilarSlot_EValueFromJSON(
  object: any
): TNeedSimilarSlot_EValue {
  switch (object) {
    case 0:
    case "unknown":
      return TNeedSimilarSlot_EValue.unknown;
    case 1:
    case "need_similar":
      return TNeedSimilarSlot_EValue.need_similar;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TNeedSimilarSlot_EValue.UNRECOGNIZED;
  }
}

export function tNeedSimilarSlot_EValueToJSON(
  object: TNeedSimilarSlot_EValue
): string {
  switch (object) {
    case TNeedSimilarSlot_EValue.unknown:
      return "unknown";
    case TNeedSimilarSlot_EValue.need_similar:
      return "need_similar";
    default:
      return "UNKNOWN";
  }
}

export interface TRepeatSlot {
  RepeatValue: TRepeatSlot_EValue | undefined;
}

export enum TRepeatSlot_EValue {
  unknown = 0,
  repeat = 1,
  UNRECOGNIZED = -1,
}

export function tRepeatSlot_EValueFromJSON(object: any): TRepeatSlot_EValue {
  switch (object) {
    case 0:
    case "unknown":
      return TRepeatSlot_EValue.unknown;
    case 1:
    case "repeat":
      return TRepeatSlot_EValue.repeat;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TRepeatSlot_EValue.UNRECOGNIZED;
  }
}

export function tRepeatSlot_EValueToJSON(object: TRepeatSlot_EValue): string {
  switch (object) {
    case TRepeatSlot_EValue.unknown:
      return "unknown";
    case TRepeatSlot_EValue.repeat:
      return "repeat";
    default:
      return "UNKNOWN";
  }
}

export interface TOrderSlot {
  OrderValue: TOrderSlot_EValue | undefined;
}

export enum TOrderSlot_EValue {
  unknown = 0,
  shuffle = 1,
  UNRECOGNIZED = -1,
}

export function tOrderSlot_EValueFromJSON(object: any): TOrderSlot_EValue {
  switch (object) {
    case 0:
    case "unknown":
      return TOrderSlot_EValue.unknown;
    case 1:
    case "shuffle":
      return TOrderSlot_EValue.shuffle;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TOrderSlot_EValue.UNRECOGNIZED;
  }
}

export function tOrderSlot_EValueToJSON(object: TOrderSlot_EValue): string {
  switch (object) {
    case TOrderSlot_EValue.unknown:
      return "unknown";
    case TOrderSlot_EValue.shuffle:
      return "shuffle";
    default:
      return "UNKNOWN";
  }
}

export interface TFairytaleThemeSlot {
  FairytaleThemeValue: TFairytaleThemeSlot_EValue | undefined;
}

export enum TFairytaleThemeSlot_EValue {
  unknown = 0,
  bedtime = 1,
  UNRECOGNIZED = -1,
}

export function tFairytaleThemeSlot_EValueFromJSON(
  object: any
): TFairytaleThemeSlot_EValue {
  switch (object) {
    case 0:
    case "unknown":
      return TFairytaleThemeSlot_EValue.unknown;
    case 1:
    case "bedtime":
      return TFairytaleThemeSlot_EValue.bedtime;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TFairytaleThemeSlot_EValue.UNRECOGNIZED;
  }
}

export function tFairytaleThemeSlot_EValueToJSON(
  object: TFairytaleThemeSlot_EValue
): string {
  switch (object) {
    case TFairytaleThemeSlot_EValue.unknown:
      return "unknown";
    case TFairytaleThemeSlot_EValue.bedtime:
      return "bedtime";
    default:
      return "UNKNOWN";
  }
}

export interface TSelectVideoFromGallerySemanticFrame {
  Action?: TStringSlot;
  VideoIndex?: TNumSlot;
  SilentResponse?: TBoolSlot;
}

export interface TOpenCurrentVideoSemanticFrame {
  Action?: TStringSlot;
  SilentResponse?: TBoolSlot;
}

export interface TOpenCurrentTrailerSemanticFrame {
  SilentResponse?: TBoolSlot;
}

export interface TOrderNotificationSemanticFrame {
  ProviderName?: TStringSlot;
  OrderId?: TStringSlot;
  OrderNotificationType?: TOrderNotificationTypeSlot;
}

export interface TVideoPlayerFinishedSemanticFrame {
  SilentResponse?: TBoolSlot;
}

export interface TVideoPaymentConfirmedSemanticFrame {
  SilentResponse?: TBoolSlot;
}

export interface TSearchSemanticFrame {
  Query?: TStringSlot;
}

/** Deprecated */
export interface TIoTBroadcastStartSemanticFrame {
  PairingToken?: TStringSlot;
  TimeoutMs?: TUInt32Slot;
}

/** Deprecated */
export interface TIoTBroadcastSuccessSemanticFrame {
  DevicesID?: TStringSlot;
  ProductIDs?: TStringSlot;
}

/** Deprecated */
export interface TIoTBroadcastFailureSemanticFrame {
  TimeoutMs?: TUInt32Slot;
  Reason?: TStringSlot;
}

export interface TIoTDiscoveryStartSemanticFrame {
  SSID?: TStringSlot;
  Password?: TStringSlot;
  DeviceType?: TStringSlot;
  TimeoutMs?: TUInt32Slot;
}

export interface TIoTDiscoveryFailureSemanticFrame {
  TimeoutMs?: TUInt32Slot;
  Reason?: TStringSlot;
  DeviceType?: TStringSlot;
}

export interface TIoTDiscoverySuccessSemanticFrame {
  DeviceIDs?: TStringSlot;
  ProductIDs?: TStringSlot;
  DeviceType?: TStringSlot;
}

export interface TStartIotDiscoverySemanticFrame {
  Request?: TStartIotDiscoveryRequestSlot;
  SessionID?: TStringSlot;
}

export interface TFinishIotDiscoverySemanticFrame {
  Request?: TFinishIotDiscoveryRequestSlot;
}

export interface TFinishIotSystemDiscoverySemanticFrame {
  Request?: TFinishIotDiscoveryRequestSlot;
}

export interface TPutMoneyOnPhoneSemanticFrame {
  Amount?: TSysNumSlot;
  PhoneNumber?: TStringSlot;
}

export interface TStartIotTuyaBroadcastSemanticFrame {
  SSID?: TStringSlot;
  Password?: TStringSlot;
}

export interface TRestoreIotNetworksSemanticFrame {}

export interface TDeleteIotNetworksSemanticFrame {}

export interface TSaveIotNetworksSemanticFrame {
  Networks?: TIotNetworksSlot;
}

export interface TForgetIotEndpointsSemanticFrame {
  Request?: TForgetIotEndpointsRequestSlot;
}

export interface TIotYandexIOActionSemanticFrame {
  Request?: TIotYandexIOActionRequestSlot;
}

export interface TIotScenarioStepActionsSemanticFrame {
  LaunchID?: TStringSlot;
  StepIndex?: TUInt32Slot;
  DeviceActionsBatch?: TIoTDeviceActionsBatchSlot;
}

/** deprecated, use TEndpointEventsBatchSemanticFrame instead */
export interface TEndpointStateUpdatesSemanticFrame {
  Request?: TEndpointStateUpdatesRequestSlot;
}

/** deprecated, use TEndpointEventsBatchSemanticFrame instead */
export interface TCapabilityEventSemanticFrame {}

/** deprecated, use TEndpointEventsBatchSemanticFrame instead */
export interface TEndpointCapabilityEventsSemanticFrame {
  Events?: TEndpointCapabilityEventsSlot;
}

export interface TEndpointEventsBatchSemanticFrame {
  Batch?: TEndpointEventsBatchSlot;
}

export interface TMordoviaHomeScreenSemanticFrame {
  DeviceID?: TStringSlot;
}

export interface TNewsSemanticFrame {
  Topic?: TStringSlot;
  MaxCount?: TNumSlot;
  SkipIntroAndEnding?: TBoolSlot;
  Provider?: TNewsProviderSlot;
  Where?: TWhereSlot;
  DisableVoiceButtons?: TBoolSlot;
  GoBack?: TBoolSlot;
  NewsIdx?: TNumSlot;
  SingleNews?: TBoolSlot;
}

export interface TGetCallerNameSemanticFrame {
  CallerDeviceID?: TStringSlot;
  CallerPayload?: TStringSlot;
}

export interface TTokenTypeSlot {
  EnumValue: TTokenTypeSlot_EValue | undefined;
}

export enum TTokenTypeSlot_EValue {
  Unknown = 0,
  XToken = 1,
  OAuthToken = 2,
  UNRECOGNIZED = -1,
}

export function tTokenTypeSlot_EValueFromJSON(
  object: any
): TTokenTypeSlot_EValue {
  switch (object) {
    case 0:
    case "Unknown":
      return TTokenTypeSlot_EValue.Unknown;
    case 1:
    case "XToken":
      return TTokenTypeSlot_EValue.XToken;
    case 2:
    case "OAuthToken":
      return TTokenTypeSlot_EValue.OAuthToken;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TTokenTypeSlot_EValue.UNRECOGNIZED;
  }
}

export function tTokenTypeSlot_EValueToJSON(
  object: TTokenTypeSlot_EValue
): string {
  switch (object) {
    case TTokenTypeSlot_EValue.Unknown:
      return "Unknown";
    case TTokenTypeSlot_EValue.XToken:
      return "XToken";
    case TTokenTypeSlot_EValue.OAuthToken:
      return "OAuthToken";
    default:
      return "UNKNOWN";
  }
}

export interface TAddAccountSemanticFrame {
  /** @deprecated */
  JwtToken?: TStringSlot;
  TokenType?: TTokenTypeSlot;
  /** @deprecated */
  Encrypted?: TBoolSlot;
  EncryptedCode?: TStringSlot;
  Signature?: TStringSlot;
  EncryptedSessionKey?: TStringSlot;
}

export interface TRemoveAccountSemanticFrame {
  Puid?: TStringSlot;
}

export interface TEnrollmentStatusTypeSlot {
  EnrollmentStatus?: TEnrollmentStatus | undefined;
}

export interface TEnrollmentStatusSemanticFrame {
  /**
   * In multiaccount feature it is possible to have multiple users enrolled
   * Puid must explicitly identify user for whom enrollment has been completed or failed
   */
  Puid?: TStringSlot;
  Status?: TEnrollmentStatusTypeSlot;
}

export interface TCentaurCollectCardsSemanticFrame {
  CarouselId?: TStringSlot;
  IsScheduledUpdate?: TBoolSlot;
  IsGetSettingsRequest?: TBoolSlot;
}

export interface TWidgetPositionSlot {
  WidgetPositionValue?: TWidgetPosition | undefined;
}

export interface TCentaurCollectMainScreenSemanticFrame {
  WidgetGalleryPosition?: TWidgetPositionSlot;
  WidgetConfigDataSlot?: TWidgetConfigDataSlot;
  WidgetMainScreenPosition?: TWidgetPositionSlot;
}

export interface TCentaurCollectUpperShutterSemanticFrame {}

export interface TCentaurGetCardSemanticFrame {
  CarouselId?: TStringSlot;
}

/** @deprecated */
export interface TCentaurCollectWidgetGallerySemanticFrame {
  Column?: TNumSlot;
  Row?: TNumSlot;
}

/** @deprecated */
export interface TWidgetDataSlot {
  WidgetDataValue?: TCentaurWidgetData | undefined;
}

export interface TWidgetConfigDataSlot {
  WidgetConfigDataValue?: TCentaurWidgetConfigData | undefined;
}

export interface TCatalogTagSlot {
  CatalogTags?: TCatalogTags | undefined;
}

export interface TCentaurAddWidgetFromGallerySemanticFrame {
  Column?: TNumSlot;
  Row?: TNumSlot;
  /** @deprecated */
  WidgetDataSlot?: TWidgetDataSlot;
  WidgetConfigDataSlot?: TWidgetConfigDataSlot;
}

export interface TScenariosForTeasersSlot {
  TeaserSettingsData?: TTeaserSettingsData | undefined;
}

export interface TCentaurSetTeaserConfigurationSemanticFrame {
  ScenariosForTeasersSlot?: TScenariosForTeasersSlot;
}

export interface TCentaurCollectTeasersPreviewSemanticFrame {}

export interface TGetPhotoFrameSemanticFrame {
  /**
   * Need for pre combinator stage
   *
   * @deprecated
   */
  CarouselId?: TStringSlot;
  /**
   * Need for pre combinator stage
   *
   * @deprecated
   */
  PhotoId?: TNumSlot;
}

export interface TExternalAppSlot {
  ExternalAppValue: string | undefined;
}

export interface TOpenSmartDeviceExternalAppFrame {
  Application?: TExternalAppSlot;
}

export interface TMusicPlayObjectTypeSlot {
  EnumValue: TMusicPlayObjectTypeSlot_EValue | undefined;
}

export enum TMusicPlayObjectTypeSlot_EValue {
  Track = 0,
  Album = 1,
  Artist = 2,
  Playlist = 3,
  Radio = 4,
  Generative = 5,
  FmRadio = 6,
  UNRECOGNIZED = -1,
}

export function tMusicPlayObjectTypeSlot_EValueFromJSON(
  object: any
): TMusicPlayObjectTypeSlot_EValue {
  switch (object) {
    case 0:
    case "Track":
      return TMusicPlayObjectTypeSlot_EValue.Track;
    case 1:
    case "Album":
      return TMusicPlayObjectTypeSlot_EValue.Album;
    case 2:
    case "Artist":
      return TMusicPlayObjectTypeSlot_EValue.Artist;
    case 3:
    case "Playlist":
      return TMusicPlayObjectTypeSlot_EValue.Playlist;
    case 4:
    case "Radio":
      return TMusicPlayObjectTypeSlot_EValue.Radio;
    case 5:
    case "Generative":
      return TMusicPlayObjectTypeSlot_EValue.Generative;
    case 6:
    case "FmRadio":
      return TMusicPlayObjectTypeSlot_EValue.FmRadio;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TMusicPlayObjectTypeSlot_EValue.UNRECOGNIZED;
  }
}

export function tMusicPlayObjectTypeSlot_EValueToJSON(
  object: TMusicPlayObjectTypeSlot_EValue
): string {
  switch (object) {
    case TMusicPlayObjectTypeSlot_EValue.Track:
      return "Track";
    case TMusicPlayObjectTypeSlot_EValue.Album:
      return "Album";
    case TMusicPlayObjectTypeSlot_EValue.Artist:
      return "Artist";
    case TMusicPlayObjectTypeSlot_EValue.Playlist:
      return "Playlist";
    case TMusicPlayObjectTypeSlot_EValue.Radio:
      return "Radio";
    case TMusicPlayObjectTypeSlot_EValue.Generative:
      return "Generative";
    case TMusicPlayObjectTypeSlot_EValue.FmRadio:
      return "FmRadio";
    default:
      return "UNKNOWN";
  }
}

export interface TMusicPlaySemanticFrame {
  SpecialPlaylist?: TStringSlot;
  SpecialAnswerInfo?: TStringSlot;
  ActionRequest?: TStringSlot;
  Epoch?: TStringSlot;
  SearchText?: TStringSlot;
  Genre?: TMusicSlot;
  Mood?: TMusicSlot;
  Activity?: TMusicSlot;
  Language?: TLanguageSlot;
  Vocal?: TMusicSlot;
  Novelty?: TMusicSlot;
  Personality?: TMusicSlot;
  Order?: TMusicSlot;
  /** Repeate mode in accordance with https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/directives.proto?rev=r8999692#L579 */
  Repeat?: TMusicSlot;
  DisableAutoflow?: TBoolSlot;
  DisableNlg?: TBoolSlot;
  PlaySingleTrack?: TBoolSlot;
  TrackOffsetIndex?: TNumSlot;
  Playlist?: TStringSlot;
  /**
   * Fields for launching music similarly to music_play_object bass action
   * id from music catalog, for example ("38646012", "Track"), ("263065:3033", "Playlist")
   */
  ObjectId?: TStringSlot;
  ObjectType?: TMusicPlayObjectTypeSlot;
  /** Start album/playlist from track with id */
  StartFromTrackId?: TStringSlot;
  OffsetSec?: TDoubleSlot;
  AlarmId?: TStringSlot;
  From?: TStringSlot;
  /** When true, navigating back to tracks played before this frame returns Irrelevant */
  DisableHistory?: TBoolSlot;
  Room?: TMusicSlot;
  Stream?: TMusicSlot;
  GenerativeStation?: TMusicSlot;
  NeedSimilar?: TMusicSlot;
  Location?: TLocationSlot;
  /** Offset of the whole album/playlist: beginning, saved_progress */
  Offset?: TMusicSlot;
  /** Temporary hack to disable multiroom, until thin client multiroom is fully supported */
  DisableMultiroom?: TBoolSlot;
  ContentType?: TMusicContentTypeSlot;
}

export interface TMusicOnboardingSemanticFrame {}

export interface TMusicOnboardingArtistsSemanticFrame {}

export interface TMusicOnboardingGenresSemanticFrame {}

export interface TMusicOnboardingTracksSemanticFrame {}

export interface TMusicOnboardingTracksReaskSemanticFrame {
  /** @deprecated */
  TrackIndex?: TNumSlot;
  TrackId?: TStringSlot;
}

export interface TMusicAnnounceDisableSemanticFrame {}

export interface TMusicAnnounceEnableSemanticFrame {}

export interface TOpenTandemSettingSemanticFrame {}

export interface TOpenSmartSpeakerSettingSemanticFrame {}

export interface TExternalSkillActivateSemanticFrame {
  ActivationPhrase?: TStringSlot;
}

export interface TExternalSkillEpisodeForShowRequestSemanticFrame {
  SkillId?: TStringSlot;
}

export interface TMusicPlayFixlistSemanticFrame {
  SpecialAnswerInfo?: TFixlistInfoSlot;
}

export interface TMusicPlayAnaphoraSemanticFrame {
  ActionRequest?: TActionRequestSlot;
  Repeat?: TRepeatSlot;
  TargetType?: TTargetTypeSlot;
  NeedSimilar?: TNeedSimilarSlot;
  Order?: TOrderSlot;
}

export interface TMusicPlayFairytaleSemanticFrame {
  FairytaleTheme?: TFairytaleThemeSlot;
}

export interface TStartMultiroomSemanticFrame {
  LocationRoom: TLocationSlot[];
  LocationGroup: TLocationSlot[];
  LocationDevice: TLocationSlot[];
  LocationEverywhere?: TLocationSlot;
}

export interface TActivationTypedSemanticFrameSlot {
  PutMoneyOnPhoneSemanticFrame?: TPutMoneyOnPhoneSemanticFrame | undefined;
}

export interface TExternalSkillFixedActivateSemanticFrame {
  FixedSkillId?: TStringSlot;
  ActivationCommand?: TStringSlot;
  /** valid JSON that will be sent to skill */
  Payload?: TStringSlot;
  ActivationSourceType?: TActivationSourceTypeSlot;
  ActivationTypedSemanticFrame?: TActivationTypedSemanticFrameSlot;
}

export interface TActivationSourceTypeSlot {
  ActivationSourceType: string | undefined;
}

export interface TVideoPlaySemanticFrame {
  ContentType?: TStringSlot;
  Action?: TStringSlot;
  SearchText?: TStringSlot;
  Season?: TNumSlot;
  Episode?: TNumSlot;
  New?: TVideoSlot;
}

export interface TWeatherSemanticFrame {
  When?: TDateTimeSlot;
  CarouselId?: TStringSlot;
  Where?: TWhereSlot;
}

export interface THardcodedMorningShowSemanticFrame {
  Offset?: TNumSlot;
  ShowType?: TStringSlot;
  NewsProvider?: THardcodedShowSerializedSettingsSlot;
  Topic?: THardcodedShowSerializedSettingsSlot;
  NextTrackIndex?: TNumSlot;
}

export interface THardcodedShowSerializedSettingsSlot {
  SerializedData: string | undefined;
}

export interface TAliceShowActivateSemanticFrame {
  NewsProvider?: TNewsProviderSlot;
  Topic?: TTopicSlot;
  DayPart?: TDayPartSlot;
  Age?: TAgeSlot;
}

export interface TAppsFixlistSemanticFrame {
  AppData?: TAppData;
}

export interface TPlayerNextTrackSemanticFrame {
  SetPause?: TBoolSlot;
}

export interface TPlayerPrevTrackSemanticFrame {
  SetPause?: TBoolSlot;
}

export interface TPlayerLikeSemanticFrame {
  ContentId?: TContentIdSlot;
}

export interface TPlayerRemoveLikeSemanticFrame {
  ContentId?: TContentIdSlot;
}

export interface TPlayerDislikeSemanticFrame {
  ContentId?: TContentIdSlot;
}

export interface TPlayerRemoveDislikeSemanticFrame {
  ContentId?: TContentIdSlot;
}

export interface TPlayerContinueSemanticFrame {}

export interface TPlayerSongsByThisArtistSemanticFrame {}

export interface TPlayerWhatIsPlayingSemanticFrame {}

export interface TPlayerWhatIsThisSongAboutSemanticFrame {}

export interface TPlayerShuffleSemanticFrame {
  DisableNlg?: TBoolSlot;
}

export interface TPlayerUnshuffleSemanticFrame {
  DisableNlg?: TBoolSlot;
}

export interface TPlayerReplaySemanticFrame {}

export interface TPlayerPauseSemanticFrame {}

export interface TRemindersCancelSemanticFrame {
  /** A string of reminder guids divided by comma. */
  Ids?: TStringSlot;
}

export interface TRemindersListSemanticFrame {
  /**
   * This counter needs to prevent infinite ReRequest of reminders list. This can happened when we ask list of reminders
   * in SearchApp and the the device state has not been contains reminders state.
   */
  Countdown?: TUInt32Slot;
}

export interface TRemindersOnShootSemanticFrame {
  /** Reminder id is a guid. */
  Id?: TStringSlot;
  /** Reminder text which will be shown/told to user. */
  Text?: TStringSlot;
  /** Timestamp when reminder has been set. */
  Epoch?: TStringSlot;
  /** Original TimeZone in which reminder has been set. */
  TimeZone?: TStringSlot;
  /** Device Id on which reminder is set. */
  OriginDeviceId?: TStringSlot;
}

export interface TRewindTypeSlot {
  StringValue: string | undefined;
  RewindTypeValue: string | undefined;
}

export interface TPlayerRewindSemanticFrame {
  Time?: TUnitsTimeSlot;
  RewindType?: TRewindTypeSlot;
}

export interface TRepeatModeSlot {
  EnumValue: TRepeatModeSlot_EValue | undefined;
}

export enum TRepeatModeSlot_EValue {
  Unknown = 0,
  None = 1,
  One = 2,
  All = 3,
  UNRECOGNIZED = -1,
}

export function tRepeatModeSlot_EValueFromJSON(
  object: any
): TRepeatModeSlot_EValue {
  switch (object) {
    case 0:
    case "Unknown":
      return TRepeatModeSlot_EValue.Unknown;
    case 1:
    case "None":
      return TRepeatModeSlot_EValue.None;
    case 2:
    case "One":
      return TRepeatModeSlot_EValue.One;
    case 3:
    case "All":
      return TRepeatModeSlot_EValue.All;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TRepeatModeSlot_EValue.UNRECOGNIZED;
  }
}

export function tRepeatModeSlot_EValueToJSON(
  object: TRepeatModeSlot_EValue
): string {
  switch (object) {
    case TRepeatModeSlot_EValue.Unknown:
      return "Unknown";
    case TRepeatModeSlot_EValue.None:
      return "None";
    case TRepeatModeSlot_EValue.One:
      return "One";
    case TRepeatModeSlot_EValue.All:
      return "All";
    default:
      return "UNKNOWN";
  }
}

export interface TPlayerRepeatSemanticFrame {
  DisableNlg?: TBoolSlot;
  Mode?: TRepeatModeSlot;
}

export interface TDoNothingSemanticFrame {}

export interface TNotificationsSubscribeSemanticFrame {
  Accept?: TStringSlot;
  NotificationSubscription?: TNotificationSlot;
}

export interface TVideoRaterSemanticFrame {}

export interface TSetupRcuStatusSemanticFrame {
  Status?: TSetupRcuStatusSlot;
}

export interface TSetupRcuAutoStatusSemanticFrame {
  Status?: TSetupRcuStatusSlot;
}

export interface TSetupRcuCheckStatusSemanticFrame {
  Status?: TSetupRcuStatusSlot;
}

export interface TSetupRcuAdvancedStatusSemanticFrame {
  Status?: TSetupRcuStatusSlot;
}

export interface TSetupRcuManualStartSemanticFrame {}

export interface TSetupRcuAutoStartSemanticFrame {
  TvModel?: TStringSlot;
}

export interface TSetupRcuStatusSlot {
  EnumValue: TSetupRcuStatusSlot_EValue | undefined;
}

export enum TSetupRcuStatusSlot_EValue {
  Success = 0,
  Error = 1,
  InactiveTimeout = 2,
  UNRECOGNIZED = -1,
}

export function tSetupRcuStatusSlot_EValueFromJSON(
  object: any
): TSetupRcuStatusSlot_EValue {
  switch (object) {
    case 0:
    case "Success":
      return TSetupRcuStatusSlot_EValue.Success;
    case 1:
    case "Error":
      return TSetupRcuStatusSlot_EValue.Error;
    case 2:
    case "InactiveTimeout":
      return TSetupRcuStatusSlot_EValue.InactiveTimeout;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TSetupRcuStatusSlot_EValue.UNRECOGNIZED;
  }
}

export function tSetupRcuStatusSlot_EValueToJSON(
  object: TSetupRcuStatusSlot_EValue
): string {
  switch (object) {
    case TSetupRcuStatusSlot_EValue.Success:
      return "Success";
    case TSetupRcuStatusSlot_EValue.Error:
      return "Error";
    case TSetupRcuStatusSlot_EValue.InactiveTimeout:
      return "InactiveTimeout";
    default:
      return "UNKNOWN";
  }
}

export interface TLinkARemoteSemanticFrame {
  LinkType?: TStringSlot;
}

export interface TRequestTechnicalSupportSemanticFrame {}

export interface TOnboardingStartingCriticalUpdateSemanticFrame {
  IsFirstSetup?: TBoolSlot;
}

export interface TOnboardingStartingConfigureSuccessSemanticFrame {}

export interface TFmRadioStationSlot {
  FmRadioValue: string | undefined;
}

export interface TFmRadioFreqSlot {
  FmRadioFreqValue: string | undefined;
}

export interface TFmRadioInfoSlot {
  FmRadioInfoValue?: TFmRadioInfo | undefined;
}

export interface TRadioPlaySemanticFrame {
  FmRadioStation?: TFmRadioStationSlot;
  FmRadioFreq?: TFmRadioFreqSlot;
  FmRadioInfo?: TFmRadioInfoSlot;
  DisableNlg?: TBoolSlot;
}

export interface TFmRadioPlaySemanticFrame {
  FmRadioStation?: TFmRadioStationSlot;
  FmRadioFreq?: TFmRadioFreqSlot;
  DisableNlg?: TBoolSlot;
}

export interface TSoundLouderSemanticFrame {}

export interface TSoundQuiterSemanticFrame {}

export interface TSoundLevelSlot {
  NumLevelValue: number | undefined;
  FloatLevelValue: number | undefined;
  CustomLevelValue: string | undefined;
}

export interface TSoundSetLevelSemanticFrame {
  Level?: TSoundLevelSlot;
}

export interface TGetTimeSemanticFrame {
  Where?: TWhereSlot;
}

export interface THowToSubscribeSemanticFrame {}

export interface TGetSmartTvCategoriesSemanticFrame {}

export interface TGetSmartTvCarouselSemanticFrame {
  CarouselId?: TStringSlot;
  DocCacheHash?: TStringSlot;
  CarouselType?: TStringSlot;
  Filter?: TStringSlot;
  Tag?: TStringSlot;
  AvailableOnly?: TBoolSlot;
  MoreUrlLimit?: TNumSlot;
  Limit?: TNumSlot;
  Offset?: TNumSlot;
  KidMode?: TBoolSlot;
  RestrictionAge?: TNumSlot;
}

export interface TGetSmartTvCarouselsSemanticFrame {
  CategoryId?: TStringSlot;
  MaxItemsCount?: TNumSlot;
  CacheHash?: TStringSlot;
  PurchasesAvailableOnly?: TStringSlot;
  Limit?: TNumSlot;
  Offset?: TNumSlot;
  KidMode?: TBoolSlot;
  RestrictionAge?: TNumSlot;
  ExternalCarouselOffset?: TNumSlot;
}

export interface TIoTScenariosPhraseActionSemanticFrame {
  Phrase?: TStringSlot;
}

export interface TIoTScenariosTextActionSemanticFrame {
  Text?: TStringSlot;
}

export interface TIoTScenariosLaunchActionSemanticFrame {
  LaunchID?: TStringSlot;
  StepIndex?: TUInt32Slot;
  Instance?: TStringSlot;
  Value?: TStringSlot;
}

export interface TIoTCapabilityActionSlot {
  CapabilityActionValue?: TIoTCapabilityAction | undefined;
}

export interface TIoTScenarioSpeakerActionSemanticFrame {
  LaunchID?: TStringSlot;
  StepIndex?: TUInt32Slot;
  CapabilityAction?: TIoTCapabilityActionSlot;
}

export interface TAlarmSetAliceShowSemanticFrame {}

export interface TRepeatAfterMeSemanticFrame {
  Text?: TStringSlot;
  Voice?: TStringSlot;
}

export interface TMediaPlaySemanticFrame {
  TuneId?: TStringSlot;
  LocationId?: TStringSlot;
}

export interface TZenContextSearchStartSemanticFrame {}

export interface TTurnClockFaceOnSemanticFrame {}

export interface TTurnClockFaceOffSemanticFrame {}

export interface TIoTDeviceActionRequestSlot {
  RequestValue?: TIoTDeviceActionRequest | undefined;
}

export interface TIoTDeviceActionSemanticFrame {
  Request?: TIoTDeviceActionRequestSlot;
}

export interface TWhisperSaySomethingSemanticFrame {}

export interface TWhisperTurnOffSemanticFrame {}

export interface TWhisperTurnOnSemanticFrame {}

export interface TWhisperWhatIsItSemanticFrame {}

export interface TTimeCapsuleNextStepSemanticFrame {}

export interface TTimeCapsuleStopSemanticFrame {}

export interface TTimeCapsuleStartSemanticFrame {}

export interface TTimeCapsuleResumeSemanticFrame {}

export interface TTimeCapsuleSkipQuestionSemanticFrame {}

export interface TActivateGenerativeTaleSemanticFrame {
  Character?: TStringSlot;
}

export interface THardcodedResponseSemanticFrame {
  HardcodedResponseName?: THardcodedResponseName;
}

export interface TSkillSessionRequestSemanticFrame {}

export interface TUpdateContactsRequestSlot {
  RequestValue?: TUpdateContactsRequest | undefined;
}

export interface TUploadContactsRequestSemanticFrame {
  UploadRequest?: TUpdateContactsRequestSlot;
}

export interface TUpdateContactsRequestSemanticFrame {
  UpdateRequest?: TUpdateContactsRequestSlot;
}

export interface TExternalSkillForceDeactivateSemanticFrame {
  DialogId?: TStringSlot;
  SilentResponse?: TBoolSlot;
}

export interface TVideoCallProviderSlot {
  EnumValue: TVideoCallProviderSlot_EValue | undefined;
}

export enum TVideoCallProviderSlot_EValue {
  Unknown = 0,
  Telegram = 1,
  UNRECOGNIZED = -1,
}

export function tVideoCallProviderSlot_EValueFromJSON(
  object: any
): TVideoCallProviderSlot_EValue {
  switch (object) {
    case 0:
    case "Unknown":
      return TVideoCallProviderSlot_EValue.Unknown;
    case 1:
    case "Telegram":
      return TVideoCallProviderSlot_EValue.Telegram;
    case -1:
    case "UNRECOGNIZED":
    default:
      return TVideoCallProviderSlot_EValue.UNRECOGNIZED;
  }
}

export function tVideoCallProviderSlot_EValueToJSON(
  object: TVideoCallProviderSlot_EValue
): string {
  switch (object) {
    case TVideoCallProviderSlot_EValue.Unknown:
      return "Unknown";
    case TVideoCallProviderSlot_EValue.Telegram:
      return "Telegram";
    default:
      return "UNKNOWN";
  }
}

export interface TVideoCallLoginFailedSemanticFrame {
  Provider?: TVideoCallProviderSlot;
}

export interface TVideoCallOutgoingAcceptedSemanticFrame {
  Provider?: TVideoCallProviderSlot;
  CallId?: TStringSlot;
  UserId?: TStringSlot;
  Contact?: TProviderContactSlot;
}

export interface TVideoCallOutgoingFailedSemanticFrame {
  Provider?: TVideoCallProviderSlot;
}

export interface TVideoCallIncomingAcceptFailedSemanticFrame {
  Provider?: TVideoCallProviderSlot;
  CallId?: TStringSlot;
}

export interface TVideoCallItemNameSlot {
  ItemName: string | undefined;
}

export interface TPhoneCallSemanticFrame {
  ItemName?: TVideoCallItemNameSlot;
}

export interface TProviderContactSlot {
  ContactData?: TProviderContactData | undefined;
}

export interface TVideoCallToSemanticFrame {
  FixedContact?: TProviderContactSlot;
  VideoEnabled?: TBoolSlot;
}

export interface TOpenAddressBookSemanticFrame {}

export interface TGetEqualizerSettingsSemanticFrame {}

export interface TProviderContactListSlot {
  ContactList?: TProviderContactList | undefined;
}

export interface TVideoCallSetFavoritesSemanticFrame {
  UserId?: TStringSlot;
  Favorites?: TProviderContactListSlot;
}

export interface TVideoCallIncomingSemanticFrame {
  Provider?: TVideoCallProviderSlot;
  CallId?: TStringSlot;
  UserId?: TStringSlot;
  Caller?: TProviderContactSlot;
}

export interface TMessengerCallAcceptSemanticFrame {}

export interface TMessengerCallDiscardSemanticFrame {}

export interface TMessengerCallHangupSemanticFrame {}

export interface TTypedSemanticFrame {
  SearchSemanticFrame?: TSearchSemanticFrame | undefined;
  IoTBroadcastStartSemanticFrame?: TIoTBroadcastStartSemanticFrame | undefined;
  IoTBroadcastSuccessSemanticFrame?:
    | TIoTBroadcastSuccessSemanticFrame
    | undefined;
  IoTBroadcastFailureSemanticFrame?:
    | TIoTBroadcastFailureSemanticFrame
    | undefined;
  MordoviaHomeScreenSemanticFrame?:
    | TMordoviaHomeScreenSemanticFrame
    | undefined;
  NewsSemanticFrame?: TNewsSemanticFrame | undefined;
  GetCallerNameSemanticFrame?: TGetCallerNameSemanticFrame | undefined;
  MusicPlaySemanticFrame?: TMusicPlaySemanticFrame | undefined;
  ExternalSkillActivateSemanticFrame?:
    | TExternalSkillActivateSemanticFrame
    | undefined;
  VideoPlaySemanticFrame?: TVideoPlaySemanticFrame | undefined;
  WeatherSemanticFrame?: TWeatherSemanticFrame | undefined;
  HardcodedMorningShowSemanticFrame?:
    | THardcodedMorningShowSemanticFrame
    | undefined;
  SelectVideoFromGallerySemanticFrame?:
    | TSelectVideoFromGallerySemanticFrame
    | undefined;
  OpenCurrentVideoSemanticFrame?: TOpenCurrentVideoSemanticFrame | undefined;
  VideoPaymentConfirmedSemanticFrame?:
    | TVideoPaymentConfirmedSemanticFrame
    | undefined;
  PlayerNextTrackSemanticFrame?: TPlayerNextTrackSemanticFrame | undefined;
  PlayerPrevTrackSemanticFrame?: TPlayerPrevTrackSemanticFrame | undefined;
  PlayerLikeSemanticFrame?: TPlayerLikeSemanticFrame | undefined;
  PlayerDislikeSemanticFrame?: TPlayerDislikeSemanticFrame | undefined;
  DoNothingSemanticFrame?: TDoNothingSemanticFrame | undefined;
  NotificationsSubscribeSemanticFrame?:
    | TNotificationsSubscribeSemanticFrame
    | undefined;
  VideoRaterSemanticFrame?: TVideoRaterSemanticFrame | undefined;
  SetupRcuStatusSemanticFrame?: TSetupRcuStatusSemanticFrame | undefined;
  SetupRcuAutoStatusSemanticFrame?:
    | TSetupRcuAutoStatusSemanticFrame
    | undefined;
  SetupRcuCheckStatusSemanticFrame?:
    | TSetupRcuCheckStatusSemanticFrame
    | undefined;
  SetupRcuAdvancedStatusSemanticFrame?:
    | TSetupRcuAdvancedStatusSemanticFrame
    | undefined;
  SetupRcuManualStartSemanticFrame?:
    | TSetupRcuManualStartSemanticFrame
    | undefined;
  SetupRcuAutoStartSemanticFrame?: TSetupRcuAutoStartSemanticFrame | undefined;
  LinkARemoteSemanticFrame?: TLinkARemoteSemanticFrame | undefined;
  RequestTechnicalSupportSemanticFrame?:
    | TRequestTechnicalSupportSemanticFrame
    | undefined;
  IoTDiscoveryStartSemanticFrame?: TIoTDiscoveryStartSemanticFrame | undefined;
  IoTDiscoverySuccessSemanticFrame?:
    | TIoTDiscoverySuccessSemanticFrame
    | undefined;
  IoTDiscoveryFailureSemanticFrame?:
    | TIoTDiscoveryFailureSemanticFrame
    | undefined;
  ExternalSkillFixedActivateSemanticFrame?:
    | TExternalSkillFixedActivateSemanticFrame
    | undefined;
  OpenCurrentTrailerSemanticFrame?:
    | TOpenCurrentTrailerSemanticFrame
    | undefined;
  OnboardingStartingCriticalUpdateSemanticFrame?:
    | TOnboardingStartingCriticalUpdateSemanticFrame
    | undefined;
  OnboardingStartingConfigureSuccessSemanticFrame?:
    | TOnboardingStartingConfigureSuccessSemanticFrame
    | undefined;
  RadioPlaySemanticFrame?: TRadioPlaySemanticFrame | undefined;
  CentaurCollectCardsSemanticFrame?:
    | TCentaurCollectCardsSemanticFrame
    | undefined;
  CentaurGetCardSemanticFrame?: TCentaurGetCardSemanticFrame | undefined;
  VideoPlayerFinishedSemanticFrame?:
    | TVideoPlayerFinishedSemanticFrame
    | undefined;
  SoundLouderSemanticFrame?: TSoundLouderSemanticFrame | undefined;
  SoundQuiterSemanticFrame?: TSoundQuiterSemanticFrame | undefined;
  SoundSetLevelSemanticFrame?: TSoundSetLevelSemanticFrame | undefined;
  GetPhotoFrameSemanticFrame?: TGetPhotoFrameSemanticFrame | undefined;
  CentaurCollectMainScreenSemanticFrame?:
    | TCentaurCollectMainScreenSemanticFrame
    | undefined;
  GetTimeSemanticFrame?: TGetTimeSemanticFrame | undefined;
  HowToSubscribeSemanticFrame?: THowToSubscribeSemanticFrame | undefined;
  MusicOnboardingSemanticFrame?: TMusicOnboardingSemanticFrame | undefined;
  MusicOnboardingArtistsSemanticFrame?:
    | TMusicOnboardingArtistsSemanticFrame
    | undefined;
  MusicOnboardingGenresSemanticFrame?:
    | TMusicOnboardingGenresSemanticFrame
    | undefined;
  MusicOnboardingTracksSemanticFrame?:
    | TMusicOnboardingTracksSemanticFrame
    | undefined;
  PlayerContinueSemanticFrame?: TPlayerContinueSemanticFrame | undefined;
  PlayerWhatIsPlayingSemanticFrame?:
    | TPlayerWhatIsPlayingSemanticFrame
    | undefined;
  PlayerShuffleSemanticFrame?: TPlayerShuffleSemanticFrame | undefined;
  PlayerReplaySemanticFrame?: TPlayerReplaySemanticFrame | undefined;
  PlayerRewindSemanticFrame?: TPlayerRewindSemanticFrame | undefined;
  PlayerRepeatSemanticFrame?: TPlayerRepeatSemanticFrame | undefined;
  GetSmartTvCategoriesSemanticFrame?:
    | TGetSmartTvCategoriesSemanticFrame
    | undefined;
  IoTScenariosPhraseActionSemanticFrame?:
    | TIoTScenariosPhraseActionSemanticFrame
    | undefined;
  IoTScenariosTextActionSemanticFrame?:
    | TIoTScenariosTextActionSemanticFrame
    | undefined;
  IoTScenariosLaunchActionSemanticFrame?:
    | TIoTScenariosLaunchActionSemanticFrame
    | undefined;
  AlarmSetAliceShowSemanticFrame?: TAlarmSetAliceShowSemanticFrame | undefined;
  PlayerUnshuffleSemanticFrame?: TPlayerUnshuffleSemanticFrame | undefined;
  RemindersOnShootSemanticFrame?: TRemindersOnShootSemanticFrame | undefined;
  RepeatAfterMeSemanticFrame?: TRepeatAfterMeSemanticFrame | undefined;
  MediaPlaySemanticFrame?: TMediaPlaySemanticFrame | undefined;
  AliceShowActivateSemanticFrame?: TAliceShowActivateSemanticFrame | undefined;
  ZenContextSearchStartSemanticFrame?:
    | TZenContextSearchStartSemanticFrame
    | undefined;
  GetSmartTvCarouselSemanticFrame?:
    | TGetSmartTvCarouselSemanticFrame
    | undefined;
  GetSmartTvCarouselsSemanticFrame?:
    | TGetSmartTvCarouselsSemanticFrame
    | undefined;
  AppsFixlistSemanticFrame?: TAppsFixlistSemanticFrame | undefined;
  PlayerPauseSemanticFrame?: TPlayerPauseSemanticFrame | undefined;
  IoTScenarioSpeakerActionSemanticFrame?:
    | TIoTScenarioSpeakerActionSemanticFrame
    | undefined;
  VideoCardDetailSemanticFrame?: TVideoCardDetailSemanticFrame | undefined;
  TurnClockFaceOnSemanticFrame?: TTurnClockFaceOnSemanticFrame | undefined;
  TurnClockFaceOffSemanticFrame?: TTurnClockFaceOffSemanticFrame | undefined;
  RemindersOnCancelSemanticFrame?: TRemindersCancelSemanticFrame | undefined;
  IoTDeviceActionSemanticFrame?: TIoTDeviceActionSemanticFrame | undefined;
  VideoThinCardDetailSmanticFrame?:
    | TVideoThinCardDetailSemanticFrame
    | undefined;
  WhisperSaySomethingSemanticFrame?:
    | TWhisperSaySomethingSemanticFrame
    | undefined;
  WhisperTurnOffSemanticFrame?: TWhisperTurnOffSemanticFrame | undefined;
  WhisperTurnOnSemanticFrame?: TWhisperTurnOnSemanticFrame | undefined;
  WhisperWhatIsItSemanticFrame?: TWhisperWhatIsItSemanticFrame | undefined;
  TimeCapsuleNextStepSemanticFrame?:
    | TTimeCapsuleNextStepSemanticFrame
    | undefined;
  TimeCapsuleStopSemanticFrame?: TTimeCapsuleStopSemanticFrame | undefined;
  ActivateGenerativeTaleSemanticFrame?:
    | TActivateGenerativeTaleSemanticFrame
    | undefined;
  StartIotDiscoverySemanticFrame?: TStartIotDiscoverySemanticFrame | undefined;
  FinishIotDiscoverySemanticFrame?:
    | TFinishIotDiscoverySemanticFrame
    | undefined;
  ForgetIotEndpointsSemanticFrame?:
    | TForgetIotEndpointsSemanticFrame
    | undefined;
  HardcodedResponseSemanticFrame?: THardcodedResponseSemanticFrame | undefined;
  IotYandexIOActionSemanticFrame?: TIotYandexIOActionSemanticFrame | undefined;
  TimeCapsuleStartSemanticFrame?: TTimeCapsuleStartSemanticFrame | undefined;
  TimeCapsuleResumeSemanticFrame?: TTimeCapsuleResumeSemanticFrame | undefined;
  AddAccountSemanticFrame?: TAddAccountSemanticFrame | undefined;
  RemoveAccountSemanticFrame?: TRemoveAccountSemanticFrame | undefined;
  EndpointStateUpdatesSemanticFrame?:
    | TEndpointStateUpdatesSemanticFrame
    | undefined;
  TimeCapsuleSkipQuestionSemanticFrame?:
    | TTimeCapsuleSkipQuestionSemanticFrame
    | undefined;
  SkillSessionRequestSemanticFrame?:
    | TSkillSessionRequestSemanticFrame
    | undefined;
  StartIotTuyaBroadcastSemanticFrame?:
    | TStartIotTuyaBroadcastSemanticFrame
    | undefined;
  OpenSmartDeviceExternalAppFrame?:
    | TOpenSmartDeviceExternalAppFrame
    | undefined;
  RestoreIotNetworksSemanticFrame?:
    | TRestoreIotNetworksSemanticFrame
    | undefined;
  SaveIotNetworksSemanticFrame?: TSaveIotNetworksSemanticFrame | undefined;
  GuestEnrollmentStartSemanticFrame?:
    | TGuestEnrollmentStartSemanticFrame
    | undefined;
  TvPromoTemplateRequestSemanticFrame?:
    | TTvPromoTemplateRequestSemanticFrame
    | undefined;
  TvPromoTemplateShownReportSemanticFrame?:
    | TTvPromoTemplateShownReportSemanticFrame
    | undefined;
  UploadContactsRequestSemanticFrame?:
    | TUploadContactsRequestSemanticFrame
    | undefined;
  DeleteIotNetworksSemanticFrame?: TDeleteIotNetworksSemanticFrame | undefined;
  CentaurCollectWidgetGallerySemanticFrame?:
    | TCentaurCollectWidgetGallerySemanticFrame
    | undefined;
  CentaurAddWidgetFromGallerySemanticFrame?:
    | TCentaurAddWidgetFromGallerySemanticFrame
    | undefined;
  UpdateContactsRequestSemanticFrame?:
    | TUpdateContactsRequestSemanticFrame
    | undefined;
  RemindersListSemanticFrame?: TRemindersListSemanticFrame | undefined;
  CapabilityEventSemanticFrame?: TCapabilityEventSemanticFrame | undefined;
  ExternalSkillForceDeactivateSemanticFrame?:
    | TExternalSkillForceDeactivateSemanticFrame
    | undefined;
  GetVideoGalleries?: TGetVideoGalleriesSemanticFrame | undefined;
  EndpointCapabilityEventsSemanticFrame?:
    | TEndpointCapabilityEventsSemanticFrame
    | undefined;
  MusicAnnounceDisableSemanticFrame?:
    | TMusicAnnounceDisableSemanticFrame
    | undefined;
  MusicAnnounceEnableSemanticFrame?:
    | TMusicAnnounceEnableSemanticFrame
    | undefined;
  GetTvSearchResult?: TGetTvSearchResultSemanticFrame | undefined;
  SwitchTvChannelSemanticFrame?: TSwitchTvChannelSemanticFrame | undefined;
  ConvertSemanticFrame?: TConvertSemanticFrame | undefined;
  GetVideoGallerySemanticFrame?: TGetVideoGallerySemanticFrame | undefined;
  MusicOnboardingTracksReaskSemanticFrame?:
    | TMusicOnboardingTracksReaskSemanticFrame
    | undefined;
  TestSemanticFrame?: TTestSemanticFrame | undefined;
  EndpointEventsBatchSemanticFrame?:
    | TEndpointEventsBatchSemanticFrame
    | undefined;
  MediaSessionPlaySemanticFrame?: TMediaSessionPlaySemanticFrame | undefined;
  MediaSessionPauseSemanticFrame?: TMediaSessionPauseSemanticFrame | undefined;
  FmRadioPlaySemanticFrame?: TFmRadioPlaySemanticFrame | undefined;
  VideoCallLoginFailedSemanticFrame?:
    | TVideoCallLoginFailedSemanticFrame
    | undefined;
  VideoCallOutgoingAcceptedSemanticFrame?:
    | TVideoCallOutgoingAcceptedSemanticFrame
    | undefined;
  VideoCallOutgoingFailedSemanticFrame?:
    | TVideoCallOutgoingFailedSemanticFrame
    | undefined;
  VideoCallIncomingAcceptFailedSemanticFrame?:
    | TVideoCallIncomingAcceptFailedSemanticFrame
    | undefined;
  IotScenarioStepActionsSemanticFrame?:
    | TIotScenarioStepActionsSemanticFrame
    | undefined;
  PhoneCallSemanticFrame?: TPhoneCallSemanticFrame | undefined;
  OpenAddressBookSemanticFrame?: TOpenAddressBookSemanticFrame | undefined;
  GetEqualizerSettingsSemanticFrame?:
    | TGetEqualizerSettingsSemanticFrame
    | undefined;
  VideoCallToSemanticFrame?: TVideoCallToSemanticFrame | undefined;
  OnboardingGetGreetingsSemanticFrame?:
    | TOnboardingGetGreetingsSemanticFrame
    | undefined;
  OpenTandemSettingSemanticFrame?: TOpenTandemSettingSemanticFrame | undefined;
  OpenSmartSpeakerSettingSemanticFrame?:
    | TOpenSmartSpeakerSettingSemanticFrame
    | undefined;
  FinishIotSystemDiscoverySemanticFrame?:
    | TFinishIotSystemDiscoverySemanticFrame
    | undefined;
  VideoCallSetFavoritesSemanticFrame?:
    | TVideoCallSetFavoritesSemanticFrame
    | undefined;
  VideoCallIncomingSemanticFrame?: TVideoCallIncomingSemanticFrame | undefined;
  MessengerCallAcceptSemanticFrame?:
    | TMessengerCallAcceptSemanticFrame
    | undefined;
  MessengerCallDiscardSemanticFrame?:
    | TMessengerCallDiscardSemanticFrame
    | undefined;
  TvLongTapTutorialSemanticFrame?: TTvLongTapTutorialSemanticFrame | undefined;
  OnboardingWhatCanYouDoSemanticFrame?:
    | TOnboardingWhatCanYouDoSemanticFrame
    | undefined;
  MessengerCallHangupSemanticFrame?:
    | TMessengerCallHangupSemanticFrame
    | undefined;
  PutMoneyOnPhoneSemanticFrame?: TPutMoneyOnPhoneSemanticFrame | undefined;
  OrderNotificationSemanticFrame?: TOrderNotificationSemanticFrame | undefined;
  PlayerRemoveLikeSemanticFrame?: TPlayerRemoveLikeSemanticFrame | undefined;
  PlayerRemoveDislikeSemanticFrame?:
    | TPlayerRemoveDislikeSemanticFrame
    | undefined;
  CentaurCollectUpperShutterSemanticFrame?:
    | TCentaurCollectUpperShutterSemanticFrame
    | undefined;
  EnrollmentStatusSemanticFrame?: TEnrollmentStatusSemanticFrame | undefined;
  GalleryVideoSelectSemanticFrame?:
    | TGalleryVideoSelectSemanticFrame
    | undefined;
  PlayerSongsByThisArtistSemanticFrame?:
    | TPlayerSongsByThisArtistSemanticFrame
    | undefined;
  ExternalSkillEpisodeForShowRequestSemanticFrame?:
    | TExternalSkillEpisodeForShowRequestSemanticFrame
    | undefined;
  MusicPlayFixlistSemanticFrame?: TMusicPlayFixlistSemanticFrame | undefined;
  MusicPlayAnaphoraSemanticFrame?: TMusicPlayAnaphoraSemanticFrame | undefined;
  MusicPlayFairytaleSemanticFrame?:
    | TMusicPlayFairytaleSemanticFrame
    | undefined;
  StartMultiroomSemanticFrame?: TStartMultiroomSemanticFrame | undefined;
  PlayerWhatIsThisSongAboutSemanticFrame?:
    | TPlayerWhatIsThisSongAboutSemanticFrame
    | undefined;
  GuestEnrollmentFinishSemanticFrame?:
    | TGuestEnrollmentFinishSemanticFrame
    | undefined;
  CentaurSetTeaserConfigurationSemanticFrame?:
    | TCentaurSetTeaserConfigurationSemanticFrame
    | undefined;
  /** WARN: Don't forget to add some tests to "alice/library/typed_frame/typed_semantic_request_ut.cpp" when adding new frame. */
  CentaurCollectTeasersPreviewSemanticFrame?:
    | TCentaurCollectTeasersPreviewSemanticFrame
    | undefined;
}

export interface TSemanticFrame {
  Name: string;
  Slots: TSemanticFrame_TSlot[];
  /** repeated TClientEntity Entities = 4 [json_name = "entities", (NYT.column_name) = "entities", deprecated=true]; */
  TypedSemanticFrame?: TTypedSemanticFrame;
}

export interface TSemanticFrame_TSlot {
  Name: string;
  Type: string;
  Value: string;
  AcceptedTypes: string[];
  /** On request to scenario should be false. */
  IsRequested: boolean;
  /**
   * todo: rename after migration to Value
   * Please use Type and Value
   *
   * @deprecated
   */
  TypedValue?: TSlotValue;
  /** Shows whether the slot was filled on this request or not. */
  IsFilled: boolean;
}

/** Using as structure for @@mm_semantic_frame request */
export interface TSemanticFrameRequestData {
  TypedSemanticFrame?: TTypedSemanticFrame;
  Analytics?: TAnalyticsTrackingModule;
  /** sender identificators */
  Origin?: TOrigin;
  /**
   * TFrameRequestParams is deprecated. Use TRequestParams instead.
   *
   * @deprecated
   */
  Params?: TFrameRequestParams;
  RequestParams?: TRequestParams;
}

export interface TVideoCardDetailSemanticFrame {
  ContentId?: TStringSlot;
  ContentType?: TStringSlot;
  ContentOntoId?: TStringSlot;
}

export interface TVideoThinCardDetailSemanticFrame {
  ContentId?: TStringSlot;
}

export interface TGuestEnrollmentStartSemanticFrame {
  Puid?: TStringSlot;
}

export interface TGuestEnrollmentFinishSemanticFrame {}

export interface TTvPromoTemplateRequestSemanticFrame {
  ChosenTemplate?: TTvChosenTemplateSlot;
}

export interface TTvPromoTemplateShownReportSemanticFrame {
  ChosenTemplate?: TTvChosenTemplateSlot;
}

export interface TGetVideoGalleriesSemanticFrame {
  CategoryId?: TStringSlot;
  MaxItemsPerGallery?: TNumSlot;
  Offset?: TNumSlot;
  Limit?: TNumSlot;
  CacheHash?: TStringSlot;
  FromScreenId?: TStringSlot;
  ParentFromScreenId?: TStringSlot;
  KidModeEnabled?: TBoolSlot;
  RestrictionAge?: TStringSlot;
}

export interface TGetVideoGallerySemanticFrame {
  Id?: TStringSlot;
  Offset?: TNumSlot;
  Limit?: TNumSlot;
  CacheHash?: TStringSlot;
  FromScreenId?: TStringSlot;
  ParentFromScreenId?: TStringSlot;
  CarouselPosition?: TNumSlot;
  CarouselTitle?: TStringSlot;
  KidModeEnabled?: TBoolSlot;
  RestrictionAge?: TStringSlot;
  SelectedTags?: TCatalogTagSlot;
}

export interface TTestSemanticFrame {
  Dummy?: TStringSlot;
}

export interface TGetTvSearchResultSemanticFrame {
  SearchText?: TStringSlot;
  RestrictionMode?: TStringSlot;
  RestrictionAge?: TStringSlot;
  SearchEntref?: TStringSlot;
}

export interface TSwitchTvChannelSemanticFrame {
  Uri?: TStringSlot;
}

export interface TCurrencySlot {
  CurrencyValue: string | undefined;
}

export interface TConvertSemanticFrame {
  TypeFrom?: TCurrencySlot;
  TypeTo?: TCurrencySlot;
  AmountFrom?: TNumSlot;
}

export interface TMediaSessionPlaySemanticFrame {
  MediaSessionId?: TStringSlot;
}

export interface TMediaSessionPauseSemanticFrame {
  MediaSessionId?: TStringSlot;
}

export interface TOnboardingGetGreetingsSemanticFrame {}

export interface TOnboardingWhatCanYouDoSemanticFrame {
  PhraseIndex?: TUInt32Slot;
}

export interface TTvLongTapTutorialSemanticFrame {}

export interface TGalleryVideoSelectSemanticFrame {
  Action?: TStringSlot;
  ProviderItemId?: TStringSlot;
  EmbedUri?: TStringSlot;
}

function createBaseTNluPhrase(): TNluPhrase {
  return { Language: 0, Phrase: "" };
}

export const TNluPhrase = {
  encode(message: TNluPhrase, writer: Writer = Writer.create()): Writer {
    if (message.Language !== 0) {
      writer.uint32(8).int32(message.Language);
    }
    if (message.Phrase !== "") {
      writer.uint32(18).string(message.Phrase);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNluPhrase {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNluPhrase();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Language = reader.int32() as any;
          break;
        case 2:
          message.Phrase = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNluPhrase {
    return {
      Language: isSet(object.language) ? eLangFromJSON(object.language) : 0,
      Phrase: isSet(object.phrase) ? String(object.phrase) : "",
    };
  },

  toJSON(message: TNluPhrase): unknown {
    const obj: any = {};
    message.Language !== undefined &&
      (obj.language = eLangToJSON(message.Language));
    message.Phrase !== undefined && (obj.phrase = message.Phrase);
    return obj;
  },
};

function createBaseTNluHint(): TNluHint {
  return { Instances: [], Video: undefined, Negatives: [] };
}

export const TNluHint = {
  encode(message: TNluHint, writer: Writer = Writer.create()): Writer {
    for (const v of message.Instances) {
      TNluPhrase.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    if (message.Video !== undefined) {
      TVideoGalleryItemMeta.encode(
        message.Video,
        writer.uint32(26).fork()
      ).ldelim();
    }
    for (const v of message.Negatives) {
      TNluPhrase.encode(v!, writer.uint32(34).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNluHint {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNluHint();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          message.Instances.push(TNluPhrase.decode(reader, reader.uint32()));
          break;
        case 3:
          message.Video = TVideoGalleryItemMeta.decode(reader, reader.uint32());
          break;
        case 4:
          message.Negatives.push(TNluPhrase.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNluHint {
    return {
      Instances: Array.isArray(object?.instances)
        ? object.instances.map((e: any) => TNluPhrase.fromJSON(e))
        : [],
      Video: isSet(object.Video)
        ? TVideoGalleryItemMeta.fromJSON(object.Video)
        : undefined,
      Negatives: Array.isArray(object?.negatives)
        ? object.negatives.map((e: any) => TNluPhrase.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TNluHint): unknown {
    const obj: any = {};
    if (message.Instances) {
      obj.instances = message.Instances.map((e) =>
        e ? TNluPhrase.toJSON(e) : undefined
      );
    } else {
      obj.instances = [];
    }
    message.Video !== undefined &&
      (obj.Video = message.Video
        ? TVideoGalleryItemMeta.toJSON(message.Video)
        : undefined);
    if (message.Negatives) {
      obj.negatives = message.Negatives.map((e) =>
        e ? TNluPhrase.toJSON(e) : undefined
      );
    } else {
      obj.negatives = [];
    }
    return obj;
  },
};

function createBaseTFrameNluHint(): TFrameNluHint {
  return { FrameName: "", Instances: [], Negatives: [] };
}

export const TFrameNluHint = {
  encode(message: TFrameNluHint, writer: Writer = Writer.create()): Writer {
    if (message.FrameName !== "") {
      writer.uint32(10).string(message.FrameName);
    }
    for (const v of message.Instances) {
      TNluPhrase.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    for (const v of message.Negatives) {
      TNluPhrase.encode(v!, writer.uint32(34).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFrameNluHint {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFrameNluHint();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FrameName = reader.string();
          break;
        case 2:
          message.Instances.push(TNluPhrase.decode(reader, reader.uint32()));
          break;
        case 4:
          message.Negatives.push(TNluPhrase.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFrameNluHint {
    return {
      FrameName: isSet(object.frame_name) ? String(object.frame_name) : "",
      Instances: Array.isArray(object?.instances)
        ? object.instances.map((e: any) => TNluPhrase.fromJSON(e))
        : [],
      Negatives: Array.isArray(object?.negatives)
        ? object.negatives.map((e: any) => TNluPhrase.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TFrameNluHint): unknown {
    const obj: any = {};
    message.FrameName !== undefined && (obj.frame_name = message.FrameName);
    if (message.Instances) {
      obj.instances = message.Instances.map((e) =>
        e ? TNluPhrase.toJSON(e) : undefined
      );
    } else {
      obj.instances = [];
    }
    if (message.Negatives) {
      obj.negatives = message.Negatives.map((e) =>
        e ? TNluPhrase.toJSON(e) : undefined
      );
    } else {
      obj.negatives = [];
    }
    return obj;
  },
};

function createBaseTClientEntity(): TClientEntity {
  return { Name: "", Items: {} };
}

export const TClientEntity = {
  encode(message: TClientEntity, writer: Writer = Writer.create()): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    Object.entries(message.Items).forEach(([key, value]) => {
      TClientEntity_ItemsEntry.encode(
        { key: key as any, value },
        writer.uint32(18).fork()
      ).ldelim();
    });
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TClientEntity {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTClientEntity();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          const entry2 = TClientEntity_ItemsEntry.decode(
            reader,
            reader.uint32()
          );
          if (entry2.value !== undefined) {
            message.Items[entry2.key] = entry2.value;
          }
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TClientEntity {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Items: isObject(object.items)
        ? Object.entries(object.items).reduce<{ [key: string]: TNluHint }>(
            (acc, [key, value]) => {
              acc[key] = TNluHint.fromJSON(value);
              return acc;
            },
            {}
          )
        : {},
    };
  },

  toJSON(message: TClientEntity): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    obj.items = {};
    if (message.Items) {
      Object.entries(message.Items).forEach(([k, v]) => {
        obj.items[k] = TNluHint.toJSON(v);
      });
    }
    return obj;
  },
};

function createBaseTClientEntity_ItemsEntry(): TClientEntity_ItemsEntry {
  return { key: "", value: undefined };
}

export const TClientEntity_ItemsEntry = {
  encode(
    message: TClientEntity_ItemsEntry,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.key !== "") {
      writer.uint32(10).string(message.key);
    }
    if (message.value !== undefined) {
      TNluHint.encode(message.value, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TClientEntity_ItemsEntry {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTClientEntity_ItemsEntry();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.key = reader.string();
          break;
        case 2:
          message.value = TNluHint.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TClientEntity_ItemsEntry {
    return {
      key: isSet(object.key) ? String(object.key) : "",
      value: isSet(object.value) ? TNluHint.fromJSON(object.value) : undefined,
    };
  },

  toJSON(message: TClientEntity_ItemsEntry): unknown {
    const obj: any = {};
    message.key !== undefined && (obj.key = message.key);
    message.value !== undefined &&
      (obj.value = message.value ? TNluHint.toJSON(message.value) : undefined);
    return obj;
  },
};

function createBaseTClientEntityList(): TClientEntityList {
  return { Entities: [] };
}

export const TClientEntityList = {
  encode(message: TClientEntityList, writer: Writer = Writer.create()): Writer {
    for (const v of message.Entities) {
      TClientEntity.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TClientEntityList {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTClientEntityList();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Entities.push(TClientEntity.decode(reader, reader.uint32()));
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TClientEntityList {
    return {
      Entities: Array.isArray(object?.entities)
        ? object.entities.map((e: any) => TClientEntity.fromJSON(e))
        : [],
    };
  },

  toJSON(message: TClientEntityList): unknown {
    const obj: any = {};
    if (message.Entities) {
      obj.entities = message.Entities.map((e) =>
        e ? TClientEntity.toJSON(e) : undefined
      );
    } else {
      obj.entities = [];
    }
    return obj;
  },
};

function createBaseTSlotValue(): TSlotValue {
  return { Type: "", String: undefined };
}

export const TSlotValue = {
  encode(message: TSlotValue, writer: Writer = Writer.create()): Writer {
    if (message.Type !== "") {
      writer.uint32(10).string(message.Type);
    }
    if (message.String !== undefined) {
      writer.uint32(18).string(message.String);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSlotValue {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSlotValue();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Type = reader.string();
          break;
        case 2:
          message.String = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSlotValue {
    return {
      Type: isSet(object.type) ? String(object.type) : "",
      String: isSet(object.string) ? String(object.string) : undefined,
    };
  },

  toJSON(message: TSlotValue): unknown {
    const obj: any = {};
    message.Type !== undefined && (obj.type = message.Type);
    message.String !== undefined && (obj.string = message.String);
    return obj;
  },
};

function createBaseTStringSlot(): TStringSlot {
  return {
    StringValue: undefined,
    SpecialPlaylistValue: undefined,
    NewsTopicValue: undefined,
    SpecialAnswerInfoValue: undefined,
    VideoContentTypeValue: undefined,
    VideoActionValue: undefined,
    ActionRequestValue: undefined,
    EpochValue: undefined,
    VideoSelectionActionValue: undefined,
  };
}

export const TStringSlot = {
  encode(message: TStringSlot, writer: Writer = Writer.create()): Writer {
    if (message.StringValue !== undefined) {
      writer.uint32(10).string(message.StringValue);
    }
    if (message.SpecialPlaylistValue !== undefined) {
      writer.uint32(18).string(message.SpecialPlaylistValue);
    }
    if (message.NewsTopicValue !== undefined) {
      writer.uint32(26).string(message.NewsTopicValue);
    }
    if (message.SpecialAnswerInfoValue !== undefined) {
      writer.uint32(34).string(message.SpecialAnswerInfoValue);
    }
    if (message.VideoContentTypeValue !== undefined) {
      writer.uint32(42).string(message.VideoContentTypeValue);
    }
    if (message.VideoActionValue !== undefined) {
      writer.uint32(50).string(message.VideoActionValue);
    }
    if (message.ActionRequestValue !== undefined) {
      writer.uint32(66).string(message.ActionRequestValue);
    }
    if (message.EpochValue !== undefined) {
      writer.uint32(74).string(message.EpochValue);
    }
    if (message.VideoSelectionActionValue !== undefined) {
      writer.uint32(82).string(message.VideoSelectionActionValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TStringSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTStringSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.StringValue = reader.string();
          break;
        case 2:
          message.SpecialPlaylistValue = reader.string();
          break;
        case 3:
          message.NewsTopicValue = reader.string();
          break;
        case 4:
          message.SpecialAnswerInfoValue = reader.string();
          break;
        case 5:
          message.VideoContentTypeValue = reader.string();
          break;
        case 6:
          message.VideoActionValue = reader.string();
          break;
        case 8:
          message.ActionRequestValue = reader.string();
          break;
        case 9:
          message.EpochValue = reader.string();
          break;
        case 10:
          message.VideoSelectionActionValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TStringSlot {
    return {
      StringValue: isSet(object.string_value)
        ? String(object.string_value)
        : undefined,
      SpecialPlaylistValue: isSet(object.special_playlist_value)
        ? String(object.special_playlist_value)
        : undefined,
      NewsTopicValue: isSet(object.news_topic_value)
        ? String(object.news_topic_value)
        : undefined,
      SpecialAnswerInfoValue: isSet(object.special_answer_info)
        ? String(object.special_answer_info)
        : undefined,
      VideoContentTypeValue: isSet(object.video_content_type_value)
        ? String(object.video_content_type_value)
        : undefined,
      VideoActionValue: isSet(object.video_action_value)
        ? String(object.video_action_value)
        : undefined,
      ActionRequestValue: isSet(object.action_request_value)
        ? String(object.action_request_value)
        : undefined,
      EpochValue: isSet(object.epoch_value)
        ? String(object.epoch_value)
        : undefined,
      VideoSelectionActionValue: isSet(object.video_selection_action_value)
        ? String(object.video_selection_action_value)
        : undefined,
    };
  },

  toJSON(message: TStringSlot): unknown {
    const obj: any = {};
    message.StringValue !== undefined &&
      (obj.string_value = message.StringValue);
    message.SpecialPlaylistValue !== undefined &&
      (obj.special_playlist_value = message.SpecialPlaylistValue);
    message.NewsTopicValue !== undefined &&
      (obj.news_topic_value = message.NewsTopicValue);
    message.SpecialAnswerInfoValue !== undefined &&
      (obj.special_answer_info = message.SpecialAnswerInfoValue);
    message.VideoContentTypeValue !== undefined &&
      (obj.video_content_type_value = message.VideoContentTypeValue);
    message.VideoActionValue !== undefined &&
      (obj.video_action_value = message.VideoActionValue);
    message.ActionRequestValue !== undefined &&
      (obj.action_request_value = message.ActionRequestValue);
    message.EpochValue !== undefined && (obj.epoch_value = message.EpochValue);
    message.VideoSelectionActionValue !== undefined &&
      (obj.video_selection_action_value = message.VideoSelectionActionValue);
    return obj;
  },
};

function createBaseTUInt32Slot(): TUInt32Slot {
  return { UInt32Value: undefined };
}

export const TUInt32Slot = {
  encode(message: TUInt32Slot, writer: Writer = Writer.create()): Writer {
    if (message.UInt32Value !== undefined) {
      writer.uint32(8).uint32(message.UInt32Value);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUInt32Slot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUInt32Slot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UInt32Value = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUInt32Slot {
    return {
      UInt32Value: isSet(object.uint32_value)
        ? Number(object.uint32_value)
        : undefined,
    };
  },

  toJSON(message: TUInt32Slot): unknown {
    const obj: any = {};
    message.UInt32Value !== undefined &&
      (obj.uint32_value = Math.round(message.UInt32Value));
    return obj;
  },
};

function createBaseTNumSlot(): TNumSlot {
  return { NumValue: undefined };
}

export const TNumSlot = {
  encode(message: TNumSlot, writer: Writer = Writer.create()): Writer {
    if (message.NumValue !== undefined) {
      writer.uint32(8).uint32(message.NumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNumSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNumSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NumValue = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNumSlot {
    return {
      NumValue: isSet(object.num_value) ? Number(object.num_value) : undefined,
    };
  },

  toJSON(message: TNumSlot): unknown {
    const obj: any = {};
    message.NumValue !== undefined &&
      (obj.num_value = Math.round(message.NumValue));
    return obj;
  },
};

function createBaseTSysNumSlot(): TSysNumSlot {
  return { NumValue: undefined };
}

export const TSysNumSlot = {
  encode(message: TSysNumSlot, writer: Writer = Writer.create()): Writer {
    if (message.NumValue !== undefined) {
      writer.uint32(8).uint32(message.NumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSysNumSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSysNumSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NumValue = reader.uint32();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSysNumSlot {
    return {
      NumValue: isSet(object.num_value) ? Number(object.num_value) : undefined,
    };
  },

  toJSON(message: TSysNumSlot): unknown {
    const obj: any = {};
    message.NumValue !== undefined &&
      (obj.num_value = Math.round(message.NumValue));
    return obj;
  },
};

function createBaseTDoubleSlot(): TDoubleSlot {
  return { DoubleValue: undefined };
}

export const TDoubleSlot = {
  encode(message: TDoubleSlot, writer: Writer = Writer.create()): Writer {
    if (message.DoubleValue !== undefined) {
      writer.uint32(9).double(message.DoubleValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDoubleSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDoubleSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DoubleValue = reader.double();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDoubleSlot {
    return {
      DoubleValue: isSet(object.double_value)
        ? Number(object.double_value)
        : undefined,
    };
  },

  toJSON(message: TDoubleSlot): unknown {
    const obj: any = {};
    message.DoubleValue !== undefined &&
      (obj.double_value = message.DoubleValue);
    return obj;
  },
};

function createBaseTBoolSlot(): TBoolSlot {
  return { BoolValue: undefined };
}

export const TBoolSlot = {
  encode(message: TBoolSlot, writer: Writer = Writer.create()): Writer {
    if (message.BoolValue !== undefined) {
      writer.uint32(8).bool(message.BoolValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TBoolSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTBoolSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.BoolValue = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TBoolSlot {
    return {
      BoolValue: isSet(object.bool_value)
        ? Boolean(object.bool_value)
        : undefined,
    };
  },

  toJSON(message: TBoolSlot): unknown {
    const obj: any = {};
    message.BoolValue !== undefined && (obj.bool_value = message.BoolValue);
    return obj;
  },
};

function createBaseTNotificationSlot(): TNotificationSlot {
  return { SubscriptionValue: undefined };
}

export const TNotificationSlot = {
  encode(message: TNotificationSlot, writer: Writer = Writer.create()): Writer {
    if (message.SubscriptionValue !== undefined) {
      writer.uint32(10).string(message.SubscriptionValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNotificationSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNotificationSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SubscriptionValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNotificationSlot {
    return {
      SubscriptionValue: isSet(object.subscription_value)
        ? String(object.subscription_value)
        : undefined,
    };
  },

  toJSON(message: TNotificationSlot): unknown {
    const obj: any = {};
    message.SubscriptionValue !== undefined &&
      (obj.subscription_value = message.SubscriptionValue);
    return obj;
  },
};

function createBaseTMusicSlot(): TMusicSlot {
  return {
    GenreValue: undefined,
    MoodValue: undefined,
    ActivityValue: undefined,
    VocalValue: undefined,
    NoveltyValue: undefined,
    PersonalityValue: undefined,
    OrderValue: undefined,
    RepeatValue: undefined,
    RoomValue: undefined,
    StreamValue: undefined,
    GenerativeStationValue: undefined,
    NeedSimilarValue: undefined,
    OffsetValue: undefined,
  };
}

export const TMusicSlot = {
  encode(message: TMusicSlot, writer: Writer = Writer.create()): Writer {
    if (message.GenreValue !== undefined) {
      writer.uint32(10).string(message.GenreValue);
    }
    if (message.MoodValue !== undefined) {
      writer.uint32(18).string(message.MoodValue);
    }
    if (message.ActivityValue !== undefined) {
      writer.uint32(26).string(message.ActivityValue);
    }
    if (message.VocalValue !== undefined) {
      writer.uint32(34).string(message.VocalValue);
    }
    if (message.NoveltyValue !== undefined) {
      writer.uint32(42).string(message.NoveltyValue);
    }
    if (message.PersonalityValue !== undefined) {
      writer.uint32(50).string(message.PersonalityValue);
    }
    if (message.OrderValue !== undefined) {
      writer.uint32(58).string(message.OrderValue);
    }
    if (message.RepeatValue !== undefined) {
      writer.uint32(66).string(message.RepeatValue);
    }
    if (message.RoomValue !== undefined) {
      writer.uint32(74).string(message.RoomValue);
    }
    if (message.StreamValue !== undefined) {
      writer.uint32(82).string(message.StreamValue);
    }
    if (message.GenerativeStationValue !== undefined) {
      writer.uint32(90).string(message.GenerativeStationValue);
    }
    if (message.NeedSimilarValue !== undefined) {
      writer.uint32(98).string(message.NeedSimilarValue);
    }
    if (message.OffsetValue !== undefined) {
      writer.uint32(106).string(message.OffsetValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TMusicSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.GenreValue = reader.string();
          break;
        case 2:
          message.MoodValue = reader.string();
          break;
        case 3:
          message.ActivityValue = reader.string();
          break;
        case 4:
          message.VocalValue = reader.string();
          break;
        case 5:
          message.NoveltyValue = reader.string();
          break;
        case 6:
          message.PersonalityValue = reader.string();
          break;
        case 7:
          message.OrderValue = reader.string();
          break;
        case 8:
          message.RepeatValue = reader.string();
          break;
        case 9:
          message.RoomValue = reader.string();
          break;
        case 10:
          message.StreamValue = reader.string();
          break;
        case 11:
          message.GenerativeStationValue = reader.string();
          break;
        case 12:
          message.NeedSimilarValue = reader.string();
          break;
        case 13:
          message.OffsetValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMusicSlot {
    return {
      GenreValue: isSet(object.genre_value)
        ? String(object.genre_value)
        : undefined,
      MoodValue: isSet(object.mood_value)
        ? String(object.mood_value)
        : undefined,
      ActivityValue: isSet(object.activity_value)
        ? String(object.activity_value)
        : undefined,
      VocalValue: isSet(object.vocal_value)
        ? String(object.vocal_value)
        : undefined,
      NoveltyValue: isSet(object.novelty_value)
        ? String(object.novelty_value)
        : undefined,
      PersonalityValue: isSet(object.personality_value)
        ? String(object.personality_value)
        : undefined,
      OrderValue: isSet(object.order_value)
        ? String(object.order_value)
        : undefined,
      RepeatValue: isSet(object.repeat_value)
        ? String(object.repeat_value)
        : undefined,
      RoomValue: isSet(object.room_value)
        ? String(object.room_value)
        : undefined,
      StreamValue: isSet(object.stream_value)
        ? String(object.stream_value)
        : undefined,
      GenerativeStationValue: isSet(object.generative_station_value)
        ? String(object.generative_station_value)
        : undefined,
      NeedSimilarValue: isSet(object.need_similar_value)
        ? String(object.need_similar_value)
        : undefined,
      OffsetValue: isSet(object.offset_value)
        ? String(object.offset_value)
        : undefined,
    };
  },

  toJSON(message: TMusicSlot): unknown {
    const obj: any = {};
    message.GenreValue !== undefined && (obj.genre_value = message.GenreValue);
    message.MoodValue !== undefined && (obj.mood_value = message.MoodValue);
    message.ActivityValue !== undefined &&
      (obj.activity_value = message.ActivityValue);
    message.VocalValue !== undefined && (obj.vocal_value = message.VocalValue);
    message.NoveltyValue !== undefined &&
      (obj.novelty_value = message.NoveltyValue);
    message.PersonalityValue !== undefined &&
      (obj.personality_value = message.PersonalityValue);
    message.OrderValue !== undefined && (obj.order_value = message.OrderValue);
    message.RepeatValue !== undefined &&
      (obj.repeat_value = message.RepeatValue);
    message.RoomValue !== undefined && (obj.room_value = message.RoomValue);
    message.StreamValue !== undefined &&
      (obj.stream_value = message.StreamValue);
    message.GenerativeStationValue !== undefined &&
      (obj.generative_station_value = message.GenerativeStationValue);
    message.NeedSimilarValue !== undefined &&
      (obj.need_similar_value = message.NeedSimilarValue);
    message.OffsetValue !== undefined &&
      (obj.offset_value = message.OffsetValue);
    return obj;
  },
};

function createBaseTMusicContentTypeSlot(): TMusicContentTypeSlot {
  return { EnumValue: undefined };
}

export const TMusicContentTypeSlot = {
  encode(
    message: TMusicContentTypeSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EnumValue !== undefined) {
      writer.uint32(8).int32(message.EnumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TMusicContentTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicContentTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnumValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMusicContentTypeSlot {
    return {
      EnumValue: isSet(object.enum_value)
        ? tMusicContentTypeSlot_EValueFromJSON(object.enum_value)
        : undefined,
    };
  },

  toJSON(message: TMusicContentTypeSlot): unknown {
    const obj: any = {};
    message.EnumValue !== undefined &&
      (obj.enum_value =
        message.EnumValue !== undefined
          ? tMusicContentTypeSlot_EValueToJSON(message.EnumValue)
          : undefined);
    return obj;
  },
};

function createBaseTVideoSlot(): TVideoSlot {
  return { NewValue: undefined };
}

export const TVideoSlot = {
  encode(message: TVideoSlot, writer: Writer = Writer.create()): Writer {
    if (message.NewValue !== undefined) {
      writer.uint32(10).string(message.NewValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TVideoSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoSlot {
    return {
      NewValue: isSet(object.new_value) ? String(object.new_value) : undefined,
    };
  },

  toJSON(message: TVideoSlot): unknown {
    const obj: any = {};
    message.NewValue !== undefined && (obj.new_value = message.NewValue);
    return obj;
  },
};

function createBaseTLanguageSlot(): TLanguageSlot {
  return { LanguageValue: undefined };
}

export const TLanguageSlot = {
  encode(message: TLanguageSlot, writer: Writer = Writer.create()): Writer {
    if (message.LanguageValue !== undefined) {
      writer.uint32(10).string(message.LanguageValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TLanguageSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLanguageSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LanguageValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLanguageSlot {
    return {
      LanguageValue: isSet(object.language_value)
        ? String(object.language_value)
        : undefined,
    };
  },

  toJSON(message: TLanguageSlot): unknown {
    const obj: any = {};
    message.LanguageValue !== undefined &&
      (obj.language_value = message.LanguageValue);
    return obj;
  },
};

function createBaseTDateTimeSlot(): TDateTimeSlot {
  return { DateTimeValue: undefined };
}

export const TDateTimeSlot = {
  encode(message: TDateTimeSlot, writer: Writer = Writer.create()): Writer {
    if (message.DateTimeValue !== undefined) {
      writer.uint32(10).string(message.DateTimeValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDateTimeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDateTimeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DateTimeValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDateTimeSlot {
    return {
      DateTimeValue: isSet(object.datetime_value)
        ? String(object.datetime_value)
        : undefined,
    };
  },

  toJSON(message: TDateTimeSlot): unknown {
    const obj: any = {};
    message.DateTimeValue !== undefined &&
      (obj.datetime_value = message.DateTimeValue);
    return obj;
  },
};

function createBaseTUnitsTimeSlot(): TUnitsTimeSlot {
  return { StringValue: undefined, UnitsTimeValue: undefined };
}

export const TUnitsTimeSlot = {
  encode(message: TUnitsTimeSlot, writer: Writer = Writer.create()): Writer {
    if (message.StringValue !== undefined) {
      writer.uint32(10).string(message.StringValue);
    }
    if (message.UnitsTimeValue !== undefined) {
      writer.uint32(18).string(message.UnitsTimeValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TUnitsTimeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUnitsTimeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.StringValue = reader.string();
          break;
        case 2:
          message.UnitsTimeValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TUnitsTimeSlot {
    return {
      StringValue: isSet(object.string_value)
        ? String(object.string_value)
        : undefined,
      UnitsTimeValue: isSet(object.units_time_value)
        ? String(object.units_time_value)
        : undefined,
    };
  },

  toJSON(message: TUnitsTimeSlot): unknown {
    const obj: any = {};
    message.StringValue !== undefined &&
      (obj.string_value = message.StringValue);
    message.UnitsTimeValue !== undefined &&
      (obj.units_time_value = message.UnitsTimeValue);
    return obj;
  },
};

function createBaseTNewsProviderSlot(): TNewsProviderSlot {
  return { NewsProviderValue: undefined };
}

export const TNewsProviderSlot = {
  encode(message: TNewsProviderSlot, writer: Writer = Writer.create()): Writer {
    if (message.NewsProviderValue !== undefined) {
      TNewsProvider.encode(
        message.NewsProviderValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsProviderSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsProviderSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsProviderValue = TNewsProvider.decode(
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

  fromJSON(object: any): TNewsProviderSlot {
    return {
      NewsProviderValue: isSet(object.news_provider_value)
        ? TNewsProvider.fromJSON(object.news_provider_value)
        : undefined,
    };
  },

  toJSON(message: TNewsProviderSlot): unknown {
    const obj: any = {};
    message.NewsProviderValue !== undefined &&
      (obj.news_provider_value = message.NewsProviderValue
        ? TNewsProvider.toJSON(message.NewsProviderValue)
        : undefined);
    return obj;
  },
};

function createBaseTTopicSlot(): TTopicSlot {
  return { TopicValue: undefined };
}

export const TTopicSlot = {
  encode(message: TTopicSlot, writer: Writer = Writer.create()): Writer {
    if (message.TopicValue !== undefined) {
      TTopic.encode(message.TopicValue, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTopicSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTopicSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TopicValue = TTopic.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTopicSlot {
    return {
      TopicValue: isSet(object.topic_value)
        ? TTopic.fromJSON(object.topic_value)
        : undefined,
    };
  },

  toJSON(message: TTopicSlot): unknown {
    const obj: any = {};
    message.TopicValue !== undefined &&
      (obj.topic_value = message.TopicValue
        ? TTopic.toJSON(message.TopicValue)
        : undefined);
    return obj;
  },
};

function createBaseTDayPartSlot(): TDayPartSlot {
  return { DayPartValue: undefined };
}

export const TDayPartSlot = {
  encode(message: TDayPartSlot, writer: Writer = Writer.create()): Writer {
    if (message.DayPartValue !== undefined) {
      writer.uint32(8).int32(message.DayPartValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDayPartSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDayPartSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DayPartValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TDayPartSlot {
    return {
      DayPartValue: isSet(object.day_part_value)
        ? tDayPart_EValueFromJSON(object.day_part_value)
        : undefined,
    };
  },

  toJSON(message: TDayPartSlot): unknown {
    const obj: any = {};
    message.DayPartValue !== undefined &&
      (obj.day_part_value =
        message.DayPartValue !== undefined
          ? tDayPart_EValueToJSON(message.DayPartValue)
          : undefined);
    return obj;
  },
};

function createBaseTAgeSlot(): TAgeSlot {
  return { AgeValue: undefined };
}

export const TAgeSlot = {
  encode(message: TAgeSlot, writer: Writer = Writer.create()): Writer {
    if (message.AgeValue !== undefined) {
      writer.uint32(8).int32(message.AgeValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TAgeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAgeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AgeValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAgeSlot {
    return {
      AgeValue: isSet(object.age_value)
        ? tAge_EValueFromJSON(object.age_value)
        : undefined,
    };
  },

  toJSON(message: TAgeSlot): unknown {
    const obj: any = {};
    message.AgeValue !== undefined &&
      (obj.age_value =
        message.AgeValue !== undefined
          ? tAge_EValueToJSON(message.AgeValue)
          : undefined);
    return obj;
  },
};

function createBaseTOrderNotificationTypeSlot(): TOrderNotificationTypeSlot {
  return { OrderNotificationTypeValue: undefined };
}

export const TOrderNotificationTypeSlot = {
  encode(
    message: TOrderNotificationTypeSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.OrderNotificationTypeValue !== undefined) {
      writer.uint32(8).int32(message.OrderNotificationTypeValue);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOrderNotificationTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrderNotificationTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.OrderNotificationTypeValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrderNotificationTypeSlot {
    return {
      OrderNotificationTypeValue: isSet(object.order_notification_type_value)
        ? eNotificationTypeFromJSON(object.order_notification_type_value)
        : undefined,
    };
  },

  toJSON(message: TOrderNotificationTypeSlot): unknown {
    const obj: any = {};
    message.OrderNotificationTypeValue !== undefined &&
      (obj.order_notification_type_value =
        message.OrderNotificationTypeValue !== undefined
          ? eNotificationTypeToJSON(message.OrderNotificationTypeValue)
          : undefined);
    return obj;
  },
};

function createBaseTAppData(): TAppData {
  return { AppDataValue: undefined };
}

export const TAppData = {
  encode(message: TAppData, writer: Writer = Writer.create()): Writer {
    if (message.AppDataValue !== undefined) {
      writer.uint32(10).string(message.AppDataValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TAppData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAppData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AppDataValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAppData {
    return {
      AppDataValue: isSet(object.app_data_value)
        ? String(object.app_data_value)
        : undefined,
    };
  },

  toJSON(message: TAppData): unknown {
    const obj: any = {};
    message.AppDataValue !== undefined &&
      (obj.app_data_value = message.AppDataValue);
    return obj;
  },
};

function createBaseTHardcodedResponseName(): THardcodedResponseName {
  return { HardcodedResponseNameValue: undefined };
}

export const THardcodedResponseName = {
  encode(
    message: THardcodedResponseName,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.HardcodedResponseNameValue !== undefined) {
      writer.uint32(10).string(message.HardcodedResponseNameValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): THardcodedResponseName {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTHardcodedResponseName();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.HardcodedResponseNameValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): THardcodedResponseName {
    return {
      HardcodedResponseNameValue: isSet(object.hardcoded_response_value)
        ? String(object.hardcoded_response_value)
        : undefined,
    };
  },

  toJSON(message: THardcodedResponseName): unknown {
    const obj: any = {};
    message.HardcodedResponseNameValue !== undefined &&
      (obj.hardcoded_response_value = message.HardcodedResponseNameValue);
    return obj;
  },
};

function createBaseTIotNetworksSlot(): TIotNetworksSlot {
  return { NetworksValue: undefined };
}

export const TIotNetworksSlot = {
  encode(message: TIotNetworksSlot, writer: Writer = Writer.create()): Writer {
    if (message.NetworksValue !== undefined) {
      TIotDiscoveryCapability_TNetworks.encode(
        message.NetworksValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TIotNetworksSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotNetworksSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NetworksValue = TIotDiscoveryCapability_TNetworks.decode(
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

  fromJSON(object: any): TIotNetworksSlot {
    return {
      NetworksValue: isSet(object.networks_value)
        ? TIotDiscoveryCapability_TNetworks.fromJSON(object.networks_value)
        : undefined,
    };
  },

  toJSON(message: TIotNetworksSlot): unknown {
    const obj: any = {};
    message.NetworksValue !== undefined &&
      (obj.networks_value = message.NetworksValue
        ? TIotDiscoveryCapability_TNetworks.toJSON(message.NetworksValue)
        : undefined);
    return obj;
  },
};

function createBaseTWhereSlot(): TWhereSlot {
  return {
    WhereValue: undefined,
    SpecialLocationValue: undefined,
    LatLonValue: undefined,
  };
}

export const TWhereSlot = {
  encode(message: TWhereSlot, writer: Writer = Writer.create()): Writer {
    if (message.WhereValue !== undefined) {
      writer.uint32(10).string(message.WhereValue);
    }
    if (message.SpecialLocationValue !== undefined) {
      writer.uint32(18).string(message.SpecialLocationValue);
    }
    if (message.LatLonValue !== undefined) {
      TLatLon.encode(message.LatLonValue, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWhereSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWhereSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.WhereValue = reader.string();
          break;
        case 2:
          message.SpecialLocationValue = reader.string();
          break;
        case 3:
          message.LatLonValue = TLatLon.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWhereSlot {
    return {
      WhereValue: isSet(object.where_value)
        ? String(object.where_value)
        : undefined,
      SpecialLocationValue: isSet(object.special_location_value)
        ? String(object.special_location_value)
        : undefined,
      LatLonValue: isSet(object.lat_lon_value)
        ? TLatLon.fromJSON(object.lat_lon_value)
        : undefined,
    };
  },

  toJSON(message: TWhereSlot): unknown {
    const obj: any = {};
    message.WhereValue !== undefined && (obj.where_value = message.WhereValue);
    message.SpecialLocationValue !== undefined &&
      (obj.special_location_value = message.SpecialLocationValue);
    message.LatLonValue !== undefined &&
      (obj.lat_lon_value = message.LatLonValue
        ? TLatLon.toJSON(message.LatLonValue)
        : undefined);
    return obj;
  },
};

function createBaseTLocationSlot(): TLocationSlot {
  return {
    UserIotRoomValue: undefined,
    DeviceIotRoomValue: undefined,
    UserIotGroupValue: undefined,
    DeviceIotGroupValue: undefined,
    UserIotDeviceValue: undefined,
    DeviceIotDeviceValue: undefined,
    UserIotMultiroomAllDevicesValue: undefined,
    DeviceIotMultiroomAllDevicesValue: undefined,
  };
}

export const TLocationSlot = {
  encode(message: TLocationSlot, writer: Writer = Writer.create()): Writer {
    if (message.UserIotRoomValue !== undefined) {
      writer.uint32(10).string(message.UserIotRoomValue);
    }
    if (message.DeviceIotRoomValue !== undefined) {
      writer.uint32(18).string(message.DeviceIotRoomValue);
    }
    if (message.UserIotGroupValue !== undefined) {
      writer.uint32(26).string(message.UserIotGroupValue);
    }
    if (message.DeviceIotGroupValue !== undefined) {
      writer.uint32(34).string(message.DeviceIotGroupValue);
    }
    if (message.UserIotDeviceValue !== undefined) {
      writer.uint32(42).string(message.UserIotDeviceValue);
    }
    if (message.DeviceIotDeviceValue !== undefined) {
      writer.uint32(50).string(message.DeviceIotDeviceValue);
    }
    if (message.UserIotMultiroomAllDevicesValue !== undefined) {
      writer.uint32(58).string(message.UserIotMultiroomAllDevicesValue);
    }
    if (message.DeviceIotMultiroomAllDevicesValue !== undefined) {
      writer.uint32(66).string(message.DeviceIotMultiroomAllDevicesValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TLocationSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLocationSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserIotRoomValue = reader.string();
          break;
        case 2:
          message.DeviceIotRoomValue = reader.string();
          break;
        case 3:
          message.UserIotGroupValue = reader.string();
          break;
        case 4:
          message.DeviceIotGroupValue = reader.string();
          break;
        case 5:
          message.UserIotDeviceValue = reader.string();
          break;
        case 6:
          message.DeviceIotDeviceValue = reader.string();
          break;
        case 7:
          message.UserIotMultiroomAllDevicesValue = reader.string();
          break;
        case 8:
          message.DeviceIotMultiroomAllDevicesValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLocationSlot {
    return {
      UserIotRoomValue: isSet(object.user_iot_room_value)
        ? String(object.user_iot_room_value)
        : undefined,
      DeviceIotRoomValue: isSet(object.device_iot_room_value)
        ? String(object.device_iot_room_value)
        : undefined,
      UserIotGroupValue: isSet(object.user_iot_group_value)
        ? String(object.user_iot_group_value)
        : undefined,
      DeviceIotGroupValue: isSet(object.device_iot_group_value)
        ? String(object.device_iot_group_value)
        : undefined,
      UserIotDeviceValue: isSet(object.user_iot_device_value)
        ? String(object.user_iot_device_value)
        : undefined,
      DeviceIotDeviceValue: isSet(object.device_iot_device_value)
        ? String(object.device_iot_device_value)
        : undefined,
      UserIotMultiroomAllDevicesValue: isSet(
        object.user_iot_multiroom_all_devices_value
      )
        ? String(object.user_iot_multiroom_all_devices_value)
        : undefined,
      DeviceIotMultiroomAllDevicesValue: isSet(
        object.device_iot_multiroom_all_devices_value
      )
        ? String(object.device_iot_multiroom_all_devices_value)
        : undefined,
    };
  },

  toJSON(message: TLocationSlot): unknown {
    const obj: any = {};
    message.UserIotRoomValue !== undefined &&
      (obj.user_iot_room_value = message.UserIotRoomValue);
    message.DeviceIotRoomValue !== undefined &&
      (obj.device_iot_room_value = message.DeviceIotRoomValue);
    message.UserIotGroupValue !== undefined &&
      (obj.user_iot_group_value = message.UserIotGroupValue);
    message.DeviceIotGroupValue !== undefined &&
      (obj.device_iot_group_value = message.DeviceIotGroupValue);
    message.UserIotDeviceValue !== undefined &&
      (obj.user_iot_device_value = message.UserIotDeviceValue);
    message.DeviceIotDeviceValue !== undefined &&
      (obj.device_iot_device_value = message.DeviceIotDeviceValue);
    message.UserIotMultiroomAllDevicesValue !== undefined &&
      (obj.user_iot_multiroom_all_devices_value =
        message.UserIotMultiroomAllDevicesValue);
    message.DeviceIotMultiroomAllDevicesValue !== undefined &&
      (obj.device_iot_multiroom_all_devices_value =
        message.DeviceIotMultiroomAllDevicesValue);
    return obj;
  },
};

function createBaseTStartIotDiscoveryRequestSlot(): TStartIotDiscoveryRequestSlot {
  return { RequestValue: undefined };
}

export const TStartIotDiscoveryRequestSlot = {
  encode(
    message: TStartIotDiscoveryRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TStartIotDiscoveryRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TStartIotDiscoveryRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTStartIotDiscoveryRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TStartIotDiscoveryRequest.decode(
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

  fromJSON(object: any): TStartIotDiscoveryRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TStartIotDiscoveryRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TStartIotDiscoveryRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TStartIotDiscoveryRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTFinishIotDiscoveryRequestSlot(): TFinishIotDiscoveryRequestSlot {
  return { RequestValue: undefined };
}

export const TFinishIotDiscoveryRequestSlot = {
  encode(
    message: TFinishIotDiscoveryRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TFinishIotDiscoveryRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TFinishIotDiscoveryRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFinishIotDiscoveryRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TFinishIotDiscoveryRequest.decode(
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

  fromJSON(object: any): TFinishIotDiscoveryRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TFinishIotDiscoveryRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TFinishIotDiscoveryRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TFinishIotDiscoveryRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTForgetIotEndpointsRequestSlot(): TForgetIotEndpointsRequestSlot {
  return { RequestValue: undefined };
}

export const TForgetIotEndpointsRequestSlot = {
  encode(
    message: TForgetIotEndpointsRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TForgetIotEndpointsRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TForgetIotEndpointsRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTForgetIotEndpointsRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TForgetIotEndpointsRequest.decode(
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

  fromJSON(object: any): TForgetIotEndpointsRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TForgetIotEndpointsRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TForgetIotEndpointsRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TForgetIotEndpointsRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTEndpointStateUpdatesRequestSlot(): TEndpointStateUpdatesRequestSlot {
  return { RequestValue: undefined };
}

export const TEndpointStateUpdatesRequestSlot = {
  encode(
    message: TEndpointStateUpdatesRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TEndpointStateUpdatesRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointStateUpdatesRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointStateUpdatesRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TEndpointStateUpdatesRequest.decode(
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

  fromJSON(object: any): TEndpointStateUpdatesRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TEndpointStateUpdatesRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TEndpointStateUpdatesRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TEndpointStateUpdatesRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTEndpointCapabilityEventsSlot(): TEndpointCapabilityEventsSlot {
  return { EventsValue: undefined };
}

export const TEndpointCapabilityEventsSlot = {
  encode(
    message: TEndpointCapabilityEventsSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EventsValue !== undefined) {
      TEndpointCapabilityEvents.encode(
        message.EventsValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointCapabilityEventsSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointCapabilityEventsSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EventsValue = TEndpointCapabilityEvents.decode(
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

  fromJSON(object: any): TEndpointCapabilityEventsSlot {
    return {
      EventsValue: isSet(object.events_value)
        ? TEndpointCapabilityEvents.fromJSON(object.events_value)
        : undefined,
    };
  },

  toJSON(message: TEndpointCapabilityEventsSlot): unknown {
    const obj: any = {};
    message.EventsValue !== undefined &&
      (obj.events_value = message.EventsValue
        ? TEndpointCapabilityEvents.toJSON(message.EventsValue)
        : undefined);
    return obj;
  },
};

function createBaseTEndpointEventsBatchSlot(): TEndpointEventsBatchSlot {
  return { BatchValue: undefined };
}

export const TEndpointEventsBatchSlot = {
  encode(
    message: TEndpointEventsBatchSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.BatchValue !== undefined) {
      TEndpointEventsBatch.encode(
        message.BatchValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointEventsBatchSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointEventsBatchSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.BatchValue = TEndpointEventsBatch.decode(
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

  fromJSON(object: any): TEndpointEventsBatchSlot {
    return {
      BatchValue: isSet(object.batch_value)
        ? TEndpointEventsBatch.fromJSON(object.batch_value)
        : undefined,
    };
  },

  toJSON(message: TEndpointEventsBatchSlot): unknown {
    const obj: any = {};
    message.BatchValue !== undefined &&
      (obj.batch_value = message.BatchValue
        ? TEndpointEventsBatch.toJSON(message.BatchValue)
        : undefined);
    return obj;
  },
};

function createBaseTIotYandexIOActionRequestSlot(): TIotYandexIOActionRequestSlot {
  return { RequestValue: undefined };
}

export const TIotYandexIOActionRequestSlot = {
  encode(
    message: TIotYandexIOActionRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TIoTYandexIOActionRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotYandexIOActionRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotYandexIOActionRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TIoTYandexIOActionRequest.decode(
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

  fromJSON(object: any): TIotYandexIOActionRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TIoTYandexIOActionRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TIotYandexIOActionRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TIoTYandexIOActionRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTTvChosenTemplateSlot(): TTvChosenTemplateSlot {
  return { TandemTemplate: undefined };
}

export const TTvChosenTemplateSlot = {
  encode(
    message: TTvChosenTemplateSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TandemTemplate !== undefined) {
      TTandemTemplate.encode(
        message.TandemTemplate,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTvChosenTemplateSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTvChosenTemplateSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TandemTemplate = TTandemTemplate.decode(
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

  fromJSON(object: any): TTvChosenTemplateSlot {
    return {
      TandemTemplate: isSet(object.tandem_promo_template)
        ? TTandemTemplate.fromJSON(object.tandem_promo_template)
        : undefined,
    };
  },

  toJSON(message: TTvChosenTemplateSlot): unknown {
    const obj: any = {};
    message.TandemTemplate !== undefined &&
      (obj.tandem_promo_template = message.TandemTemplate
        ? TTandemTemplate.toJSON(message.TandemTemplate)
        : undefined);
    return obj;
  },
};

function createBaseTIoTDeviceActionsBatchSlot(): TIoTDeviceActionsBatchSlot {
  return { BatchValue: undefined };
}

export const TIoTDeviceActionsBatchSlot = {
  encode(
    message: TIoTDeviceActionsBatchSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.BatchValue !== undefined) {
      TIoTDeviceActionsBatch.encode(
        message.BatchValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTDeviceActionsBatchSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDeviceActionsBatchSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.BatchValue = TIoTDeviceActionsBatch.decode(
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

  fromJSON(object: any): TIoTDeviceActionsBatchSlot {
    return {
      BatchValue: isSet(object.batch_value)
        ? TIoTDeviceActionsBatch.fromJSON(object.batch_value)
        : undefined,
    };
  },

  toJSON(message: TIoTDeviceActionsBatchSlot): unknown {
    const obj: any = {};
    message.BatchValue !== undefined &&
      (obj.batch_value = message.BatchValue
        ? TIoTDeviceActionsBatch.toJSON(message.BatchValue)
        : undefined);
    return obj;
  },
};

function createBaseTContentIdSlot(): TContentIdSlot {
  return { ContentIdValue: undefined };
}

export const TContentIdSlot = {
  encode(message: TContentIdSlot, writer: Writer = Writer.create()): Writer {
    if (message.ContentIdValue !== undefined) {
      TContentId.encode(
        message.ContentIdValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TContentIdSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTContentIdSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentIdValue = TContentId.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TContentIdSlot {
    return {
      ContentIdValue: isSet(object.content_id_value)
        ? TContentId.fromJSON(object.content_id_value)
        : undefined,
    };
  },

  toJSON(message: TContentIdSlot): unknown {
    const obj: any = {};
    message.ContentIdValue !== undefined &&
      (obj.content_id_value = message.ContentIdValue
        ? TContentId.toJSON(message.ContentIdValue)
        : undefined);
    return obj;
  },
};

function createBaseTFixlistInfoSlot(): TFixlistInfoSlot {
  return { FixlistInfoValue: undefined };
}

export const TFixlistInfoSlot = {
  encode(message: TFixlistInfoSlot, writer: Writer = Writer.create()): Writer {
    if (message.FixlistInfoValue !== undefined) {
      writer.uint32(10).string(message.FixlistInfoValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFixlistInfoSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFixlistInfoSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FixlistInfoValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFixlistInfoSlot {
    return {
      FixlistInfoValue: isSet(object.fixlist_info_value)
        ? String(object.fixlist_info_value)
        : undefined,
    };
  },

  toJSON(message: TFixlistInfoSlot): unknown {
    const obj: any = {};
    message.FixlistInfoValue !== undefined &&
      (obj.fixlist_info_value = message.FixlistInfoValue);
    return obj;
  },
};

function createBaseTTargetTypeSlot(): TTargetTypeSlot {
  return { TargetTypeValue: undefined };
}

export const TTargetTypeSlot = {
  encode(message: TTargetTypeSlot, writer: Writer = Writer.create()): Writer {
    if (message.TargetTypeValue !== undefined) {
      writer.uint32(8).int32(message.TargetTypeValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTargetTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTargetTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TargetTypeValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTargetTypeSlot {
    return {
      TargetTypeValue: isSet(object.target_type_value)
        ? tTargetTypeSlot_EValueFromJSON(object.target_type_value)
        : undefined,
    };
  },

  toJSON(message: TTargetTypeSlot): unknown {
    const obj: any = {};
    message.TargetTypeValue !== undefined &&
      (obj.target_type_value =
        message.TargetTypeValue !== undefined
          ? tTargetTypeSlot_EValueToJSON(message.TargetTypeValue)
          : undefined);
    return obj;
  },
};

function createBaseTActionRequestSlot(): TActionRequestSlot {
  return { ActionRequestValue: undefined };
}

export const TActionRequestSlot = {
  encode(
    message: TActionRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ActionRequestValue !== undefined) {
      writer.uint32(8).int32(message.ActionRequestValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TActionRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTActionRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ActionRequestValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TActionRequestSlot {
    return {
      ActionRequestValue: isSet(object.action_request_value)
        ? tActionRequestSlot_EValueFromJSON(object.action_request_value)
        : undefined,
    };
  },

  toJSON(message: TActionRequestSlot): unknown {
    const obj: any = {};
    message.ActionRequestValue !== undefined &&
      (obj.action_request_value =
        message.ActionRequestValue !== undefined
          ? tActionRequestSlot_EValueToJSON(message.ActionRequestValue)
          : undefined);
    return obj;
  },
};

function createBaseTNeedSimilarSlot(): TNeedSimilarSlot {
  return { NeedSimilarValue: undefined };
}

export const TNeedSimilarSlot = {
  encode(message: TNeedSimilarSlot, writer: Writer = Writer.create()): Writer {
    if (message.NeedSimilarValue !== undefined) {
      writer.uint32(8).int32(message.NeedSimilarValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNeedSimilarSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNeedSimilarSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NeedSimilarValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNeedSimilarSlot {
    return {
      NeedSimilarValue: isSet(object.need_similar_value)
        ? tNeedSimilarSlot_EValueFromJSON(object.need_similar_value)
        : undefined,
    };
  },

  toJSON(message: TNeedSimilarSlot): unknown {
    const obj: any = {};
    message.NeedSimilarValue !== undefined &&
      (obj.need_similar_value =
        message.NeedSimilarValue !== undefined
          ? tNeedSimilarSlot_EValueToJSON(message.NeedSimilarValue)
          : undefined);
    return obj;
  },
};

function createBaseTRepeatSlot(): TRepeatSlot {
  return { RepeatValue: undefined };
}

export const TRepeatSlot = {
  encode(message: TRepeatSlot, writer: Writer = Writer.create()): Writer {
    if (message.RepeatValue !== undefined) {
      writer.uint32(8).int32(message.RepeatValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TRepeatSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRepeatSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RepeatValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRepeatSlot {
    return {
      RepeatValue: isSet(object.repeat_value)
        ? tRepeatSlot_EValueFromJSON(object.repeat_value)
        : undefined,
    };
  },

  toJSON(message: TRepeatSlot): unknown {
    const obj: any = {};
    message.RepeatValue !== undefined &&
      (obj.repeat_value =
        message.RepeatValue !== undefined
          ? tRepeatSlot_EValueToJSON(message.RepeatValue)
          : undefined);
    return obj;
  },
};

function createBaseTOrderSlot(): TOrderSlot {
  return { OrderValue: undefined };
}

export const TOrderSlot = {
  encode(message: TOrderSlot, writer: Writer = Writer.create()): Writer {
    if (message.OrderValue !== undefined) {
      writer.uint32(8).int32(message.OrderValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TOrderSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrderSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.OrderValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOrderSlot {
    return {
      OrderValue: isSet(object.order_value)
        ? tOrderSlot_EValueFromJSON(object.order_value)
        : undefined,
    };
  },

  toJSON(message: TOrderSlot): unknown {
    const obj: any = {};
    message.OrderValue !== undefined &&
      (obj.order_value =
        message.OrderValue !== undefined
          ? tOrderSlot_EValueToJSON(message.OrderValue)
          : undefined);
    return obj;
  },
};

function createBaseTFairytaleThemeSlot(): TFairytaleThemeSlot {
  return { FairytaleThemeValue: undefined };
}

export const TFairytaleThemeSlot = {
  encode(
    message: TFairytaleThemeSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FairytaleThemeValue !== undefined) {
      writer.uint32(8).int32(message.FairytaleThemeValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFairytaleThemeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFairytaleThemeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FairytaleThemeValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFairytaleThemeSlot {
    return {
      FairytaleThemeValue: isSet(object.fairytale_theme_value)
        ? tFairytaleThemeSlot_EValueFromJSON(object.fairytale_theme_value)
        : undefined,
    };
  },

  toJSON(message: TFairytaleThemeSlot): unknown {
    const obj: any = {};
    message.FairytaleThemeValue !== undefined &&
      (obj.fairytale_theme_value =
        message.FairytaleThemeValue !== undefined
          ? tFairytaleThemeSlot_EValueToJSON(message.FairytaleThemeValue)
          : undefined);
    return obj;
  },
};

function createBaseTSelectVideoFromGallerySemanticFrame(): TSelectVideoFromGallerySemanticFrame {
  return {
    Action: undefined,
    VideoIndex: undefined,
    SilentResponse: undefined,
  };
}

export const TSelectVideoFromGallerySemanticFrame = {
  encode(
    message: TSelectVideoFromGallerySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Action !== undefined) {
      TStringSlot.encode(message.Action, writer.uint32(10).fork()).ldelim();
    }
    if (message.VideoIndex !== undefined) {
      TNumSlot.encode(message.VideoIndex, writer.uint32(18).fork()).ldelim();
    }
    if (message.SilentResponse !== undefined) {
      TBoolSlot.encode(
        message.SilentResponse,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSelectVideoFromGallerySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSelectVideoFromGallerySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Action = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.VideoIndex = TNumSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.SilentResponse = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSelectVideoFromGallerySemanticFrame {
    return {
      Action: isSet(object.action)
        ? TStringSlot.fromJSON(object.action)
        : undefined,
      VideoIndex: isSet(object.video_index)
        ? TNumSlot.fromJSON(object.video_index)
        : undefined,
      SilentResponse: isSet(object.silent_response)
        ? TBoolSlot.fromJSON(object.silent_response)
        : undefined,
    };
  },

  toJSON(message: TSelectVideoFromGallerySemanticFrame): unknown {
    const obj: any = {};
    message.Action !== undefined &&
      (obj.action = message.Action
        ? TStringSlot.toJSON(message.Action)
        : undefined);
    message.VideoIndex !== undefined &&
      (obj.video_index = message.VideoIndex
        ? TNumSlot.toJSON(message.VideoIndex)
        : undefined);
    message.SilentResponse !== undefined &&
      (obj.silent_response = message.SilentResponse
        ? TBoolSlot.toJSON(message.SilentResponse)
        : undefined);
    return obj;
  },
};

function createBaseTOpenCurrentVideoSemanticFrame(): TOpenCurrentVideoSemanticFrame {
  return { Action: undefined, SilentResponse: undefined };
}

export const TOpenCurrentVideoSemanticFrame = {
  encode(
    message: TOpenCurrentVideoSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Action !== undefined) {
      TStringSlot.encode(message.Action, writer.uint32(10).fork()).ldelim();
    }
    if (message.SilentResponse !== undefined) {
      TBoolSlot.encode(
        message.SilentResponse,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOpenCurrentVideoSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOpenCurrentVideoSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Action = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.SilentResponse = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOpenCurrentVideoSemanticFrame {
    return {
      Action: isSet(object.action)
        ? TStringSlot.fromJSON(object.action)
        : undefined,
      SilentResponse: isSet(object.silent_response)
        ? TBoolSlot.fromJSON(object.silent_response)
        : undefined,
    };
  },

  toJSON(message: TOpenCurrentVideoSemanticFrame): unknown {
    const obj: any = {};
    message.Action !== undefined &&
      (obj.action = message.Action
        ? TStringSlot.toJSON(message.Action)
        : undefined);
    message.SilentResponse !== undefined &&
      (obj.silent_response = message.SilentResponse
        ? TBoolSlot.toJSON(message.SilentResponse)
        : undefined);
    return obj;
  },
};

function createBaseTOpenCurrentTrailerSemanticFrame(): TOpenCurrentTrailerSemanticFrame {
  return { SilentResponse: undefined };
}

export const TOpenCurrentTrailerSemanticFrame = {
  encode(
    message: TOpenCurrentTrailerSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SilentResponse !== undefined) {
      TBoolSlot.encode(
        message.SilentResponse,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOpenCurrentTrailerSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOpenCurrentTrailerSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SilentResponse = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOpenCurrentTrailerSemanticFrame {
    return {
      SilentResponse: isSet(object.silent_response)
        ? TBoolSlot.fromJSON(object.silent_response)
        : undefined,
    };
  },

  toJSON(message: TOpenCurrentTrailerSemanticFrame): unknown {
    const obj: any = {};
    message.SilentResponse !== undefined &&
      (obj.silent_response = message.SilentResponse
        ? TBoolSlot.toJSON(message.SilentResponse)
        : undefined);
    return obj;
  },
};

function createBaseTOrderNotificationSemanticFrame(): TOrderNotificationSemanticFrame {
  return {
    ProviderName: undefined,
    OrderId: undefined,
    OrderNotificationType: undefined,
  };
}

export const TOrderNotificationSemanticFrame = {
  encode(
    message: TOrderNotificationSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ProviderName !== undefined) {
      TStringSlot.encode(
        message.ProviderName,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.OrderId !== undefined) {
      TStringSlot.encode(message.OrderId, writer.uint32(18).fork()).ldelim();
    }
    if (message.OrderNotificationType !== undefined) {
      TOrderNotificationTypeSlot.encode(
        message.OrderNotificationType,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOrderNotificationSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOrderNotificationSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ProviderName = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.OrderId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.OrderNotificationType = TOrderNotificationTypeSlot.decode(
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

  fromJSON(object: any): TOrderNotificationSemanticFrame {
    return {
      ProviderName: isSet(object.provider_name)
        ? TStringSlot.fromJSON(object.provider_name)
        : undefined,
      OrderId: isSet(object.order_id)
        ? TStringSlot.fromJSON(object.order_id)
        : undefined,
      OrderNotificationType: isSet(object.order_notification_type)
        ? TOrderNotificationTypeSlot.fromJSON(object.order_notification_type)
        : undefined,
    };
  },

  toJSON(message: TOrderNotificationSemanticFrame): unknown {
    const obj: any = {};
    message.ProviderName !== undefined &&
      (obj.provider_name = message.ProviderName
        ? TStringSlot.toJSON(message.ProviderName)
        : undefined);
    message.OrderId !== undefined &&
      (obj.order_id = message.OrderId
        ? TStringSlot.toJSON(message.OrderId)
        : undefined);
    message.OrderNotificationType !== undefined &&
      (obj.order_notification_type = message.OrderNotificationType
        ? TOrderNotificationTypeSlot.toJSON(message.OrderNotificationType)
        : undefined);
    return obj;
  },
};

function createBaseTVideoPlayerFinishedSemanticFrame(): TVideoPlayerFinishedSemanticFrame {
  return { SilentResponse: undefined };
}

export const TVideoPlayerFinishedSemanticFrame = {
  encode(
    message: TVideoPlayerFinishedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SilentResponse !== undefined) {
      TBoolSlot.encode(
        message.SilentResponse,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoPlayerFinishedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoPlayerFinishedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SilentResponse = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoPlayerFinishedSemanticFrame {
    return {
      SilentResponse: isSet(object.silent_response)
        ? TBoolSlot.fromJSON(object.silent_response)
        : undefined,
    };
  },

  toJSON(message: TVideoPlayerFinishedSemanticFrame): unknown {
    const obj: any = {};
    message.SilentResponse !== undefined &&
      (obj.silent_response = message.SilentResponse
        ? TBoolSlot.toJSON(message.SilentResponse)
        : undefined);
    return obj;
  },
};

function createBaseTVideoPaymentConfirmedSemanticFrame(): TVideoPaymentConfirmedSemanticFrame {
  return { SilentResponse: undefined };
}

export const TVideoPaymentConfirmedSemanticFrame = {
  encode(
    message: TVideoPaymentConfirmedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SilentResponse !== undefined) {
      TBoolSlot.encode(
        message.SilentResponse,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoPaymentConfirmedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoPaymentConfirmedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SilentResponse = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoPaymentConfirmedSemanticFrame {
    return {
      SilentResponse: isSet(object.silent_response)
        ? TBoolSlot.fromJSON(object.silent_response)
        : undefined,
    };
  },

  toJSON(message: TVideoPaymentConfirmedSemanticFrame): unknown {
    const obj: any = {};
    message.SilentResponse !== undefined &&
      (obj.silent_response = message.SilentResponse
        ? TBoolSlot.toJSON(message.SilentResponse)
        : undefined);
    return obj;
  },
};

function createBaseTSearchSemanticFrame(): TSearchSemanticFrame {
  return { Query: undefined };
}

export const TSearchSemanticFrame = {
  encode(
    message: TSearchSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Query !== undefined) {
      TStringSlot.encode(message.Query, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSearchSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSearchSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Query = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSearchSemanticFrame {
    return {
      Query: isSet(object.query)
        ? TStringSlot.fromJSON(object.query)
        : undefined,
    };
  },

  toJSON(message: TSearchSemanticFrame): unknown {
    const obj: any = {};
    message.Query !== undefined &&
      (obj.query = message.Query
        ? TStringSlot.toJSON(message.Query)
        : undefined);
    return obj;
  },
};

function createBaseTIoTBroadcastStartSemanticFrame(): TIoTBroadcastStartSemanticFrame {
  return { PairingToken: undefined, TimeoutMs: undefined };
}

export const TIoTBroadcastStartSemanticFrame = {
  encode(
    message: TIoTBroadcastStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.PairingToken !== undefined) {
      TStringSlot.encode(
        message.PairingToken,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.TimeoutMs !== undefined) {
      TUInt32Slot.encode(message.TimeoutMs, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTBroadcastStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTBroadcastStartSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.PairingToken = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.TimeoutMs = TUInt32Slot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTBroadcastStartSemanticFrame {
    return {
      PairingToken: isSet(object.pairing_token)
        ? TStringSlot.fromJSON(object.pairing_token)
        : undefined,
      TimeoutMs: isSet(object.timeout_ms)
        ? TUInt32Slot.fromJSON(object.timeout_ms)
        : undefined,
    };
  },

  toJSON(message: TIoTBroadcastStartSemanticFrame): unknown {
    const obj: any = {};
    message.PairingToken !== undefined &&
      (obj.pairing_token = message.PairingToken
        ? TStringSlot.toJSON(message.PairingToken)
        : undefined);
    message.TimeoutMs !== undefined &&
      (obj.timeout_ms = message.TimeoutMs
        ? TUInt32Slot.toJSON(message.TimeoutMs)
        : undefined);
    return obj;
  },
};

function createBaseTIoTBroadcastSuccessSemanticFrame(): TIoTBroadcastSuccessSemanticFrame {
  return { DevicesID: undefined, ProductIDs: undefined };
}

export const TIoTBroadcastSuccessSemanticFrame = {
  encode(
    message: TIoTBroadcastSuccessSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DevicesID !== undefined) {
      TStringSlot.encode(message.DevicesID, writer.uint32(10).fork()).ldelim();
    }
    if (message.ProductIDs !== undefined) {
      TStringSlot.encode(message.ProductIDs, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTBroadcastSuccessSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTBroadcastSuccessSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DevicesID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.ProductIDs = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTBroadcastSuccessSemanticFrame {
    return {
      DevicesID: isSet(object.devices_id)
        ? TStringSlot.fromJSON(object.devices_id)
        : undefined,
      ProductIDs: isSet(object.product_ids)
        ? TStringSlot.fromJSON(object.product_ids)
        : undefined,
    };
  },

  toJSON(message: TIoTBroadcastSuccessSemanticFrame): unknown {
    const obj: any = {};
    message.DevicesID !== undefined &&
      (obj.devices_id = message.DevicesID
        ? TStringSlot.toJSON(message.DevicesID)
        : undefined);
    message.ProductIDs !== undefined &&
      (obj.product_ids = message.ProductIDs
        ? TStringSlot.toJSON(message.ProductIDs)
        : undefined);
    return obj;
  },
};

function createBaseTIoTBroadcastFailureSemanticFrame(): TIoTBroadcastFailureSemanticFrame {
  return { TimeoutMs: undefined, Reason: undefined };
}

export const TIoTBroadcastFailureSemanticFrame = {
  encode(
    message: TIoTBroadcastFailureSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TimeoutMs !== undefined) {
      TUInt32Slot.encode(message.TimeoutMs, writer.uint32(18).fork()).ldelim();
    }
    if (message.Reason !== undefined) {
      TStringSlot.encode(message.Reason, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTBroadcastFailureSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTBroadcastFailureSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          message.TimeoutMs = TUInt32Slot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Reason = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTBroadcastFailureSemanticFrame {
    return {
      TimeoutMs: isSet(object.timeout_ms)
        ? TUInt32Slot.fromJSON(object.timeout_ms)
        : undefined,
      Reason: isSet(object.reason)
        ? TStringSlot.fromJSON(object.reason)
        : undefined,
    };
  },

  toJSON(message: TIoTBroadcastFailureSemanticFrame): unknown {
    const obj: any = {};
    message.TimeoutMs !== undefined &&
      (obj.timeout_ms = message.TimeoutMs
        ? TUInt32Slot.toJSON(message.TimeoutMs)
        : undefined);
    message.Reason !== undefined &&
      (obj.reason = message.Reason
        ? TStringSlot.toJSON(message.Reason)
        : undefined);
    return obj;
  },
};

function createBaseTIoTDiscoveryStartSemanticFrame(): TIoTDiscoveryStartSemanticFrame {
  return {
    SSID: undefined,
    Password: undefined,
    DeviceType: undefined,
    TimeoutMs: undefined,
  };
}

export const TIoTDiscoveryStartSemanticFrame = {
  encode(
    message: TIoTDiscoveryStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SSID !== undefined) {
      TStringSlot.encode(message.SSID, writer.uint32(10).fork()).ldelim();
    }
    if (message.Password !== undefined) {
      TStringSlot.encode(message.Password, writer.uint32(18).fork()).ldelim();
    }
    if (message.DeviceType !== undefined) {
      TStringSlot.encode(message.DeviceType, writer.uint32(26).fork()).ldelim();
    }
    if (message.TimeoutMs !== undefined) {
      TUInt32Slot.encode(message.TimeoutMs, writer.uint32(34).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTDiscoveryStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDiscoveryStartSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SSID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Password = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.DeviceType = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.TimeoutMs = TUInt32Slot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTDiscoveryStartSemanticFrame {
    return {
      SSID: isSet(object.ssid) ? TStringSlot.fromJSON(object.ssid) : undefined,
      Password: isSet(object.password)
        ? TStringSlot.fromJSON(object.password)
        : undefined,
      DeviceType: isSet(object.device_type)
        ? TStringSlot.fromJSON(object.device_type)
        : undefined,
      TimeoutMs: isSet(object.timeout_ms)
        ? TUInt32Slot.fromJSON(object.timeout_ms)
        : undefined,
    };
  },

  toJSON(message: TIoTDiscoveryStartSemanticFrame): unknown {
    const obj: any = {};
    message.SSID !== undefined &&
      (obj.ssid = message.SSID ? TStringSlot.toJSON(message.SSID) : undefined);
    message.Password !== undefined &&
      (obj.password = message.Password
        ? TStringSlot.toJSON(message.Password)
        : undefined);
    message.DeviceType !== undefined &&
      (obj.device_type = message.DeviceType
        ? TStringSlot.toJSON(message.DeviceType)
        : undefined);
    message.TimeoutMs !== undefined &&
      (obj.timeout_ms = message.TimeoutMs
        ? TUInt32Slot.toJSON(message.TimeoutMs)
        : undefined);
    return obj;
  },
};

function createBaseTIoTDiscoveryFailureSemanticFrame(): TIoTDiscoveryFailureSemanticFrame {
  return { TimeoutMs: undefined, Reason: undefined, DeviceType: undefined };
}

export const TIoTDiscoveryFailureSemanticFrame = {
  encode(
    message: TIoTDiscoveryFailureSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TimeoutMs !== undefined) {
      TUInt32Slot.encode(message.TimeoutMs, writer.uint32(10).fork()).ldelim();
    }
    if (message.Reason !== undefined) {
      TStringSlot.encode(message.Reason, writer.uint32(18).fork()).ldelim();
    }
    if (message.DeviceType !== undefined) {
      TStringSlot.encode(message.DeviceType, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTDiscoveryFailureSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDiscoveryFailureSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TimeoutMs = TUInt32Slot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Reason = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.DeviceType = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTDiscoveryFailureSemanticFrame {
    return {
      TimeoutMs: isSet(object.timeout_ms)
        ? TUInt32Slot.fromJSON(object.timeout_ms)
        : undefined,
      Reason: isSet(object.reason)
        ? TStringSlot.fromJSON(object.reason)
        : undefined,
      DeviceType: isSet(object.device_type)
        ? TStringSlot.fromJSON(object.device_type)
        : undefined,
    };
  },

  toJSON(message: TIoTDiscoveryFailureSemanticFrame): unknown {
    const obj: any = {};
    message.TimeoutMs !== undefined &&
      (obj.timeout_ms = message.TimeoutMs
        ? TUInt32Slot.toJSON(message.TimeoutMs)
        : undefined);
    message.Reason !== undefined &&
      (obj.reason = message.Reason
        ? TStringSlot.toJSON(message.Reason)
        : undefined);
    message.DeviceType !== undefined &&
      (obj.device_type = message.DeviceType
        ? TStringSlot.toJSON(message.DeviceType)
        : undefined);
    return obj;
  },
};

function createBaseTIoTDiscoverySuccessSemanticFrame(): TIoTDiscoverySuccessSemanticFrame {
  return { DeviceIDs: undefined, ProductIDs: undefined, DeviceType: undefined };
}

export const TIoTDiscoverySuccessSemanticFrame = {
  encode(
    message: TIoTDiscoverySuccessSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DeviceIDs !== undefined) {
      TStringSlot.encode(message.DeviceIDs, writer.uint32(10).fork()).ldelim();
    }
    if (message.ProductIDs !== undefined) {
      TStringSlot.encode(message.ProductIDs, writer.uint32(18).fork()).ldelim();
    }
    if (message.DeviceType !== undefined) {
      TStringSlot.encode(message.DeviceType, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTDiscoverySuccessSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDiscoverySuccessSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceIDs = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.ProductIDs = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.DeviceType = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTDiscoverySuccessSemanticFrame {
    return {
      DeviceIDs: isSet(object.device_ids)
        ? TStringSlot.fromJSON(object.device_ids)
        : undefined,
      ProductIDs: isSet(object.product_ids)
        ? TStringSlot.fromJSON(object.product_ids)
        : undefined,
      DeviceType: isSet(object.device_type)
        ? TStringSlot.fromJSON(object.device_type)
        : undefined,
    };
  },

  toJSON(message: TIoTDiscoverySuccessSemanticFrame): unknown {
    const obj: any = {};
    message.DeviceIDs !== undefined &&
      (obj.device_ids = message.DeviceIDs
        ? TStringSlot.toJSON(message.DeviceIDs)
        : undefined);
    message.ProductIDs !== undefined &&
      (obj.product_ids = message.ProductIDs
        ? TStringSlot.toJSON(message.ProductIDs)
        : undefined);
    message.DeviceType !== undefined &&
      (obj.device_type = message.DeviceType
        ? TStringSlot.toJSON(message.DeviceType)
        : undefined);
    return obj;
  },
};

function createBaseTStartIotDiscoverySemanticFrame(): TStartIotDiscoverySemanticFrame {
  return { Request: undefined, SessionID: undefined };
}

export const TStartIotDiscoverySemanticFrame = {
  encode(
    message: TStartIotDiscoverySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TStartIotDiscoveryRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.SessionID !== undefined) {
      TStringSlot.encode(message.SessionID, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TStartIotDiscoverySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTStartIotDiscoverySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TStartIotDiscoveryRequestSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.SessionID = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TStartIotDiscoverySemanticFrame {
    return {
      Request: isSet(object.request)
        ? TStartIotDiscoveryRequestSlot.fromJSON(object.request)
        : undefined,
      SessionID: isSet(object.session_id)
        ? TStringSlot.fromJSON(object.session_id)
        : undefined,
    };
  },

  toJSON(message: TStartIotDiscoverySemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TStartIotDiscoveryRequestSlot.toJSON(message.Request)
        : undefined);
    message.SessionID !== undefined &&
      (obj.session_id = message.SessionID
        ? TStringSlot.toJSON(message.SessionID)
        : undefined);
    return obj;
  },
};

function createBaseTFinishIotDiscoverySemanticFrame(): TFinishIotDiscoverySemanticFrame {
  return { Request: undefined };
}

export const TFinishIotDiscoverySemanticFrame = {
  encode(
    message: TFinishIotDiscoverySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TFinishIotDiscoveryRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TFinishIotDiscoverySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFinishIotDiscoverySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TFinishIotDiscoveryRequestSlot.decode(
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

  fromJSON(object: any): TFinishIotDiscoverySemanticFrame {
    return {
      Request: isSet(object.request)
        ? TFinishIotDiscoveryRequestSlot.fromJSON(object.request)
        : undefined,
    };
  },

  toJSON(message: TFinishIotDiscoverySemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TFinishIotDiscoveryRequestSlot.toJSON(message.Request)
        : undefined);
    return obj;
  },
};

function createBaseTFinishIotSystemDiscoverySemanticFrame(): TFinishIotSystemDiscoverySemanticFrame {
  return { Request: undefined };
}

export const TFinishIotSystemDiscoverySemanticFrame = {
  encode(
    message: TFinishIotSystemDiscoverySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TFinishIotDiscoveryRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TFinishIotSystemDiscoverySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFinishIotSystemDiscoverySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TFinishIotDiscoveryRequestSlot.decode(
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

  fromJSON(object: any): TFinishIotSystemDiscoverySemanticFrame {
    return {
      Request: isSet(object.request)
        ? TFinishIotDiscoveryRequestSlot.fromJSON(object.request)
        : undefined,
    };
  },

  toJSON(message: TFinishIotSystemDiscoverySemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TFinishIotDiscoveryRequestSlot.toJSON(message.Request)
        : undefined);
    return obj;
  },
};

function createBaseTPutMoneyOnPhoneSemanticFrame(): TPutMoneyOnPhoneSemanticFrame {
  return { Amount: undefined, PhoneNumber: undefined };
}

export const TPutMoneyOnPhoneSemanticFrame = {
  encode(
    message: TPutMoneyOnPhoneSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Amount !== undefined) {
      TSysNumSlot.encode(message.Amount, writer.uint32(10).fork()).ldelim();
    }
    if (message.PhoneNumber !== undefined) {
      TStringSlot.encode(
        message.PhoneNumber,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPutMoneyOnPhoneSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPutMoneyOnPhoneSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Amount = TSysNumSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.PhoneNumber = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPutMoneyOnPhoneSemanticFrame {
    return {
      Amount: isSet(object.amount)
        ? TSysNumSlot.fromJSON(object.amount)
        : undefined,
      PhoneNumber: isSet(object.phone_number)
        ? TStringSlot.fromJSON(object.phone_number)
        : undefined,
    };
  },

  toJSON(message: TPutMoneyOnPhoneSemanticFrame): unknown {
    const obj: any = {};
    message.Amount !== undefined &&
      (obj.amount = message.Amount
        ? TSysNumSlot.toJSON(message.Amount)
        : undefined);
    message.PhoneNumber !== undefined &&
      (obj.phone_number = message.PhoneNumber
        ? TStringSlot.toJSON(message.PhoneNumber)
        : undefined);
    return obj;
  },
};

function createBaseTStartIotTuyaBroadcastSemanticFrame(): TStartIotTuyaBroadcastSemanticFrame {
  return { SSID: undefined, Password: undefined };
}

export const TStartIotTuyaBroadcastSemanticFrame = {
  encode(
    message: TStartIotTuyaBroadcastSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SSID !== undefined) {
      TStringSlot.encode(message.SSID, writer.uint32(10).fork()).ldelim();
    }
    if (message.Password !== undefined) {
      TStringSlot.encode(message.Password, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TStartIotTuyaBroadcastSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTStartIotTuyaBroadcastSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SSID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Password = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TStartIotTuyaBroadcastSemanticFrame {
    return {
      SSID: isSet(object.ssid) ? TStringSlot.fromJSON(object.ssid) : undefined,
      Password: isSet(object.password)
        ? TStringSlot.fromJSON(object.password)
        : undefined,
    };
  },

  toJSON(message: TStartIotTuyaBroadcastSemanticFrame): unknown {
    const obj: any = {};
    message.SSID !== undefined &&
      (obj.ssid = message.SSID ? TStringSlot.toJSON(message.SSID) : undefined);
    message.Password !== undefined &&
      (obj.password = message.Password
        ? TStringSlot.toJSON(message.Password)
        : undefined);
    return obj;
  },
};

function createBaseTRestoreIotNetworksSemanticFrame(): TRestoreIotNetworksSemanticFrame {
  return {};
}

export const TRestoreIotNetworksSemanticFrame = {
  encode(
    _: TRestoreIotNetworksSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRestoreIotNetworksSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRestoreIotNetworksSemanticFrame();
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

  fromJSON(_: any): TRestoreIotNetworksSemanticFrame {
    return {};
  },

  toJSON(_: TRestoreIotNetworksSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTDeleteIotNetworksSemanticFrame(): TDeleteIotNetworksSemanticFrame {
  return {};
}

export const TDeleteIotNetworksSemanticFrame = {
  encode(
    _: TDeleteIotNetworksSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TDeleteIotNetworksSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDeleteIotNetworksSemanticFrame();
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

  fromJSON(_: any): TDeleteIotNetworksSemanticFrame {
    return {};
  },

  toJSON(_: TDeleteIotNetworksSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTSaveIotNetworksSemanticFrame(): TSaveIotNetworksSemanticFrame {
  return { Networks: undefined };
}

export const TSaveIotNetworksSemanticFrame = {
  encode(
    message: TSaveIotNetworksSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Networks !== undefined) {
      TIotNetworksSlot.encode(
        message.Networks,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSaveIotNetworksSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSaveIotNetworksSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Networks = TIotNetworksSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSaveIotNetworksSemanticFrame {
    return {
      Networks: isSet(object.networks)
        ? TIotNetworksSlot.fromJSON(object.networks)
        : undefined,
    };
  },

  toJSON(message: TSaveIotNetworksSemanticFrame): unknown {
    const obj: any = {};
    message.Networks !== undefined &&
      (obj.networks = message.Networks
        ? TIotNetworksSlot.toJSON(message.Networks)
        : undefined);
    return obj;
  },
};

function createBaseTForgetIotEndpointsSemanticFrame(): TForgetIotEndpointsSemanticFrame {
  return { Request: undefined };
}

export const TForgetIotEndpointsSemanticFrame = {
  encode(
    message: TForgetIotEndpointsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TForgetIotEndpointsRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TForgetIotEndpointsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTForgetIotEndpointsSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TForgetIotEndpointsRequestSlot.decode(
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

  fromJSON(object: any): TForgetIotEndpointsSemanticFrame {
    return {
      Request: isSet(object.request)
        ? TForgetIotEndpointsRequestSlot.fromJSON(object.request)
        : undefined,
    };
  },

  toJSON(message: TForgetIotEndpointsSemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TForgetIotEndpointsRequestSlot.toJSON(message.Request)
        : undefined);
    return obj;
  },
};

function createBaseTIotYandexIOActionSemanticFrame(): TIotYandexIOActionSemanticFrame {
  return { Request: undefined };
}

export const TIotYandexIOActionSemanticFrame = {
  encode(
    message: TIotYandexIOActionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TIotYandexIOActionRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotYandexIOActionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotYandexIOActionSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TIotYandexIOActionRequestSlot.decode(
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

  fromJSON(object: any): TIotYandexIOActionSemanticFrame {
    return {
      Request: isSet(object.request)
        ? TIotYandexIOActionRequestSlot.fromJSON(object.request)
        : undefined,
    };
  },

  toJSON(message: TIotYandexIOActionSemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TIotYandexIOActionRequestSlot.toJSON(message.Request)
        : undefined);
    return obj;
  },
};

function createBaseTIotScenarioStepActionsSemanticFrame(): TIotScenarioStepActionsSemanticFrame {
  return {
    LaunchID: undefined,
    StepIndex: undefined,
    DeviceActionsBatch: undefined,
  };
}

export const TIotScenarioStepActionsSemanticFrame = {
  encode(
    message: TIotScenarioStepActionsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.LaunchID !== undefined) {
      TStringSlot.encode(message.LaunchID, writer.uint32(10).fork()).ldelim();
    }
    if (message.StepIndex !== undefined) {
      TUInt32Slot.encode(message.StepIndex, writer.uint32(18).fork()).ldelim();
    }
    if (message.DeviceActionsBatch !== undefined) {
      TIoTDeviceActionsBatchSlot.encode(
        message.DeviceActionsBatch,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIotScenarioStepActionsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIotScenarioStepActionsSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LaunchID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.StepIndex = TUInt32Slot.decode(reader, reader.uint32());
          break;
        case 3:
          message.DeviceActionsBatch = TIoTDeviceActionsBatchSlot.decode(
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

  fromJSON(object: any): TIotScenarioStepActionsSemanticFrame {
    return {
      LaunchID: isSet(object.launch_id)
        ? TStringSlot.fromJSON(object.launch_id)
        : undefined,
      StepIndex: isSet(object.step_index)
        ? TUInt32Slot.fromJSON(object.step_index)
        : undefined,
      DeviceActionsBatch: isSet(object.device_actions_batch)
        ? TIoTDeviceActionsBatchSlot.fromJSON(object.device_actions_batch)
        : undefined,
    };
  },

  toJSON(message: TIotScenarioStepActionsSemanticFrame): unknown {
    const obj: any = {};
    message.LaunchID !== undefined &&
      (obj.launch_id = message.LaunchID
        ? TStringSlot.toJSON(message.LaunchID)
        : undefined);
    message.StepIndex !== undefined &&
      (obj.step_index = message.StepIndex
        ? TUInt32Slot.toJSON(message.StepIndex)
        : undefined);
    message.DeviceActionsBatch !== undefined &&
      (obj.device_actions_batch = message.DeviceActionsBatch
        ? TIoTDeviceActionsBatchSlot.toJSON(message.DeviceActionsBatch)
        : undefined);
    return obj;
  },
};

function createBaseTEndpointStateUpdatesSemanticFrame(): TEndpointStateUpdatesSemanticFrame {
  return { Request: undefined };
}

export const TEndpointStateUpdatesSemanticFrame = {
  encode(
    message: TEndpointStateUpdatesSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TEndpointStateUpdatesRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointStateUpdatesSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointStateUpdatesSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TEndpointStateUpdatesRequestSlot.decode(
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

  fromJSON(object: any): TEndpointStateUpdatesSemanticFrame {
    return {
      Request: isSet(object.request)
        ? TEndpointStateUpdatesRequestSlot.fromJSON(object.request)
        : undefined,
    };
  },

  toJSON(message: TEndpointStateUpdatesSemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TEndpointStateUpdatesRequestSlot.toJSON(message.Request)
        : undefined);
    return obj;
  },
};

function createBaseTCapabilityEventSemanticFrame(): TCapabilityEventSemanticFrame {
  return {};
}

export const TCapabilityEventSemanticFrame = {
  encode(
    _: TCapabilityEventSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCapabilityEventSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCapabilityEventSemanticFrame();
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

  fromJSON(_: any): TCapabilityEventSemanticFrame {
    return {};
  },

  toJSON(_: TCapabilityEventSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTEndpointCapabilityEventsSemanticFrame(): TEndpointCapabilityEventsSemanticFrame {
  return { Events: undefined };
}

export const TEndpointCapabilityEventsSemanticFrame = {
  encode(
    message: TEndpointCapabilityEventsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Events !== undefined) {
      TEndpointCapabilityEventsSlot.encode(
        message.Events,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointCapabilityEventsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointCapabilityEventsSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Events = TEndpointCapabilityEventsSlot.decode(
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

  fromJSON(object: any): TEndpointCapabilityEventsSemanticFrame {
    return {
      Events: isSet(object.events)
        ? TEndpointCapabilityEventsSlot.fromJSON(object.events)
        : undefined,
    };
  },

  toJSON(message: TEndpointCapabilityEventsSemanticFrame): unknown {
    const obj: any = {};
    message.Events !== undefined &&
      (obj.events = message.Events
        ? TEndpointCapabilityEventsSlot.toJSON(message.Events)
        : undefined);
    return obj;
  },
};

function createBaseTEndpointEventsBatchSemanticFrame(): TEndpointEventsBatchSemanticFrame {
  return { Batch: undefined };
}

export const TEndpointEventsBatchSemanticFrame = {
  encode(
    message: TEndpointEventsBatchSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Batch !== undefined) {
      TEndpointEventsBatchSlot.encode(
        message.Batch,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEndpointEventsBatchSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEndpointEventsBatchSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Batch = TEndpointEventsBatchSlot.decode(
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

  fromJSON(object: any): TEndpointEventsBatchSemanticFrame {
    return {
      Batch: isSet(object.batch)
        ? TEndpointEventsBatchSlot.fromJSON(object.batch)
        : undefined,
    };
  },

  toJSON(message: TEndpointEventsBatchSemanticFrame): unknown {
    const obj: any = {};
    message.Batch !== undefined &&
      (obj.batch = message.Batch
        ? TEndpointEventsBatchSlot.toJSON(message.Batch)
        : undefined);
    return obj;
  },
};

function createBaseTMordoviaHomeScreenSemanticFrame(): TMordoviaHomeScreenSemanticFrame {
  return { DeviceID: undefined };
}

export const TMordoviaHomeScreenSemanticFrame = {
  encode(
    message: TMordoviaHomeScreenSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DeviceID !== undefined) {
      TStringSlot.encode(message.DeviceID, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMordoviaHomeScreenSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMordoviaHomeScreenSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DeviceID = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMordoviaHomeScreenSemanticFrame {
    return {
      DeviceID: isSet(object.device_id)
        ? TStringSlot.fromJSON(object.device_id)
        : undefined,
    };
  },

  toJSON(message: TMordoviaHomeScreenSemanticFrame): unknown {
    const obj: any = {};
    message.DeviceID !== undefined &&
      (obj.device_id = message.DeviceID
        ? TStringSlot.toJSON(message.DeviceID)
        : undefined);
    return obj;
  },
};

function createBaseTNewsSemanticFrame(): TNewsSemanticFrame {
  return {
    Topic: undefined,
    MaxCount: undefined,
    SkipIntroAndEnding: undefined,
    Provider: undefined,
    Where: undefined,
    DisableVoiceButtons: undefined,
    GoBack: undefined,
    NewsIdx: undefined,
    SingleNews: undefined,
  };
}

export const TNewsSemanticFrame = {
  encode(
    message: TNewsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Topic !== undefined) {
      TStringSlot.encode(message.Topic, writer.uint32(10).fork()).ldelim();
    }
    if (message.MaxCount !== undefined) {
      TNumSlot.encode(message.MaxCount, writer.uint32(18).fork()).ldelim();
    }
    if (message.SkipIntroAndEnding !== undefined) {
      TBoolSlot.encode(
        message.SkipIntroAndEnding,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Provider !== undefined) {
      TNewsProviderSlot.encode(
        message.Provider,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.Where !== undefined) {
      TWhereSlot.encode(message.Where, writer.uint32(42).fork()).ldelim();
    }
    if (message.DisableVoiceButtons !== undefined) {
      TBoolSlot.encode(
        message.DisableVoiceButtons,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.GoBack !== undefined) {
      TBoolSlot.encode(message.GoBack, writer.uint32(58).fork()).ldelim();
    }
    if (message.NewsIdx !== undefined) {
      TNumSlot.encode(message.NewsIdx, writer.uint32(66).fork()).ldelim();
    }
    if (message.SingleNews !== undefined) {
      TBoolSlot.encode(message.SingleNews, writer.uint32(74).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TNewsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNewsSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Topic = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.MaxCount = TNumSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.SkipIntroAndEnding = TBoolSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.Provider = TNewsProviderSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.Where = TWhereSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.DisableVoiceButtons = TBoolSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 7:
          message.GoBack = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 8:
          message.NewsIdx = TNumSlot.decode(reader, reader.uint32());
          break;
        case 9:
          message.SingleNews = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TNewsSemanticFrame {
    return {
      Topic: isSet(object.topic)
        ? TStringSlot.fromJSON(object.topic)
        : undefined,
      MaxCount: isSet(object.max_count)
        ? TNumSlot.fromJSON(object.max_count)
        : undefined,
      SkipIntroAndEnding: isSet(object.skip_intro_and_ending)
        ? TBoolSlot.fromJSON(object.skip_intro_and_ending)
        : undefined,
      Provider: isSet(object.provider)
        ? TNewsProviderSlot.fromJSON(object.provider)
        : undefined,
      Where: isSet(object.where)
        ? TWhereSlot.fromJSON(object.where)
        : undefined,
      DisableVoiceButtons: isSet(object.disable_voice_buttons)
        ? TBoolSlot.fromJSON(object.disable_voice_buttons)
        : undefined,
      GoBack: isSet(object.go_back)
        ? TBoolSlot.fromJSON(object.go_back)
        : undefined,
      NewsIdx: isSet(object.news_idx)
        ? TNumSlot.fromJSON(object.news_idx)
        : undefined,
      SingleNews: isSet(object.single_news)
        ? TBoolSlot.fromJSON(object.single_news)
        : undefined,
    };
  },

  toJSON(message: TNewsSemanticFrame): unknown {
    const obj: any = {};
    message.Topic !== undefined &&
      (obj.topic = message.Topic
        ? TStringSlot.toJSON(message.Topic)
        : undefined);
    message.MaxCount !== undefined &&
      (obj.max_count = message.MaxCount
        ? TNumSlot.toJSON(message.MaxCount)
        : undefined);
    message.SkipIntroAndEnding !== undefined &&
      (obj.skip_intro_and_ending = message.SkipIntroAndEnding
        ? TBoolSlot.toJSON(message.SkipIntroAndEnding)
        : undefined);
    message.Provider !== undefined &&
      (obj.provider = message.Provider
        ? TNewsProviderSlot.toJSON(message.Provider)
        : undefined);
    message.Where !== undefined &&
      (obj.where = message.Where
        ? TWhereSlot.toJSON(message.Where)
        : undefined);
    message.DisableVoiceButtons !== undefined &&
      (obj.disable_voice_buttons = message.DisableVoiceButtons
        ? TBoolSlot.toJSON(message.DisableVoiceButtons)
        : undefined);
    message.GoBack !== undefined &&
      (obj.go_back = message.GoBack
        ? TBoolSlot.toJSON(message.GoBack)
        : undefined);
    message.NewsIdx !== undefined &&
      (obj.news_idx = message.NewsIdx
        ? TNumSlot.toJSON(message.NewsIdx)
        : undefined);
    message.SingleNews !== undefined &&
      (obj.single_news = message.SingleNews
        ? TBoolSlot.toJSON(message.SingleNews)
        : undefined);
    return obj;
  },
};

function createBaseTGetCallerNameSemanticFrame(): TGetCallerNameSemanticFrame {
  return { CallerDeviceID: undefined, CallerPayload: undefined };
}

export const TGetCallerNameSemanticFrame = {
  encode(
    message: TGetCallerNameSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CallerDeviceID !== undefined) {
      TStringSlot.encode(
        message.CallerDeviceID,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.CallerPayload !== undefined) {
      TStringSlot.encode(
        message.CallerPayload,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetCallerNameSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetCallerNameSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CallerDeviceID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.CallerPayload = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetCallerNameSemanticFrame {
    return {
      CallerDeviceID: isSet(object.caller_device_id)
        ? TStringSlot.fromJSON(object.caller_device_id)
        : undefined,
      CallerPayload: isSet(object.caller_payload)
        ? TStringSlot.fromJSON(object.caller_payload)
        : undefined,
    };
  },

  toJSON(message: TGetCallerNameSemanticFrame): unknown {
    const obj: any = {};
    message.CallerDeviceID !== undefined &&
      (obj.caller_device_id = message.CallerDeviceID
        ? TStringSlot.toJSON(message.CallerDeviceID)
        : undefined);
    message.CallerPayload !== undefined &&
      (obj.caller_payload = message.CallerPayload
        ? TStringSlot.toJSON(message.CallerPayload)
        : undefined);
    return obj;
  },
};

function createBaseTTokenTypeSlot(): TTokenTypeSlot {
  return { EnumValue: undefined };
}

export const TTokenTypeSlot = {
  encode(message: TTokenTypeSlot, writer: Writer = Writer.create()): Writer {
    if (message.EnumValue !== undefined) {
      writer.uint32(8).int32(message.EnumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTokenTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTokenTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnumValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTokenTypeSlot {
    return {
      EnumValue: isSet(object.enum_value)
        ? tTokenTypeSlot_EValueFromJSON(object.enum_value)
        : undefined,
    };
  },

  toJSON(message: TTokenTypeSlot): unknown {
    const obj: any = {};
    message.EnumValue !== undefined &&
      (obj.enum_value =
        message.EnumValue !== undefined
          ? tTokenTypeSlot_EValueToJSON(message.EnumValue)
          : undefined);
    return obj;
  },
};

function createBaseTAddAccountSemanticFrame(): TAddAccountSemanticFrame {
  return {
    JwtToken: undefined,
    TokenType: undefined,
    Encrypted: undefined,
    EncryptedCode: undefined,
    Signature: undefined,
    EncryptedSessionKey: undefined,
  };
}

export const TAddAccountSemanticFrame = {
  encode(
    message: TAddAccountSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.JwtToken !== undefined) {
      TStringSlot.encode(message.JwtToken, writer.uint32(10).fork()).ldelim();
    }
    if (message.TokenType !== undefined) {
      TTokenTypeSlot.encode(
        message.TokenType,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Encrypted !== undefined) {
      TBoolSlot.encode(message.Encrypted, writer.uint32(26).fork()).ldelim();
    }
    if (message.EncryptedCode !== undefined) {
      TStringSlot.encode(
        message.EncryptedCode,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.Signature !== undefined) {
      TStringSlot.encode(message.Signature, writer.uint32(42).fork()).ldelim();
    }
    if (message.EncryptedSessionKey !== undefined) {
      TStringSlot.encode(
        message.EncryptedSessionKey,
        writer.uint32(50).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAddAccountSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAddAccountSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.JwtToken = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.TokenType = TTokenTypeSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Encrypted = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.EncryptedCode = TStringSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.Signature = TStringSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.EncryptedSessionKey = TStringSlot.decode(
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

  fromJSON(object: any): TAddAccountSemanticFrame {
    return {
      JwtToken: isSet(object.jwt_token)
        ? TStringSlot.fromJSON(object.jwt_token)
        : undefined,
      TokenType: isSet(object.token_type)
        ? TTokenTypeSlot.fromJSON(object.token_type)
        : undefined,
      Encrypted: isSet(object.encrypted)
        ? TBoolSlot.fromJSON(object.encrypted)
        : undefined,
      EncryptedCode: isSet(object.encrypted_code)
        ? TStringSlot.fromJSON(object.encrypted_code)
        : undefined,
      Signature: isSet(object.signature)
        ? TStringSlot.fromJSON(object.signature)
        : undefined,
      EncryptedSessionKey: isSet(object.encrypted_session_key)
        ? TStringSlot.fromJSON(object.encrypted_session_key)
        : undefined,
    };
  },

  toJSON(message: TAddAccountSemanticFrame): unknown {
    const obj: any = {};
    message.JwtToken !== undefined &&
      (obj.jwt_token = message.JwtToken
        ? TStringSlot.toJSON(message.JwtToken)
        : undefined);
    message.TokenType !== undefined &&
      (obj.token_type = message.TokenType
        ? TTokenTypeSlot.toJSON(message.TokenType)
        : undefined);
    message.Encrypted !== undefined &&
      (obj.encrypted = message.Encrypted
        ? TBoolSlot.toJSON(message.Encrypted)
        : undefined);
    message.EncryptedCode !== undefined &&
      (obj.encrypted_code = message.EncryptedCode
        ? TStringSlot.toJSON(message.EncryptedCode)
        : undefined);
    message.Signature !== undefined &&
      (obj.signature = message.Signature
        ? TStringSlot.toJSON(message.Signature)
        : undefined);
    message.EncryptedSessionKey !== undefined &&
      (obj.encrypted_session_key = message.EncryptedSessionKey
        ? TStringSlot.toJSON(message.EncryptedSessionKey)
        : undefined);
    return obj;
  },
};

function createBaseTRemoveAccountSemanticFrame(): TRemoveAccountSemanticFrame {
  return { Puid: undefined };
}

export const TRemoveAccountSemanticFrame = {
  encode(
    message: TRemoveAccountSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Puid !== undefined) {
      TStringSlot.encode(message.Puid, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRemoveAccountSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRemoveAccountSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Puid = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRemoveAccountSemanticFrame {
    return {
      Puid: isSet(object.puid) ? TStringSlot.fromJSON(object.puid) : undefined,
    };
  },

  toJSON(message: TRemoveAccountSemanticFrame): unknown {
    const obj: any = {};
    message.Puid !== undefined &&
      (obj.puid = message.Puid ? TStringSlot.toJSON(message.Puid) : undefined);
    return obj;
  },
};

function createBaseTEnrollmentStatusTypeSlot(): TEnrollmentStatusTypeSlot {
  return { EnrollmentStatus: undefined };
}

export const TEnrollmentStatusTypeSlot = {
  encode(
    message: TEnrollmentStatusTypeSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EnrollmentStatus !== undefined) {
      TEnrollmentStatus.encode(
        message.EnrollmentStatus,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEnrollmentStatusTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEnrollmentStatusTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnrollmentStatus = TEnrollmentStatus.decode(
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

  fromJSON(object: any): TEnrollmentStatusTypeSlot {
    return {
      EnrollmentStatus: isSet(object.enrollment_status)
        ? TEnrollmentStatus.fromJSON(object.enrollment_status)
        : undefined,
    };
  },

  toJSON(message: TEnrollmentStatusTypeSlot): unknown {
    const obj: any = {};
    message.EnrollmentStatus !== undefined &&
      (obj.enrollment_status = message.EnrollmentStatus
        ? TEnrollmentStatus.toJSON(message.EnrollmentStatus)
        : undefined);
    return obj;
  },
};

function createBaseTEnrollmentStatusSemanticFrame(): TEnrollmentStatusSemanticFrame {
  return { Puid: undefined, Status: undefined };
}

export const TEnrollmentStatusSemanticFrame = {
  encode(
    message: TEnrollmentStatusSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Puid !== undefined) {
      TStringSlot.encode(message.Puid, writer.uint32(10).fork()).ldelim();
    }
    if (message.Status !== undefined) {
      TEnrollmentStatusTypeSlot.encode(
        message.Status,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TEnrollmentStatusSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTEnrollmentStatusSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Puid = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Status = TEnrollmentStatusTypeSlot.decode(
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

  fromJSON(object: any): TEnrollmentStatusSemanticFrame {
    return {
      Puid: isSet(object.puid) ? TStringSlot.fromJSON(object.puid) : undefined,
      Status: isSet(object.status)
        ? TEnrollmentStatusTypeSlot.fromJSON(object.status)
        : undefined,
    };
  },

  toJSON(message: TEnrollmentStatusSemanticFrame): unknown {
    const obj: any = {};
    message.Puid !== undefined &&
      (obj.puid = message.Puid ? TStringSlot.toJSON(message.Puid) : undefined);
    message.Status !== undefined &&
      (obj.status = message.Status
        ? TEnrollmentStatusTypeSlot.toJSON(message.Status)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurCollectCardsSemanticFrame(): TCentaurCollectCardsSemanticFrame {
  return {
    CarouselId: undefined,
    IsScheduledUpdate: undefined,
    IsGetSettingsRequest: undefined,
  };
}

export const TCentaurCollectCardsSemanticFrame = {
  encode(
    message: TCentaurCollectCardsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CarouselId !== undefined) {
      TStringSlot.encode(message.CarouselId, writer.uint32(10).fork()).ldelim();
    }
    if (message.IsScheduledUpdate !== undefined) {
      TBoolSlot.encode(
        message.IsScheduledUpdate,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.IsGetSettingsRequest !== undefined) {
      TBoolSlot.encode(
        message.IsGetSettingsRequest,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurCollectCardsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurCollectCardsSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CarouselId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.IsScheduledUpdate = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.IsGetSettingsRequest = TBoolSlot.decode(
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

  fromJSON(object: any): TCentaurCollectCardsSemanticFrame {
    return {
      CarouselId: isSet(object.carousel_id)
        ? TStringSlot.fromJSON(object.carousel_id)
        : undefined,
      IsScheduledUpdate: isSet(object.is_scheduled_update)
        ? TBoolSlot.fromJSON(object.is_scheduled_update)
        : undefined,
      IsGetSettingsRequest: isSet(object.is_get_settings_request)
        ? TBoolSlot.fromJSON(object.is_get_settings_request)
        : undefined,
    };
  },

  toJSON(message: TCentaurCollectCardsSemanticFrame): unknown {
    const obj: any = {};
    message.CarouselId !== undefined &&
      (obj.carousel_id = message.CarouselId
        ? TStringSlot.toJSON(message.CarouselId)
        : undefined);
    message.IsScheduledUpdate !== undefined &&
      (obj.is_scheduled_update = message.IsScheduledUpdate
        ? TBoolSlot.toJSON(message.IsScheduledUpdate)
        : undefined);
    message.IsGetSettingsRequest !== undefined &&
      (obj.is_get_settings_request = message.IsGetSettingsRequest
        ? TBoolSlot.toJSON(message.IsGetSettingsRequest)
        : undefined);
    return obj;
  },
};

function createBaseTWidgetPositionSlot(): TWidgetPositionSlot {
  return { WidgetPositionValue: undefined };
}

export const TWidgetPositionSlot = {
  encode(
    message: TWidgetPositionSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.WidgetPositionValue !== undefined) {
      TWidgetPosition.encode(
        message.WidgetPositionValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWidgetPositionSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWidgetPositionSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.WidgetPositionValue = TWidgetPosition.decode(
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

  fromJSON(object: any): TWidgetPositionSlot {
    return {
      WidgetPositionValue: isSet(object.widget_position_value)
        ? TWidgetPosition.fromJSON(object.widget_position_value)
        : undefined,
    };
  },

  toJSON(message: TWidgetPositionSlot): unknown {
    const obj: any = {};
    message.WidgetPositionValue !== undefined &&
      (obj.widget_position_value = message.WidgetPositionValue
        ? TWidgetPosition.toJSON(message.WidgetPositionValue)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurCollectMainScreenSemanticFrame(): TCentaurCollectMainScreenSemanticFrame {
  return {
    WidgetGalleryPosition: undefined,
    WidgetConfigDataSlot: undefined,
    WidgetMainScreenPosition: undefined,
  };
}

export const TCentaurCollectMainScreenSemanticFrame = {
  encode(
    message: TCentaurCollectMainScreenSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.WidgetGalleryPosition !== undefined) {
      TWidgetPositionSlot.encode(
        message.WidgetGalleryPosition,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.WidgetConfigDataSlot !== undefined) {
      TWidgetConfigDataSlot.encode(
        message.WidgetConfigDataSlot,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.WidgetMainScreenPosition !== undefined) {
      TWidgetPositionSlot.encode(
        message.WidgetMainScreenPosition,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurCollectMainScreenSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurCollectMainScreenSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.WidgetGalleryPosition = TWidgetPositionSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.WidgetConfigDataSlot = TWidgetConfigDataSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.WidgetMainScreenPosition = TWidgetPositionSlot.decode(
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

  fromJSON(object: any): TCentaurCollectMainScreenSemanticFrame {
    return {
      WidgetGalleryPosition: isSet(object.widget_gallery_position)
        ? TWidgetPositionSlot.fromJSON(object.widget_gallery_position)
        : undefined,
      WidgetConfigDataSlot: isSet(object.widget_config_data_slot)
        ? TWidgetConfigDataSlot.fromJSON(object.widget_config_data_slot)
        : undefined,
      WidgetMainScreenPosition: isSet(object.widget_main_screen_position)
        ? TWidgetPositionSlot.fromJSON(object.widget_main_screen_position)
        : undefined,
    };
  },

  toJSON(message: TCentaurCollectMainScreenSemanticFrame): unknown {
    const obj: any = {};
    message.WidgetGalleryPosition !== undefined &&
      (obj.widget_gallery_position = message.WidgetGalleryPosition
        ? TWidgetPositionSlot.toJSON(message.WidgetGalleryPosition)
        : undefined);
    message.WidgetConfigDataSlot !== undefined &&
      (obj.widget_config_data_slot = message.WidgetConfigDataSlot
        ? TWidgetConfigDataSlot.toJSON(message.WidgetConfigDataSlot)
        : undefined);
    message.WidgetMainScreenPosition !== undefined &&
      (obj.widget_main_screen_position = message.WidgetMainScreenPosition
        ? TWidgetPositionSlot.toJSON(message.WidgetMainScreenPosition)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurCollectUpperShutterSemanticFrame(): TCentaurCollectUpperShutterSemanticFrame {
  return {};
}

export const TCentaurCollectUpperShutterSemanticFrame = {
  encode(
    _: TCentaurCollectUpperShutterSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurCollectUpperShutterSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurCollectUpperShutterSemanticFrame();
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

  fromJSON(_: any): TCentaurCollectUpperShutterSemanticFrame {
    return {};
  },

  toJSON(_: TCentaurCollectUpperShutterSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTCentaurGetCardSemanticFrame(): TCentaurGetCardSemanticFrame {
  return { CarouselId: undefined };
}

export const TCentaurGetCardSemanticFrame = {
  encode(
    message: TCentaurGetCardSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CarouselId !== undefined) {
      TStringSlot.encode(message.CarouselId, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurGetCardSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurGetCardSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CarouselId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCentaurGetCardSemanticFrame {
    return {
      CarouselId: isSet(object.carousel_id)
        ? TStringSlot.fromJSON(object.carousel_id)
        : undefined,
    };
  },

  toJSON(message: TCentaurGetCardSemanticFrame): unknown {
    const obj: any = {};
    message.CarouselId !== undefined &&
      (obj.carousel_id = message.CarouselId
        ? TStringSlot.toJSON(message.CarouselId)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurCollectWidgetGallerySemanticFrame(): TCentaurCollectWidgetGallerySemanticFrame {
  return { Column: undefined, Row: undefined };
}

export const TCentaurCollectWidgetGallerySemanticFrame = {
  encode(
    message: TCentaurCollectWidgetGallerySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Column !== undefined) {
      TNumSlot.encode(message.Column, writer.uint32(10).fork()).ldelim();
    }
    if (message.Row !== undefined) {
      TNumSlot.encode(message.Row, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurCollectWidgetGallerySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurCollectWidgetGallerySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Column = TNumSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Row = TNumSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCentaurCollectWidgetGallerySemanticFrame {
    return {
      Column: isSet(object.column)
        ? TNumSlot.fromJSON(object.column)
        : undefined,
      Row: isSet(object.row) ? TNumSlot.fromJSON(object.row) : undefined,
    };
  },

  toJSON(message: TCentaurCollectWidgetGallerySemanticFrame): unknown {
    const obj: any = {};
    message.Column !== undefined &&
      (obj.column = message.Column
        ? TNumSlot.toJSON(message.Column)
        : undefined);
    message.Row !== undefined &&
      (obj.row = message.Row ? TNumSlot.toJSON(message.Row) : undefined);
    return obj;
  },
};

function createBaseTWidgetDataSlot(): TWidgetDataSlot {
  return { WidgetDataValue: undefined };
}

export const TWidgetDataSlot = {
  encode(message: TWidgetDataSlot, writer: Writer = Writer.create()): Writer {
    if (message.WidgetDataValue !== undefined) {
      TCentaurWidgetData.encode(
        message.WidgetDataValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWidgetDataSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWidgetDataSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.WidgetDataValue = TCentaurWidgetData.decode(
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

  fromJSON(object: any): TWidgetDataSlot {
    return {
      WidgetDataValue: isSet(object.widget_data_value)
        ? TCentaurWidgetData.fromJSON(object.widget_data_value)
        : undefined,
    };
  },

  toJSON(message: TWidgetDataSlot): unknown {
    const obj: any = {};
    message.WidgetDataValue !== undefined &&
      (obj.widget_data_value = message.WidgetDataValue
        ? TCentaurWidgetData.toJSON(message.WidgetDataValue)
        : undefined);
    return obj;
  },
};

function createBaseTWidgetConfigDataSlot(): TWidgetConfigDataSlot {
  return { WidgetConfigDataValue: undefined };
}

export const TWidgetConfigDataSlot = {
  encode(
    message: TWidgetConfigDataSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.WidgetConfigDataValue !== undefined) {
      TCentaurWidgetConfigData.encode(
        message.WidgetConfigDataValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWidgetConfigDataSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWidgetConfigDataSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.WidgetConfigDataValue = TCentaurWidgetConfigData.decode(
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

  fromJSON(object: any): TWidgetConfigDataSlot {
    return {
      WidgetConfigDataValue: isSet(object.widget_config_data_value)
        ? TCentaurWidgetConfigData.fromJSON(object.widget_config_data_value)
        : undefined,
    };
  },

  toJSON(message: TWidgetConfigDataSlot): unknown {
    const obj: any = {};
    message.WidgetConfigDataValue !== undefined &&
      (obj.widget_config_data_value = message.WidgetConfigDataValue
        ? TCentaurWidgetConfigData.toJSON(message.WidgetConfigDataValue)
        : undefined);
    return obj;
  },
};

function createBaseTCatalogTagSlot(): TCatalogTagSlot {
  return { CatalogTags: undefined };
}

export const TCatalogTagSlot = {
  encode(message: TCatalogTagSlot, writer: Writer = Writer.create()): Writer {
    if (message.CatalogTags !== undefined) {
      TCatalogTags.encode(
        message.CatalogTags,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCatalogTagSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCatalogTagSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CatalogTags = TCatalogTags.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCatalogTagSlot {
    return {
      CatalogTags: isSet(object.catalog_tags)
        ? TCatalogTags.fromJSON(object.catalog_tags)
        : undefined,
    };
  },

  toJSON(message: TCatalogTagSlot): unknown {
    const obj: any = {};
    message.CatalogTags !== undefined &&
      (obj.catalog_tags = message.CatalogTags
        ? TCatalogTags.toJSON(message.CatalogTags)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurAddWidgetFromGallerySemanticFrame(): TCentaurAddWidgetFromGallerySemanticFrame {
  return {
    Column: undefined,
    Row: undefined,
    WidgetDataSlot: undefined,
    WidgetConfigDataSlot: undefined,
  };
}

export const TCentaurAddWidgetFromGallerySemanticFrame = {
  encode(
    message: TCentaurAddWidgetFromGallerySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Column !== undefined) {
      TNumSlot.encode(message.Column, writer.uint32(10).fork()).ldelim();
    }
    if (message.Row !== undefined) {
      TNumSlot.encode(message.Row, writer.uint32(18).fork()).ldelim();
    }
    if (message.WidgetDataSlot !== undefined) {
      TWidgetDataSlot.encode(
        message.WidgetDataSlot,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.WidgetConfigDataSlot !== undefined) {
      TWidgetConfigDataSlot.encode(
        message.WidgetConfigDataSlot,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurAddWidgetFromGallerySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurAddWidgetFromGallerySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Column = TNumSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Row = TNumSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.WidgetDataSlot = TWidgetDataSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.WidgetConfigDataSlot = TWidgetConfigDataSlot.decode(
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

  fromJSON(object: any): TCentaurAddWidgetFromGallerySemanticFrame {
    return {
      Column: isSet(object.column)
        ? TNumSlot.fromJSON(object.column)
        : undefined,
      Row: isSet(object.row) ? TNumSlot.fromJSON(object.row) : undefined,
      WidgetDataSlot: isSet(object.widget_data_slot)
        ? TWidgetDataSlot.fromJSON(object.widget_data_slot)
        : undefined,
      WidgetConfigDataSlot: isSet(object.widget_config_data_slot)
        ? TWidgetConfigDataSlot.fromJSON(object.widget_config_data_slot)
        : undefined,
    };
  },

  toJSON(message: TCentaurAddWidgetFromGallerySemanticFrame): unknown {
    const obj: any = {};
    message.Column !== undefined &&
      (obj.column = message.Column
        ? TNumSlot.toJSON(message.Column)
        : undefined);
    message.Row !== undefined &&
      (obj.row = message.Row ? TNumSlot.toJSON(message.Row) : undefined);
    message.WidgetDataSlot !== undefined &&
      (obj.widget_data_slot = message.WidgetDataSlot
        ? TWidgetDataSlot.toJSON(message.WidgetDataSlot)
        : undefined);
    message.WidgetConfigDataSlot !== undefined &&
      (obj.widget_config_data_slot = message.WidgetConfigDataSlot
        ? TWidgetConfigDataSlot.toJSON(message.WidgetConfigDataSlot)
        : undefined);
    return obj;
  },
};

function createBaseTScenariosForTeasersSlot(): TScenariosForTeasersSlot {
  return { TeaserSettingsData: undefined };
}

export const TScenariosForTeasersSlot = {
  encode(
    message: TScenariosForTeasersSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TeaserSettingsData !== undefined) {
      TTeaserSettingsData.encode(
        message.TeaserSettingsData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TScenariosForTeasersSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTScenariosForTeasersSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TeaserSettingsData = TTeaserSettingsData.decode(
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

  fromJSON(object: any): TScenariosForTeasersSlot {
    return {
      TeaserSettingsData: isSet(object.teaser_settings_data)
        ? TTeaserSettingsData.fromJSON(object.teaser_settings_data)
        : undefined,
    };
  },

  toJSON(message: TScenariosForTeasersSlot): unknown {
    const obj: any = {};
    message.TeaserSettingsData !== undefined &&
      (obj.teaser_settings_data = message.TeaserSettingsData
        ? TTeaserSettingsData.toJSON(message.TeaserSettingsData)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurSetTeaserConfigurationSemanticFrame(): TCentaurSetTeaserConfigurationSemanticFrame {
  return { ScenariosForTeasersSlot: undefined };
}

export const TCentaurSetTeaserConfigurationSemanticFrame = {
  encode(
    message: TCentaurSetTeaserConfigurationSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ScenariosForTeasersSlot !== undefined) {
      TScenariosForTeasersSlot.encode(
        message.ScenariosForTeasersSlot,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurSetTeaserConfigurationSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurSetTeaserConfigurationSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ScenariosForTeasersSlot = TScenariosForTeasersSlot.decode(
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

  fromJSON(object: any): TCentaurSetTeaserConfigurationSemanticFrame {
    return {
      ScenariosForTeasersSlot: isSet(object.scenarios_for_teasers_slot)
        ? TScenariosForTeasersSlot.fromJSON(object.scenarios_for_teasers_slot)
        : undefined,
    };
  },

  toJSON(message: TCentaurSetTeaserConfigurationSemanticFrame): unknown {
    const obj: any = {};
    message.ScenariosForTeasersSlot !== undefined &&
      (obj.scenarios_for_teasers_slot = message.ScenariosForTeasersSlot
        ? TScenariosForTeasersSlot.toJSON(message.ScenariosForTeasersSlot)
        : undefined);
    return obj;
  },
};

function createBaseTCentaurCollectTeasersPreviewSemanticFrame(): TCentaurCollectTeasersPreviewSemanticFrame {
  return {};
}

export const TCentaurCollectTeasersPreviewSemanticFrame = {
  encode(
    _: TCentaurCollectTeasersPreviewSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TCentaurCollectTeasersPreviewSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCentaurCollectTeasersPreviewSemanticFrame();
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

  fromJSON(_: any): TCentaurCollectTeasersPreviewSemanticFrame {
    return {};
  },

  toJSON(_: TCentaurCollectTeasersPreviewSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTGetPhotoFrameSemanticFrame(): TGetPhotoFrameSemanticFrame {
  return { CarouselId: undefined, PhotoId: undefined };
}

export const TGetPhotoFrameSemanticFrame = {
  encode(
    message: TGetPhotoFrameSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CarouselId !== undefined) {
      TStringSlot.encode(message.CarouselId, writer.uint32(10).fork()).ldelim();
    }
    if (message.PhotoId !== undefined) {
      TNumSlot.encode(message.PhotoId, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetPhotoFrameSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetPhotoFrameSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CarouselId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.PhotoId = TNumSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetPhotoFrameSemanticFrame {
    return {
      CarouselId: isSet(object.carousel_id)
        ? TStringSlot.fromJSON(object.carousel_id)
        : undefined,
      PhotoId: isSet(object.photo_id)
        ? TNumSlot.fromJSON(object.photo_id)
        : undefined,
    };
  },

  toJSON(message: TGetPhotoFrameSemanticFrame): unknown {
    const obj: any = {};
    message.CarouselId !== undefined &&
      (obj.carousel_id = message.CarouselId
        ? TStringSlot.toJSON(message.CarouselId)
        : undefined);
    message.PhotoId !== undefined &&
      (obj.photo_id = message.PhotoId
        ? TNumSlot.toJSON(message.PhotoId)
        : undefined);
    return obj;
  },
};

function createBaseTExternalAppSlot(): TExternalAppSlot {
  return { ExternalAppValue: undefined };
}

export const TExternalAppSlot = {
  encode(message: TExternalAppSlot, writer: Writer = Writer.create()): Writer {
    if (message.ExternalAppValue !== undefined) {
      writer.uint32(10).string(message.ExternalAppValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TExternalAppSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTExternalAppSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ExternalAppValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TExternalAppSlot {
    return {
      ExternalAppValue: isSet(object.external_app_value)
        ? String(object.external_app_value)
        : undefined,
    };
  },

  toJSON(message: TExternalAppSlot): unknown {
    const obj: any = {};
    message.ExternalAppValue !== undefined &&
      (obj.external_app_value = message.ExternalAppValue);
    return obj;
  },
};

function createBaseTOpenSmartDeviceExternalAppFrame(): TOpenSmartDeviceExternalAppFrame {
  return { Application: undefined };
}

export const TOpenSmartDeviceExternalAppFrame = {
  encode(
    message: TOpenSmartDeviceExternalAppFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Application !== undefined) {
      TExternalAppSlot.encode(
        message.Application,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOpenSmartDeviceExternalAppFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOpenSmartDeviceExternalAppFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Application = TExternalAppSlot.decode(
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

  fromJSON(object: any): TOpenSmartDeviceExternalAppFrame {
    return {
      Application: isSet(object.application)
        ? TExternalAppSlot.fromJSON(object.application)
        : undefined,
    };
  },

  toJSON(message: TOpenSmartDeviceExternalAppFrame): unknown {
    const obj: any = {};
    message.Application !== undefined &&
      (obj.application = message.Application
        ? TExternalAppSlot.toJSON(message.Application)
        : undefined);
    return obj;
  },
};

function createBaseTMusicPlayObjectTypeSlot(): TMusicPlayObjectTypeSlot {
  return { EnumValue: undefined };
}

export const TMusicPlayObjectTypeSlot = {
  encode(
    message: TMusicPlayObjectTypeSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EnumValue !== undefined) {
      writer.uint32(8).int32(message.EnumValue);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicPlayObjectTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicPlayObjectTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnumValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMusicPlayObjectTypeSlot {
    return {
      EnumValue: isSet(object.enum_value)
        ? tMusicPlayObjectTypeSlot_EValueFromJSON(object.enum_value)
        : undefined,
    };
  },

  toJSON(message: TMusicPlayObjectTypeSlot): unknown {
    const obj: any = {};
    message.EnumValue !== undefined &&
      (obj.enum_value =
        message.EnumValue !== undefined
          ? tMusicPlayObjectTypeSlot_EValueToJSON(message.EnumValue)
          : undefined);
    return obj;
  },
};

function createBaseTMusicPlaySemanticFrame(): TMusicPlaySemanticFrame {
  return {
    SpecialPlaylist: undefined,
    SpecialAnswerInfo: undefined,
    ActionRequest: undefined,
    Epoch: undefined,
    SearchText: undefined,
    Genre: undefined,
    Mood: undefined,
    Activity: undefined,
    Language: undefined,
    Vocal: undefined,
    Novelty: undefined,
    Personality: undefined,
    Order: undefined,
    Repeat: undefined,
    DisableAutoflow: undefined,
    DisableNlg: undefined,
    PlaySingleTrack: undefined,
    TrackOffsetIndex: undefined,
    Playlist: undefined,
    ObjectId: undefined,
    ObjectType: undefined,
    StartFromTrackId: undefined,
    OffsetSec: undefined,
    AlarmId: undefined,
    From: undefined,
    DisableHistory: undefined,
    Room: undefined,
    Stream: undefined,
    GenerativeStation: undefined,
    NeedSimilar: undefined,
    Location: undefined,
    Offset: undefined,
    DisableMultiroom: undefined,
    ContentType: undefined,
  };
}

export const TMusicPlaySemanticFrame = {
  encode(
    message: TMusicPlaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SpecialPlaylist !== undefined) {
      TStringSlot.encode(
        message.SpecialPlaylist,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.SpecialAnswerInfo !== undefined) {
      TStringSlot.encode(
        message.SpecialAnswerInfo,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.ActionRequest !== undefined) {
      TStringSlot.encode(
        message.ActionRequest,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.Epoch !== undefined) {
      TStringSlot.encode(message.Epoch, writer.uint32(42).fork()).ldelim();
    }
    if (message.SearchText !== undefined) {
      TStringSlot.encode(message.SearchText, writer.uint32(50).fork()).ldelim();
    }
    if (message.Genre !== undefined) {
      TMusicSlot.encode(message.Genre, writer.uint32(58).fork()).ldelim();
    }
    if (message.Mood !== undefined) {
      TMusicSlot.encode(message.Mood, writer.uint32(66).fork()).ldelim();
    }
    if (message.Activity !== undefined) {
      TMusicSlot.encode(message.Activity, writer.uint32(74).fork()).ldelim();
    }
    if (message.Language !== undefined) {
      TLanguageSlot.encode(message.Language, writer.uint32(82).fork()).ldelim();
    }
    if (message.Vocal !== undefined) {
      TMusicSlot.encode(message.Vocal, writer.uint32(90).fork()).ldelim();
    }
    if (message.Novelty !== undefined) {
      TMusicSlot.encode(message.Novelty, writer.uint32(98).fork()).ldelim();
    }
    if (message.Personality !== undefined) {
      TMusicSlot.encode(
        message.Personality,
        writer.uint32(106).fork()
      ).ldelim();
    }
    if (message.Order !== undefined) {
      TMusicSlot.encode(message.Order, writer.uint32(114).fork()).ldelim();
    }
    if (message.Repeat !== undefined) {
      TMusicSlot.encode(message.Repeat, writer.uint32(122).fork()).ldelim();
    }
    if (message.DisableAutoflow !== undefined) {
      TBoolSlot.encode(
        message.DisableAutoflow,
        writer.uint32(130).fork()
      ).ldelim();
    }
    if (message.DisableNlg !== undefined) {
      TBoolSlot.encode(message.DisableNlg, writer.uint32(138).fork()).ldelim();
    }
    if (message.PlaySingleTrack !== undefined) {
      TBoolSlot.encode(
        message.PlaySingleTrack,
        writer.uint32(146).fork()
      ).ldelim();
    }
    if (message.TrackOffsetIndex !== undefined) {
      TNumSlot.encode(
        message.TrackOffsetIndex,
        writer.uint32(154).fork()
      ).ldelim();
    }
    if (message.Playlist !== undefined) {
      TStringSlot.encode(message.Playlist, writer.uint32(162).fork()).ldelim();
    }
    if (message.ObjectId !== undefined) {
      TStringSlot.encode(message.ObjectId, writer.uint32(170).fork()).ldelim();
    }
    if (message.ObjectType !== undefined) {
      TMusicPlayObjectTypeSlot.encode(
        message.ObjectType,
        writer.uint32(178).fork()
      ).ldelim();
    }
    if (message.StartFromTrackId !== undefined) {
      TStringSlot.encode(
        message.StartFromTrackId,
        writer.uint32(186).fork()
      ).ldelim();
    }
    if (message.OffsetSec !== undefined) {
      TDoubleSlot.encode(message.OffsetSec, writer.uint32(194).fork()).ldelim();
    }
    if (message.AlarmId !== undefined) {
      TStringSlot.encode(message.AlarmId, writer.uint32(202).fork()).ldelim();
    }
    if (message.From !== undefined) {
      TStringSlot.encode(message.From, writer.uint32(210).fork()).ldelim();
    }
    if (message.DisableHistory !== undefined) {
      TBoolSlot.encode(
        message.DisableHistory,
        writer.uint32(218).fork()
      ).ldelim();
    }
    if (message.Room !== undefined) {
      TMusicSlot.encode(message.Room, writer.uint32(226).fork()).ldelim();
    }
    if (message.Stream !== undefined) {
      TMusicSlot.encode(message.Stream, writer.uint32(234).fork()).ldelim();
    }
    if (message.GenerativeStation !== undefined) {
      TMusicSlot.encode(
        message.GenerativeStation,
        writer.uint32(242).fork()
      ).ldelim();
    }
    if (message.NeedSimilar !== undefined) {
      TMusicSlot.encode(
        message.NeedSimilar,
        writer.uint32(250).fork()
      ).ldelim();
    }
    if (message.Location !== undefined) {
      TLocationSlot.encode(
        message.Location,
        writer.uint32(258).fork()
      ).ldelim();
    }
    if (message.Offset !== undefined) {
      TMusicSlot.encode(message.Offset, writer.uint32(266).fork()).ldelim();
    }
    if (message.DisableMultiroom !== undefined) {
      TBoolSlot.encode(
        message.DisableMultiroom,
        writer.uint32(274).fork()
      ).ldelim();
    }
    if (message.ContentType !== undefined) {
      TMusicContentTypeSlot.encode(
        message.ContentType,
        writer.uint32(282).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TMusicPlaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicPlaySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SpecialPlaylist = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.SpecialAnswerInfo = TStringSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.ActionRequest = TStringSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.Epoch = TStringSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.SearchText = TStringSlot.decode(reader, reader.uint32());
          break;
        case 7:
          message.Genre = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 8:
          message.Mood = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 9:
          message.Activity = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 10:
          message.Language = TLanguageSlot.decode(reader, reader.uint32());
          break;
        case 11:
          message.Vocal = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 12:
          message.Novelty = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 13:
          message.Personality = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 14:
          message.Order = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 15:
          message.Repeat = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 16:
          message.DisableAutoflow = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 17:
          message.DisableNlg = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 18:
          message.PlaySingleTrack = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 19:
          message.TrackOffsetIndex = TNumSlot.decode(reader, reader.uint32());
          break;
        case 20:
          message.Playlist = TStringSlot.decode(reader, reader.uint32());
          break;
        case 21:
          message.ObjectId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 22:
          message.ObjectType = TMusicPlayObjectTypeSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 23:
          message.StartFromTrackId = TStringSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 24:
          message.OffsetSec = TDoubleSlot.decode(reader, reader.uint32());
          break;
        case 25:
          message.AlarmId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 26:
          message.From = TStringSlot.decode(reader, reader.uint32());
          break;
        case 27:
          message.DisableHistory = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 28:
          message.Room = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 29:
          message.Stream = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 30:
          message.GenerativeStation = TMusicSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 31:
          message.NeedSimilar = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 32:
          message.Location = TLocationSlot.decode(reader, reader.uint32());
          break;
        case 33:
          message.Offset = TMusicSlot.decode(reader, reader.uint32());
          break;
        case 34:
          message.DisableMultiroom = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 35:
          message.ContentType = TMusicContentTypeSlot.decode(
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

  fromJSON(object: any): TMusicPlaySemanticFrame {
    return {
      SpecialPlaylist: isSet(object.special_playlist)
        ? TStringSlot.fromJSON(object.special_playlist)
        : undefined,
      SpecialAnswerInfo: isSet(object.special_answer_info)
        ? TStringSlot.fromJSON(object.special_answer_info)
        : undefined,
      ActionRequest: isSet(object.action_request)
        ? TStringSlot.fromJSON(object.action_request)
        : undefined,
      Epoch: isSet(object.epoch)
        ? TStringSlot.fromJSON(object.epoch)
        : undefined,
      SearchText: isSet(object.search_text)
        ? TStringSlot.fromJSON(object.search_text)
        : undefined,
      Genre: isSet(object.genre)
        ? TMusicSlot.fromJSON(object.genre)
        : undefined,
      Mood: isSet(object.mood) ? TMusicSlot.fromJSON(object.mood) : undefined,
      Activity: isSet(object.activity)
        ? TMusicSlot.fromJSON(object.activity)
        : undefined,
      Language: isSet(object.language)
        ? TLanguageSlot.fromJSON(object.language)
        : undefined,
      Vocal: isSet(object.vocal)
        ? TMusicSlot.fromJSON(object.vocal)
        : undefined,
      Novelty: isSet(object.novelty)
        ? TMusicSlot.fromJSON(object.novelty)
        : undefined,
      Personality: isSet(object.personality)
        ? TMusicSlot.fromJSON(object.personality)
        : undefined,
      Order: isSet(object.order)
        ? TMusicSlot.fromJSON(object.order)
        : undefined,
      Repeat: isSet(object.repeat)
        ? TMusicSlot.fromJSON(object.repeat)
        : undefined,
      DisableAutoflow: isSet(object.disable_autoflow)
        ? TBoolSlot.fromJSON(object.disable_autoflow)
        : undefined,
      DisableNlg: isSet(object.disable_nlg)
        ? TBoolSlot.fromJSON(object.disable_nlg)
        : undefined,
      PlaySingleTrack: isSet(object.play_single_track)
        ? TBoolSlot.fromJSON(object.play_single_track)
        : undefined,
      TrackOffsetIndex: isSet(object.track_offset_index)
        ? TNumSlot.fromJSON(object.track_offset_index)
        : undefined,
      Playlist: isSet(object.playlist)
        ? TStringSlot.fromJSON(object.playlist)
        : undefined,
      ObjectId: isSet(object.object_id)
        ? TStringSlot.fromJSON(object.object_id)
        : undefined,
      ObjectType: isSet(object.object_type)
        ? TMusicPlayObjectTypeSlot.fromJSON(object.object_type)
        : undefined,
      StartFromTrackId: isSet(object.start_from_track_id)
        ? TStringSlot.fromJSON(object.start_from_track_id)
        : undefined,
      OffsetSec: isSet(object.offset_sec)
        ? TDoubleSlot.fromJSON(object.offset_sec)
        : undefined,
      AlarmId: isSet(object.alarm_id)
        ? TStringSlot.fromJSON(object.alarm_id)
        : undefined,
      From: isSet(object.from) ? TStringSlot.fromJSON(object.from) : undefined,
      DisableHistory: isSet(object.disable_history)
        ? TBoolSlot.fromJSON(object.disable_history)
        : undefined,
      Room: isSet(object.room) ? TMusicSlot.fromJSON(object.room) : undefined,
      Stream: isSet(object.stream)
        ? TMusicSlot.fromJSON(object.stream)
        : undefined,
      GenerativeStation: isSet(object.generative_station)
        ? TMusicSlot.fromJSON(object.generative_station)
        : undefined,
      NeedSimilar: isSet(object.need_similar)
        ? TMusicSlot.fromJSON(object.need_similar)
        : undefined,
      Location: isSet(object.location)
        ? TLocationSlot.fromJSON(object.location)
        : undefined,
      Offset: isSet(object.offset)
        ? TMusicSlot.fromJSON(object.offset)
        : undefined,
      DisableMultiroom: isSet(object.disable_multiroom)
        ? TBoolSlot.fromJSON(object.disable_multiroom)
        : undefined,
      ContentType: isSet(object.content_type)
        ? TMusicContentTypeSlot.fromJSON(object.content_type)
        : undefined,
    };
  },

  toJSON(message: TMusicPlaySemanticFrame): unknown {
    const obj: any = {};
    message.SpecialPlaylist !== undefined &&
      (obj.special_playlist = message.SpecialPlaylist
        ? TStringSlot.toJSON(message.SpecialPlaylist)
        : undefined);
    message.SpecialAnswerInfo !== undefined &&
      (obj.special_answer_info = message.SpecialAnswerInfo
        ? TStringSlot.toJSON(message.SpecialAnswerInfo)
        : undefined);
    message.ActionRequest !== undefined &&
      (obj.action_request = message.ActionRequest
        ? TStringSlot.toJSON(message.ActionRequest)
        : undefined);
    message.Epoch !== undefined &&
      (obj.epoch = message.Epoch
        ? TStringSlot.toJSON(message.Epoch)
        : undefined);
    message.SearchText !== undefined &&
      (obj.search_text = message.SearchText
        ? TStringSlot.toJSON(message.SearchText)
        : undefined);
    message.Genre !== undefined &&
      (obj.genre = message.Genre
        ? TMusicSlot.toJSON(message.Genre)
        : undefined);
    message.Mood !== undefined &&
      (obj.mood = message.Mood ? TMusicSlot.toJSON(message.Mood) : undefined);
    message.Activity !== undefined &&
      (obj.activity = message.Activity
        ? TMusicSlot.toJSON(message.Activity)
        : undefined);
    message.Language !== undefined &&
      (obj.language = message.Language
        ? TLanguageSlot.toJSON(message.Language)
        : undefined);
    message.Vocal !== undefined &&
      (obj.vocal = message.Vocal
        ? TMusicSlot.toJSON(message.Vocal)
        : undefined);
    message.Novelty !== undefined &&
      (obj.novelty = message.Novelty
        ? TMusicSlot.toJSON(message.Novelty)
        : undefined);
    message.Personality !== undefined &&
      (obj.personality = message.Personality
        ? TMusicSlot.toJSON(message.Personality)
        : undefined);
    message.Order !== undefined &&
      (obj.order = message.Order
        ? TMusicSlot.toJSON(message.Order)
        : undefined);
    message.Repeat !== undefined &&
      (obj.repeat = message.Repeat
        ? TMusicSlot.toJSON(message.Repeat)
        : undefined);
    message.DisableAutoflow !== undefined &&
      (obj.disable_autoflow = message.DisableAutoflow
        ? TBoolSlot.toJSON(message.DisableAutoflow)
        : undefined);
    message.DisableNlg !== undefined &&
      (obj.disable_nlg = message.DisableNlg
        ? TBoolSlot.toJSON(message.DisableNlg)
        : undefined);
    message.PlaySingleTrack !== undefined &&
      (obj.play_single_track = message.PlaySingleTrack
        ? TBoolSlot.toJSON(message.PlaySingleTrack)
        : undefined);
    message.TrackOffsetIndex !== undefined &&
      (obj.track_offset_index = message.TrackOffsetIndex
        ? TNumSlot.toJSON(message.TrackOffsetIndex)
        : undefined);
    message.Playlist !== undefined &&
      (obj.playlist = message.Playlist
        ? TStringSlot.toJSON(message.Playlist)
        : undefined);
    message.ObjectId !== undefined &&
      (obj.object_id = message.ObjectId
        ? TStringSlot.toJSON(message.ObjectId)
        : undefined);
    message.ObjectType !== undefined &&
      (obj.object_type = message.ObjectType
        ? TMusicPlayObjectTypeSlot.toJSON(message.ObjectType)
        : undefined);
    message.StartFromTrackId !== undefined &&
      (obj.start_from_track_id = message.StartFromTrackId
        ? TStringSlot.toJSON(message.StartFromTrackId)
        : undefined);
    message.OffsetSec !== undefined &&
      (obj.offset_sec = message.OffsetSec
        ? TDoubleSlot.toJSON(message.OffsetSec)
        : undefined);
    message.AlarmId !== undefined &&
      (obj.alarm_id = message.AlarmId
        ? TStringSlot.toJSON(message.AlarmId)
        : undefined);
    message.From !== undefined &&
      (obj.from = message.From ? TStringSlot.toJSON(message.From) : undefined);
    message.DisableHistory !== undefined &&
      (obj.disable_history = message.DisableHistory
        ? TBoolSlot.toJSON(message.DisableHistory)
        : undefined);
    message.Room !== undefined &&
      (obj.room = message.Room ? TMusicSlot.toJSON(message.Room) : undefined);
    message.Stream !== undefined &&
      (obj.stream = message.Stream
        ? TMusicSlot.toJSON(message.Stream)
        : undefined);
    message.GenerativeStation !== undefined &&
      (obj.generative_station = message.GenerativeStation
        ? TMusicSlot.toJSON(message.GenerativeStation)
        : undefined);
    message.NeedSimilar !== undefined &&
      (obj.need_similar = message.NeedSimilar
        ? TMusicSlot.toJSON(message.NeedSimilar)
        : undefined);
    message.Location !== undefined &&
      (obj.location = message.Location
        ? TLocationSlot.toJSON(message.Location)
        : undefined);
    message.Offset !== undefined &&
      (obj.offset = message.Offset
        ? TMusicSlot.toJSON(message.Offset)
        : undefined);
    message.DisableMultiroom !== undefined &&
      (obj.disable_multiroom = message.DisableMultiroom
        ? TBoolSlot.toJSON(message.DisableMultiroom)
        : undefined);
    message.ContentType !== undefined &&
      (obj.content_type = message.ContentType
        ? TMusicContentTypeSlot.toJSON(message.ContentType)
        : undefined);
    return obj;
  },
};

function createBaseTMusicOnboardingSemanticFrame(): TMusicOnboardingSemanticFrame {
  return {};
}

export const TMusicOnboardingSemanticFrame = {
  encode(
    _: TMusicOnboardingSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicOnboardingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicOnboardingSemanticFrame();
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

  fromJSON(_: any): TMusicOnboardingSemanticFrame {
    return {};
  },

  toJSON(_: TMusicOnboardingSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMusicOnboardingArtistsSemanticFrame(): TMusicOnboardingArtistsSemanticFrame {
  return {};
}

export const TMusicOnboardingArtistsSemanticFrame = {
  encode(
    _: TMusicOnboardingArtistsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicOnboardingArtistsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicOnboardingArtistsSemanticFrame();
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

  fromJSON(_: any): TMusicOnboardingArtistsSemanticFrame {
    return {};
  },

  toJSON(_: TMusicOnboardingArtistsSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMusicOnboardingGenresSemanticFrame(): TMusicOnboardingGenresSemanticFrame {
  return {};
}

export const TMusicOnboardingGenresSemanticFrame = {
  encode(
    _: TMusicOnboardingGenresSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicOnboardingGenresSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicOnboardingGenresSemanticFrame();
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

  fromJSON(_: any): TMusicOnboardingGenresSemanticFrame {
    return {};
  },

  toJSON(_: TMusicOnboardingGenresSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMusicOnboardingTracksSemanticFrame(): TMusicOnboardingTracksSemanticFrame {
  return {};
}

export const TMusicOnboardingTracksSemanticFrame = {
  encode(
    _: TMusicOnboardingTracksSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicOnboardingTracksSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicOnboardingTracksSemanticFrame();
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

  fromJSON(_: any): TMusicOnboardingTracksSemanticFrame {
    return {};
  },

  toJSON(_: TMusicOnboardingTracksSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMusicOnboardingTracksReaskSemanticFrame(): TMusicOnboardingTracksReaskSemanticFrame {
  return { TrackIndex: undefined, TrackId: undefined };
}

export const TMusicOnboardingTracksReaskSemanticFrame = {
  encode(
    message: TMusicOnboardingTracksReaskSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TrackIndex !== undefined) {
      TNumSlot.encode(message.TrackIndex, writer.uint32(10).fork()).ldelim();
    }
    if (message.TrackId !== undefined) {
      TStringSlot.encode(message.TrackId, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicOnboardingTracksReaskSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicOnboardingTracksReaskSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TrackIndex = TNumSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.TrackId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMusicOnboardingTracksReaskSemanticFrame {
    return {
      TrackIndex: isSet(object.track_index)
        ? TNumSlot.fromJSON(object.track_index)
        : undefined,
      TrackId: isSet(object.track_id)
        ? TStringSlot.fromJSON(object.track_id)
        : undefined,
    };
  },

  toJSON(message: TMusicOnboardingTracksReaskSemanticFrame): unknown {
    const obj: any = {};
    message.TrackIndex !== undefined &&
      (obj.track_index = message.TrackIndex
        ? TNumSlot.toJSON(message.TrackIndex)
        : undefined);
    message.TrackId !== undefined &&
      (obj.track_id = message.TrackId
        ? TStringSlot.toJSON(message.TrackId)
        : undefined);
    return obj;
  },
};

function createBaseTMusicAnnounceDisableSemanticFrame(): TMusicAnnounceDisableSemanticFrame {
  return {};
}

export const TMusicAnnounceDisableSemanticFrame = {
  encode(
    _: TMusicAnnounceDisableSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicAnnounceDisableSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicAnnounceDisableSemanticFrame();
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

  fromJSON(_: any): TMusicAnnounceDisableSemanticFrame {
    return {};
  },

  toJSON(_: TMusicAnnounceDisableSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMusicAnnounceEnableSemanticFrame(): TMusicAnnounceEnableSemanticFrame {
  return {};
}

export const TMusicAnnounceEnableSemanticFrame = {
  encode(
    _: TMusicAnnounceEnableSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicAnnounceEnableSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicAnnounceEnableSemanticFrame();
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

  fromJSON(_: any): TMusicAnnounceEnableSemanticFrame {
    return {};
  },

  toJSON(_: TMusicAnnounceEnableSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTOpenTandemSettingSemanticFrame(): TOpenTandemSettingSemanticFrame {
  return {};
}

export const TOpenTandemSettingSemanticFrame = {
  encode(
    _: TOpenTandemSettingSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOpenTandemSettingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOpenTandemSettingSemanticFrame();
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

  fromJSON(_: any): TOpenTandemSettingSemanticFrame {
    return {};
  },

  toJSON(_: TOpenTandemSettingSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTOpenSmartSpeakerSettingSemanticFrame(): TOpenSmartSpeakerSettingSemanticFrame {
  return {};
}

export const TOpenSmartSpeakerSettingSemanticFrame = {
  encode(
    _: TOpenSmartSpeakerSettingSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOpenSmartSpeakerSettingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOpenSmartSpeakerSettingSemanticFrame();
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

  fromJSON(_: any): TOpenSmartSpeakerSettingSemanticFrame {
    return {};
  },

  toJSON(_: TOpenSmartSpeakerSettingSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTExternalSkillActivateSemanticFrame(): TExternalSkillActivateSemanticFrame {
  return { ActivationPhrase: undefined };
}

export const TExternalSkillActivateSemanticFrame = {
  encode(
    message: TExternalSkillActivateSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ActivationPhrase !== undefined) {
      TStringSlot.encode(
        message.ActivationPhrase,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TExternalSkillActivateSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTExternalSkillActivateSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ActivationPhrase = TStringSlot.decode(
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

  fromJSON(object: any): TExternalSkillActivateSemanticFrame {
    return {
      ActivationPhrase: isSet(object.activation_phrase)
        ? TStringSlot.fromJSON(object.activation_phrase)
        : undefined,
    };
  },

  toJSON(message: TExternalSkillActivateSemanticFrame): unknown {
    const obj: any = {};
    message.ActivationPhrase !== undefined &&
      (obj.activation_phrase = message.ActivationPhrase
        ? TStringSlot.toJSON(message.ActivationPhrase)
        : undefined);
    return obj;
  },
};

function createBaseTExternalSkillEpisodeForShowRequestSemanticFrame(): TExternalSkillEpisodeForShowRequestSemanticFrame {
  return { SkillId: undefined };
}

export const TExternalSkillEpisodeForShowRequestSemanticFrame = {
  encode(
    message: TExternalSkillEpisodeForShowRequestSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SkillId !== undefined) {
      TStringSlot.encode(message.SkillId, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TExternalSkillEpisodeForShowRequestSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTExternalSkillEpisodeForShowRequestSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SkillId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TExternalSkillEpisodeForShowRequestSemanticFrame {
    return {
      SkillId: isSet(object.skill_id)
        ? TStringSlot.fromJSON(object.skill_id)
        : undefined,
    };
  },

  toJSON(message: TExternalSkillEpisodeForShowRequestSemanticFrame): unknown {
    const obj: any = {};
    message.SkillId !== undefined &&
      (obj.skill_id = message.SkillId
        ? TStringSlot.toJSON(message.SkillId)
        : undefined);
    return obj;
  },
};

function createBaseTMusicPlayFixlistSemanticFrame(): TMusicPlayFixlistSemanticFrame {
  return { SpecialAnswerInfo: undefined };
}

export const TMusicPlayFixlistSemanticFrame = {
  encode(
    message: TMusicPlayFixlistSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SpecialAnswerInfo !== undefined) {
      TFixlistInfoSlot.encode(
        message.SpecialAnswerInfo,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicPlayFixlistSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicPlayFixlistSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SpecialAnswerInfo = TFixlistInfoSlot.decode(
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

  fromJSON(object: any): TMusicPlayFixlistSemanticFrame {
    return {
      SpecialAnswerInfo: isSet(object.special_answer_info)
        ? TFixlistInfoSlot.fromJSON(object.special_answer_info)
        : undefined,
    };
  },

  toJSON(message: TMusicPlayFixlistSemanticFrame): unknown {
    const obj: any = {};
    message.SpecialAnswerInfo !== undefined &&
      (obj.special_answer_info = message.SpecialAnswerInfo
        ? TFixlistInfoSlot.toJSON(message.SpecialAnswerInfo)
        : undefined);
    return obj;
  },
};

function createBaseTMusicPlayAnaphoraSemanticFrame(): TMusicPlayAnaphoraSemanticFrame {
  return {
    ActionRequest: undefined,
    Repeat: undefined,
    TargetType: undefined,
    NeedSimilar: undefined,
    Order: undefined,
  };
}

export const TMusicPlayAnaphoraSemanticFrame = {
  encode(
    message: TMusicPlayAnaphoraSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ActionRequest !== undefined) {
      TActionRequestSlot.encode(
        message.ActionRequest,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Repeat !== undefined) {
      TRepeatSlot.encode(message.Repeat, writer.uint32(18).fork()).ldelim();
    }
    if (message.TargetType !== undefined) {
      TTargetTypeSlot.encode(
        message.TargetType,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.NeedSimilar !== undefined) {
      TNeedSimilarSlot.encode(
        message.NeedSimilar,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.Order !== undefined) {
      TOrderSlot.encode(message.Order, writer.uint32(42).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicPlayAnaphoraSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicPlayAnaphoraSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ActionRequest = TActionRequestSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.Repeat = TRepeatSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.TargetType = TTargetTypeSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.NeedSimilar = TNeedSimilarSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.Order = TOrderSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMusicPlayAnaphoraSemanticFrame {
    return {
      ActionRequest: isSet(object.action_request)
        ? TActionRequestSlot.fromJSON(object.action_request)
        : undefined,
      Repeat: isSet(object.repeat)
        ? TRepeatSlot.fromJSON(object.repeat)
        : undefined,
      TargetType: isSet(object.target_type)
        ? TTargetTypeSlot.fromJSON(object.target_type)
        : undefined,
      NeedSimilar: isSet(object.need_similar)
        ? TNeedSimilarSlot.fromJSON(object.need_similar)
        : undefined,
      Order: isSet(object.order)
        ? TOrderSlot.fromJSON(object.order)
        : undefined,
    };
  },

  toJSON(message: TMusicPlayAnaphoraSemanticFrame): unknown {
    const obj: any = {};
    message.ActionRequest !== undefined &&
      (obj.action_request = message.ActionRequest
        ? TActionRequestSlot.toJSON(message.ActionRequest)
        : undefined);
    message.Repeat !== undefined &&
      (obj.repeat = message.Repeat
        ? TRepeatSlot.toJSON(message.Repeat)
        : undefined);
    message.TargetType !== undefined &&
      (obj.target_type = message.TargetType
        ? TTargetTypeSlot.toJSON(message.TargetType)
        : undefined);
    message.NeedSimilar !== undefined &&
      (obj.need_similar = message.NeedSimilar
        ? TNeedSimilarSlot.toJSON(message.NeedSimilar)
        : undefined);
    message.Order !== undefined &&
      (obj.order = message.Order
        ? TOrderSlot.toJSON(message.Order)
        : undefined);
    return obj;
  },
};

function createBaseTMusicPlayFairytaleSemanticFrame(): TMusicPlayFairytaleSemanticFrame {
  return { FairytaleTheme: undefined };
}

export const TMusicPlayFairytaleSemanticFrame = {
  encode(
    message: TMusicPlayFairytaleSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FairytaleTheme !== undefined) {
      TFairytaleThemeSlot.encode(
        message.FairytaleTheme,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMusicPlayFairytaleSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMusicPlayFairytaleSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FairytaleTheme = TFairytaleThemeSlot.decode(
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

  fromJSON(object: any): TMusicPlayFairytaleSemanticFrame {
    return {
      FairytaleTheme: isSet(object.fairytale_theme)
        ? TFairytaleThemeSlot.fromJSON(object.fairytale_theme)
        : undefined,
    };
  },

  toJSON(message: TMusicPlayFairytaleSemanticFrame): unknown {
    const obj: any = {};
    message.FairytaleTheme !== undefined &&
      (obj.fairytale_theme = message.FairytaleTheme
        ? TFairytaleThemeSlot.toJSON(message.FairytaleTheme)
        : undefined);
    return obj;
  },
};

function createBaseTStartMultiroomSemanticFrame(): TStartMultiroomSemanticFrame {
  return {
    LocationRoom: [],
    LocationGroup: [],
    LocationDevice: [],
    LocationEverywhere: undefined,
  };
}

export const TStartMultiroomSemanticFrame = {
  encode(
    message: TStartMultiroomSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    for (const v of message.LocationRoom) {
      TLocationSlot.encode(v!, writer.uint32(10).fork()).ldelim();
    }
    for (const v of message.LocationGroup) {
      TLocationSlot.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    for (const v of message.LocationDevice) {
      TLocationSlot.encode(v!, writer.uint32(26).fork()).ldelim();
    }
    if (message.LocationEverywhere !== undefined) {
      TLocationSlot.encode(
        message.LocationEverywhere,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TStartMultiroomSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTStartMultiroomSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LocationRoom.push(
            TLocationSlot.decode(reader, reader.uint32())
          );
          break;
        case 2:
          message.LocationGroup.push(
            TLocationSlot.decode(reader, reader.uint32())
          );
          break;
        case 3:
          message.LocationDevice.push(
            TLocationSlot.decode(reader, reader.uint32())
          );
          break;
        case 4:
          message.LocationEverywhere = TLocationSlot.decode(
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

  fromJSON(object: any): TStartMultiroomSemanticFrame {
    return {
      LocationRoom: Array.isArray(object?.location_room)
        ? object.location_room.map((e: any) => TLocationSlot.fromJSON(e))
        : [],
      LocationGroup: Array.isArray(object?.location_group)
        ? object.location_group.map((e: any) => TLocationSlot.fromJSON(e))
        : [],
      LocationDevice: Array.isArray(object?.location_device)
        ? object.location_device.map((e: any) => TLocationSlot.fromJSON(e))
        : [],
      LocationEverywhere: isSet(object.location_everywhere)
        ? TLocationSlot.fromJSON(object.location_everywhere)
        : undefined,
    };
  },

  toJSON(message: TStartMultiroomSemanticFrame): unknown {
    const obj: any = {};
    if (message.LocationRoom) {
      obj.location_room = message.LocationRoom.map((e) =>
        e ? TLocationSlot.toJSON(e) : undefined
      );
    } else {
      obj.location_room = [];
    }
    if (message.LocationGroup) {
      obj.location_group = message.LocationGroup.map((e) =>
        e ? TLocationSlot.toJSON(e) : undefined
      );
    } else {
      obj.location_group = [];
    }
    if (message.LocationDevice) {
      obj.location_device = message.LocationDevice.map((e) =>
        e ? TLocationSlot.toJSON(e) : undefined
      );
    } else {
      obj.location_device = [];
    }
    message.LocationEverywhere !== undefined &&
      (obj.location_everywhere = message.LocationEverywhere
        ? TLocationSlot.toJSON(message.LocationEverywhere)
        : undefined);
    return obj;
  },
};

function createBaseTActivationTypedSemanticFrameSlot(): TActivationTypedSemanticFrameSlot {
  return { PutMoneyOnPhoneSemanticFrame: undefined };
}

export const TActivationTypedSemanticFrameSlot = {
  encode(
    message: TActivationTypedSemanticFrameSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.PutMoneyOnPhoneSemanticFrame !== undefined) {
      TPutMoneyOnPhoneSemanticFrame.encode(
        message.PutMoneyOnPhoneSemanticFrame,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TActivationTypedSemanticFrameSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTActivationTypedSemanticFrameSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.PutMoneyOnPhoneSemanticFrame =
            TPutMoneyOnPhoneSemanticFrame.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TActivationTypedSemanticFrameSlot {
    return {
      PutMoneyOnPhoneSemanticFrame: isSet(object.put_money_on_phone_value)
        ? TPutMoneyOnPhoneSemanticFrame.fromJSON(
            object.put_money_on_phone_value
          )
        : undefined,
    };
  },

  toJSON(message: TActivationTypedSemanticFrameSlot): unknown {
    const obj: any = {};
    message.PutMoneyOnPhoneSemanticFrame !== undefined &&
      (obj.put_money_on_phone_value = message.PutMoneyOnPhoneSemanticFrame
        ? TPutMoneyOnPhoneSemanticFrame.toJSON(
            message.PutMoneyOnPhoneSemanticFrame
          )
        : undefined);
    return obj;
  },
};

function createBaseTExternalSkillFixedActivateSemanticFrame(): TExternalSkillFixedActivateSemanticFrame {
  return {
    FixedSkillId: undefined,
    ActivationCommand: undefined,
    Payload: undefined,
    ActivationSourceType: undefined,
    ActivationTypedSemanticFrame: undefined,
  };
}

export const TExternalSkillFixedActivateSemanticFrame = {
  encode(
    message: TExternalSkillFixedActivateSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FixedSkillId !== undefined) {
      TStringSlot.encode(
        message.FixedSkillId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.ActivationCommand !== undefined) {
      TStringSlot.encode(
        message.ActivationCommand,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Payload !== undefined) {
      TStringSlot.encode(message.Payload, writer.uint32(26).fork()).ldelim();
    }
    if (message.ActivationSourceType !== undefined) {
      TActivationSourceTypeSlot.encode(
        message.ActivationSourceType,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.ActivationTypedSemanticFrame !== undefined) {
      TActivationTypedSemanticFrameSlot.encode(
        message.ActivationTypedSemanticFrame,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TExternalSkillFixedActivateSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTExternalSkillFixedActivateSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FixedSkillId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.ActivationCommand = TStringSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.Payload = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.ActivationSourceType = TActivationSourceTypeSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.ActivationTypedSemanticFrame =
            TActivationTypedSemanticFrameSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TExternalSkillFixedActivateSemanticFrame {
    return {
      FixedSkillId: isSet(object.fixed_skill_id)
        ? TStringSlot.fromJSON(object.fixed_skill_id)
        : undefined,
      ActivationCommand: isSet(object.activation_command)
        ? TStringSlot.fromJSON(object.activation_command)
        : undefined,
      Payload: isSet(object.payload)
        ? TStringSlot.fromJSON(object.payload)
        : undefined,
      ActivationSourceType: isSet(object.activation_source_type)
        ? TActivationSourceTypeSlot.fromJSON(object.activation_source_type)
        : undefined,
      ActivationTypedSemanticFrame: isSet(
        object.activation_typed_semantic_frame
      )
        ? TActivationTypedSemanticFrameSlot.fromJSON(
            object.activation_typed_semantic_frame
          )
        : undefined,
    };
  },

  toJSON(message: TExternalSkillFixedActivateSemanticFrame): unknown {
    const obj: any = {};
    message.FixedSkillId !== undefined &&
      (obj.fixed_skill_id = message.FixedSkillId
        ? TStringSlot.toJSON(message.FixedSkillId)
        : undefined);
    message.ActivationCommand !== undefined &&
      (obj.activation_command = message.ActivationCommand
        ? TStringSlot.toJSON(message.ActivationCommand)
        : undefined);
    message.Payload !== undefined &&
      (obj.payload = message.Payload
        ? TStringSlot.toJSON(message.Payload)
        : undefined);
    message.ActivationSourceType !== undefined &&
      (obj.activation_source_type = message.ActivationSourceType
        ? TActivationSourceTypeSlot.toJSON(message.ActivationSourceType)
        : undefined);
    message.ActivationTypedSemanticFrame !== undefined &&
      (obj.activation_typed_semantic_frame =
        message.ActivationTypedSemanticFrame
          ? TActivationTypedSemanticFrameSlot.toJSON(
              message.ActivationTypedSemanticFrame
            )
          : undefined);
    return obj;
  },
};

function createBaseTActivationSourceTypeSlot(): TActivationSourceTypeSlot {
  return { ActivationSourceType: undefined };
}

export const TActivationSourceTypeSlot = {
  encode(
    message: TActivationSourceTypeSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ActivationSourceType !== undefined) {
      writer.uint32(10).string(message.ActivationSourceType);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TActivationSourceTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTActivationSourceTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ActivationSourceType = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TActivationSourceTypeSlot {
    return {
      ActivationSourceType: isSet(object.activation_source_type)
        ? String(object.activation_source_type)
        : undefined,
    };
  },

  toJSON(message: TActivationSourceTypeSlot): unknown {
    const obj: any = {};
    message.ActivationSourceType !== undefined &&
      (obj.activation_source_type = message.ActivationSourceType);
    return obj;
  },
};

function createBaseTVideoPlaySemanticFrame(): TVideoPlaySemanticFrame {
  return {
    ContentType: undefined,
    Action: undefined,
    SearchText: undefined,
    Season: undefined,
    Episode: undefined,
    New: undefined,
  };
}

export const TVideoPlaySemanticFrame = {
  encode(
    message: TVideoPlaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentType !== undefined) {
      TStringSlot.encode(
        message.ContentType,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Action !== undefined) {
      TStringSlot.encode(message.Action, writer.uint32(18).fork()).ldelim();
    }
    if (message.SearchText !== undefined) {
      TStringSlot.encode(message.SearchText, writer.uint32(26).fork()).ldelim();
    }
    if (message.Season !== undefined) {
      TNumSlot.encode(message.Season, writer.uint32(34).fork()).ldelim();
    }
    if (message.Episode !== undefined) {
      TNumSlot.encode(message.Episode, writer.uint32(42).fork()).ldelim();
    }
    if (message.New !== undefined) {
      TVideoSlot.encode(message.New, writer.uint32(50).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TVideoPlaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoPlaySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentType = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Action = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.SearchText = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Season = TNumSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.Episode = TNumSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.New = TVideoSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoPlaySemanticFrame {
    return {
      ContentType: isSet(object.content_type)
        ? TStringSlot.fromJSON(object.content_type)
        : undefined,
      Action: isSet(object.action)
        ? TStringSlot.fromJSON(object.action)
        : undefined,
      SearchText: isSet(object.search_text)
        ? TStringSlot.fromJSON(object.search_text)
        : undefined,
      Season: isSet(object.season)
        ? TNumSlot.fromJSON(object.season)
        : undefined,
      Episode: isSet(object.episode)
        ? TNumSlot.fromJSON(object.episode)
        : undefined,
      New: isSet(object.new) ? TVideoSlot.fromJSON(object.new) : undefined,
    };
  },

  toJSON(message: TVideoPlaySemanticFrame): unknown {
    const obj: any = {};
    message.ContentType !== undefined &&
      (obj.content_type = message.ContentType
        ? TStringSlot.toJSON(message.ContentType)
        : undefined);
    message.Action !== undefined &&
      (obj.action = message.Action
        ? TStringSlot.toJSON(message.Action)
        : undefined);
    message.SearchText !== undefined &&
      (obj.search_text = message.SearchText
        ? TStringSlot.toJSON(message.SearchText)
        : undefined);
    message.Season !== undefined &&
      (obj.season = message.Season
        ? TNumSlot.toJSON(message.Season)
        : undefined);
    message.Episode !== undefined &&
      (obj.episode = message.Episode
        ? TNumSlot.toJSON(message.Episode)
        : undefined);
    message.New !== undefined &&
      (obj.new = message.New ? TVideoSlot.toJSON(message.New) : undefined);
    return obj;
  },
};

function createBaseTWeatherSemanticFrame(): TWeatherSemanticFrame {
  return { When: undefined, CarouselId: undefined, Where: undefined };
}

export const TWeatherSemanticFrame = {
  encode(
    message: TWeatherSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.When !== undefined) {
      TDateTimeSlot.encode(message.When, writer.uint32(10).fork()).ldelim();
    }
    if (message.CarouselId !== undefined) {
      TStringSlot.encode(message.CarouselId, writer.uint32(18).fork()).ldelim();
    }
    if (message.Where !== undefined) {
      TWhereSlot.encode(message.Where, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TWeatherSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWeatherSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.When = TDateTimeSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.CarouselId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Where = TWhereSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TWeatherSemanticFrame {
    return {
      When: isSet(object.when)
        ? TDateTimeSlot.fromJSON(object.when)
        : undefined,
      CarouselId: isSet(object.carousel_id)
        ? TStringSlot.fromJSON(object.carousel_id)
        : undefined,
      Where: isSet(object.where)
        ? TWhereSlot.fromJSON(object.where)
        : undefined,
    };
  },

  toJSON(message: TWeatherSemanticFrame): unknown {
    const obj: any = {};
    message.When !== undefined &&
      (obj.when = message.When
        ? TDateTimeSlot.toJSON(message.When)
        : undefined);
    message.CarouselId !== undefined &&
      (obj.carousel_id = message.CarouselId
        ? TStringSlot.toJSON(message.CarouselId)
        : undefined);
    message.Where !== undefined &&
      (obj.where = message.Where
        ? TWhereSlot.toJSON(message.Where)
        : undefined);
    return obj;
  },
};

function createBaseTHardcodedMorningShowSemanticFrame(): THardcodedMorningShowSemanticFrame {
  return {
    Offset: undefined,
    ShowType: undefined,
    NewsProvider: undefined,
    Topic: undefined,
    NextTrackIndex: undefined,
  };
}

export const THardcodedMorningShowSemanticFrame = {
  encode(
    message: THardcodedMorningShowSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Offset !== undefined) {
      TNumSlot.encode(message.Offset, writer.uint32(10).fork()).ldelim();
    }
    if (message.ShowType !== undefined) {
      TStringSlot.encode(message.ShowType, writer.uint32(18).fork()).ldelim();
    }
    if (message.NewsProvider !== undefined) {
      THardcodedShowSerializedSettingsSlot.encode(
        message.NewsProvider,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Topic !== undefined) {
      THardcodedShowSerializedSettingsSlot.encode(
        message.Topic,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.NextTrackIndex !== undefined) {
      TNumSlot.encode(
        message.NextTrackIndex,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): THardcodedMorningShowSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTHardcodedMorningShowSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Offset = TNumSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.ShowType = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.NewsProvider = THardcodedShowSerializedSettingsSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.Topic = THardcodedShowSerializedSettingsSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.NextTrackIndex = TNumSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): THardcodedMorningShowSemanticFrame {
    return {
      Offset: isSet(object.offset)
        ? TNumSlot.fromJSON(object.offset)
        : undefined,
      ShowType: isSet(object.show_type)
        ? TStringSlot.fromJSON(object.show_type)
        : undefined,
      NewsProvider: isSet(object.news_provider)
        ? THardcodedShowSerializedSettingsSlot.fromJSON(object.news_provider)
        : undefined,
      Topic: isSet(object.topic)
        ? THardcodedShowSerializedSettingsSlot.fromJSON(object.topic)
        : undefined,
      NextTrackIndex: isSet(object.next_track_index)
        ? TNumSlot.fromJSON(object.next_track_index)
        : undefined,
    };
  },

  toJSON(message: THardcodedMorningShowSemanticFrame): unknown {
    const obj: any = {};
    message.Offset !== undefined &&
      (obj.offset = message.Offset
        ? TNumSlot.toJSON(message.Offset)
        : undefined);
    message.ShowType !== undefined &&
      (obj.show_type = message.ShowType
        ? TStringSlot.toJSON(message.ShowType)
        : undefined);
    message.NewsProvider !== undefined &&
      (obj.news_provider = message.NewsProvider
        ? THardcodedShowSerializedSettingsSlot.toJSON(message.NewsProvider)
        : undefined);
    message.Topic !== undefined &&
      (obj.topic = message.Topic
        ? THardcodedShowSerializedSettingsSlot.toJSON(message.Topic)
        : undefined);
    message.NextTrackIndex !== undefined &&
      (obj.next_track_index = message.NextTrackIndex
        ? TNumSlot.toJSON(message.NextTrackIndex)
        : undefined);
    return obj;
  },
};

function createBaseTHardcodedShowSerializedSettingsSlot(): THardcodedShowSerializedSettingsSlot {
  return { SerializedData: undefined };
}

export const THardcodedShowSerializedSettingsSlot = {
  encode(
    message: THardcodedShowSerializedSettingsSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SerializedData !== undefined) {
      writer.uint32(10).string(message.SerializedData);
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): THardcodedShowSerializedSettingsSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTHardcodedShowSerializedSettingsSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SerializedData = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): THardcodedShowSerializedSettingsSlot {
    return {
      SerializedData: isSet(object.data_value)
        ? String(object.data_value)
        : undefined,
    };
  },

  toJSON(message: THardcodedShowSerializedSettingsSlot): unknown {
    const obj: any = {};
    message.SerializedData !== undefined &&
      (obj.data_value = message.SerializedData);
    return obj;
  },
};

function createBaseTAliceShowActivateSemanticFrame(): TAliceShowActivateSemanticFrame {
  return {
    NewsProvider: undefined,
    Topic: undefined,
    DayPart: undefined,
    Age: undefined,
  };
}

export const TAliceShowActivateSemanticFrame = {
  encode(
    message: TAliceShowActivateSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.NewsProvider !== undefined) {
      TNewsProviderSlot.encode(
        message.NewsProvider,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Topic !== undefined) {
      TTopicSlot.encode(message.Topic, writer.uint32(18).fork()).ldelim();
    }
    if (message.DayPart !== undefined) {
      TDayPartSlot.encode(message.DayPart, writer.uint32(26).fork()).ldelim();
    }
    if (message.Age !== undefined) {
      TAgeSlot.encode(message.Age, writer.uint32(34).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAliceShowActivateSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAliceShowActivateSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NewsProvider = TNewsProviderSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.Topic = TTopicSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.DayPart = TDayPartSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Age = TAgeSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAliceShowActivateSemanticFrame {
    return {
      NewsProvider: isSet(object.news_provider)
        ? TNewsProviderSlot.fromJSON(object.news_provider)
        : undefined,
      Topic: isSet(object.topic)
        ? TTopicSlot.fromJSON(object.topic)
        : undefined,
      DayPart: isSet(object.day_part)
        ? TDayPartSlot.fromJSON(object.day_part)
        : undefined,
      Age: isSet(object.age) ? TAgeSlot.fromJSON(object.age) : undefined,
    };
  },

  toJSON(message: TAliceShowActivateSemanticFrame): unknown {
    const obj: any = {};
    message.NewsProvider !== undefined &&
      (obj.news_provider = message.NewsProvider
        ? TNewsProviderSlot.toJSON(message.NewsProvider)
        : undefined);
    message.Topic !== undefined &&
      (obj.topic = message.Topic
        ? TTopicSlot.toJSON(message.Topic)
        : undefined);
    message.DayPart !== undefined &&
      (obj.day_part = message.DayPart
        ? TDayPartSlot.toJSON(message.DayPart)
        : undefined);
    message.Age !== undefined &&
      (obj.age = message.Age ? TAgeSlot.toJSON(message.Age) : undefined);
    return obj;
  },
};

function createBaseTAppsFixlistSemanticFrame(): TAppsFixlistSemanticFrame {
  return { AppData: undefined };
}

export const TAppsFixlistSemanticFrame = {
  encode(
    message: TAppsFixlistSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.AppData !== undefined) {
      TAppData.encode(message.AppData, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAppsFixlistSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAppsFixlistSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.AppData = TAppData.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TAppsFixlistSemanticFrame {
    return {
      AppData: isSet(object.app_data)
        ? TAppData.fromJSON(object.app_data)
        : undefined,
    };
  },

  toJSON(message: TAppsFixlistSemanticFrame): unknown {
    const obj: any = {};
    message.AppData !== undefined &&
      (obj.app_data = message.AppData
        ? TAppData.toJSON(message.AppData)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerNextTrackSemanticFrame(): TPlayerNextTrackSemanticFrame {
  return { SetPause: undefined };
}

export const TPlayerNextTrackSemanticFrame = {
  encode(
    message: TPlayerNextTrackSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SetPause !== undefined) {
      TBoolSlot.encode(message.SetPause, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerNextTrackSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerNextTrackSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SetPause = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerNextTrackSemanticFrame {
    return {
      SetPause: isSet(object.set_pause)
        ? TBoolSlot.fromJSON(object.set_pause)
        : undefined,
    };
  },

  toJSON(message: TPlayerNextTrackSemanticFrame): unknown {
    const obj: any = {};
    message.SetPause !== undefined &&
      (obj.set_pause = message.SetPause
        ? TBoolSlot.toJSON(message.SetPause)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerPrevTrackSemanticFrame(): TPlayerPrevTrackSemanticFrame {
  return { SetPause: undefined };
}

export const TPlayerPrevTrackSemanticFrame = {
  encode(
    message: TPlayerPrevTrackSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SetPause !== undefined) {
      TBoolSlot.encode(message.SetPause, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerPrevTrackSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerPrevTrackSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SetPause = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerPrevTrackSemanticFrame {
    return {
      SetPause: isSet(object.set_pause)
        ? TBoolSlot.fromJSON(object.set_pause)
        : undefined,
    };
  },

  toJSON(message: TPlayerPrevTrackSemanticFrame): unknown {
    const obj: any = {};
    message.SetPause !== undefined &&
      (obj.set_pause = message.SetPause
        ? TBoolSlot.toJSON(message.SetPause)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerLikeSemanticFrame(): TPlayerLikeSemanticFrame {
  return { ContentId: undefined };
}

export const TPlayerLikeSemanticFrame = {
  encode(
    message: TPlayerLikeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentId !== undefined) {
      TContentIdSlot.encode(
        message.ContentId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerLikeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerLikeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentId = TContentIdSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerLikeSemanticFrame {
    return {
      ContentId: isSet(object.content_id)
        ? TContentIdSlot.fromJSON(object.content_id)
        : undefined,
    };
  },

  toJSON(message: TPlayerLikeSemanticFrame): unknown {
    const obj: any = {};
    message.ContentId !== undefined &&
      (obj.content_id = message.ContentId
        ? TContentIdSlot.toJSON(message.ContentId)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerRemoveLikeSemanticFrame(): TPlayerRemoveLikeSemanticFrame {
  return { ContentId: undefined };
}

export const TPlayerRemoveLikeSemanticFrame = {
  encode(
    message: TPlayerRemoveLikeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentId !== undefined) {
      TContentIdSlot.encode(
        message.ContentId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerRemoveLikeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerRemoveLikeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentId = TContentIdSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerRemoveLikeSemanticFrame {
    return {
      ContentId: isSet(object.content_id)
        ? TContentIdSlot.fromJSON(object.content_id)
        : undefined,
    };
  },

  toJSON(message: TPlayerRemoveLikeSemanticFrame): unknown {
    const obj: any = {};
    message.ContentId !== undefined &&
      (obj.content_id = message.ContentId
        ? TContentIdSlot.toJSON(message.ContentId)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerDislikeSemanticFrame(): TPlayerDislikeSemanticFrame {
  return { ContentId: undefined };
}

export const TPlayerDislikeSemanticFrame = {
  encode(
    message: TPlayerDislikeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentId !== undefined) {
      TContentIdSlot.encode(
        message.ContentId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerDislikeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerDislikeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentId = TContentIdSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerDislikeSemanticFrame {
    return {
      ContentId: isSet(object.content_id)
        ? TContentIdSlot.fromJSON(object.content_id)
        : undefined,
    };
  },

  toJSON(message: TPlayerDislikeSemanticFrame): unknown {
    const obj: any = {};
    message.ContentId !== undefined &&
      (obj.content_id = message.ContentId
        ? TContentIdSlot.toJSON(message.ContentId)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerRemoveDislikeSemanticFrame(): TPlayerRemoveDislikeSemanticFrame {
  return { ContentId: undefined };
}

export const TPlayerRemoveDislikeSemanticFrame = {
  encode(
    message: TPlayerRemoveDislikeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentId !== undefined) {
      TContentIdSlot.encode(
        message.ContentId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerRemoveDislikeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerRemoveDislikeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentId = TContentIdSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerRemoveDislikeSemanticFrame {
    return {
      ContentId: isSet(object.content_id)
        ? TContentIdSlot.fromJSON(object.content_id)
        : undefined,
    };
  },

  toJSON(message: TPlayerRemoveDislikeSemanticFrame): unknown {
    const obj: any = {};
    message.ContentId !== undefined &&
      (obj.content_id = message.ContentId
        ? TContentIdSlot.toJSON(message.ContentId)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerContinueSemanticFrame(): TPlayerContinueSemanticFrame {
  return {};
}

export const TPlayerContinueSemanticFrame = {
  encode(
    _: TPlayerContinueSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerContinueSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerContinueSemanticFrame();
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

  fromJSON(_: any): TPlayerContinueSemanticFrame {
    return {};
  },

  toJSON(_: TPlayerContinueSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTPlayerSongsByThisArtistSemanticFrame(): TPlayerSongsByThisArtistSemanticFrame {
  return {};
}

export const TPlayerSongsByThisArtistSemanticFrame = {
  encode(
    _: TPlayerSongsByThisArtistSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerSongsByThisArtistSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerSongsByThisArtistSemanticFrame();
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

  fromJSON(_: any): TPlayerSongsByThisArtistSemanticFrame {
    return {};
  },

  toJSON(_: TPlayerSongsByThisArtistSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTPlayerWhatIsPlayingSemanticFrame(): TPlayerWhatIsPlayingSemanticFrame {
  return {};
}

export const TPlayerWhatIsPlayingSemanticFrame = {
  encode(
    _: TPlayerWhatIsPlayingSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerWhatIsPlayingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerWhatIsPlayingSemanticFrame();
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

  fromJSON(_: any): TPlayerWhatIsPlayingSemanticFrame {
    return {};
  },

  toJSON(_: TPlayerWhatIsPlayingSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTPlayerWhatIsThisSongAboutSemanticFrame(): TPlayerWhatIsThisSongAboutSemanticFrame {
  return {};
}

export const TPlayerWhatIsThisSongAboutSemanticFrame = {
  encode(
    _: TPlayerWhatIsThisSongAboutSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerWhatIsThisSongAboutSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerWhatIsThisSongAboutSemanticFrame();
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

  fromJSON(_: any): TPlayerWhatIsThisSongAboutSemanticFrame {
    return {};
  },

  toJSON(_: TPlayerWhatIsThisSongAboutSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTPlayerShuffleSemanticFrame(): TPlayerShuffleSemanticFrame {
  return { DisableNlg: undefined };
}

export const TPlayerShuffleSemanticFrame = {
  encode(
    message: TPlayerShuffleSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DisableNlg !== undefined) {
      TBoolSlot.encode(message.DisableNlg, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerShuffleSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerShuffleSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DisableNlg = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerShuffleSemanticFrame {
    return {
      DisableNlg: isSet(object.disable_nlg)
        ? TBoolSlot.fromJSON(object.disable_nlg)
        : undefined,
    };
  },

  toJSON(message: TPlayerShuffleSemanticFrame): unknown {
    const obj: any = {};
    message.DisableNlg !== undefined &&
      (obj.disable_nlg = message.DisableNlg
        ? TBoolSlot.toJSON(message.DisableNlg)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerUnshuffleSemanticFrame(): TPlayerUnshuffleSemanticFrame {
  return { DisableNlg: undefined };
}

export const TPlayerUnshuffleSemanticFrame = {
  encode(
    message: TPlayerUnshuffleSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DisableNlg !== undefined) {
      TBoolSlot.encode(message.DisableNlg, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerUnshuffleSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerUnshuffleSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DisableNlg = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerUnshuffleSemanticFrame {
    return {
      DisableNlg: isSet(object.disable_nlg)
        ? TBoolSlot.fromJSON(object.disable_nlg)
        : undefined,
    };
  },

  toJSON(message: TPlayerUnshuffleSemanticFrame): unknown {
    const obj: any = {};
    message.DisableNlg !== undefined &&
      (obj.disable_nlg = message.DisableNlg
        ? TBoolSlot.toJSON(message.DisableNlg)
        : undefined);
    return obj;
  },
};

function createBaseTPlayerReplaySemanticFrame(): TPlayerReplaySemanticFrame {
  return {};
}

export const TPlayerReplaySemanticFrame = {
  encode(
    _: TPlayerReplaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerReplaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerReplaySemanticFrame();
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

  fromJSON(_: any): TPlayerReplaySemanticFrame {
    return {};
  },

  toJSON(_: TPlayerReplaySemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTPlayerPauseSemanticFrame(): TPlayerPauseSemanticFrame {
  return {};
}

export const TPlayerPauseSemanticFrame = {
  encode(
    _: TPlayerPauseSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerPauseSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerPauseSemanticFrame();
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

  fromJSON(_: any): TPlayerPauseSemanticFrame {
    return {};
  },

  toJSON(_: TPlayerPauseSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTRemindersCancelSemanticFrame(): TRemindersCancelSemanticFrame {
  return { Ids: undefined };
}

export const TRemindersCancelSemanticFrame = {
  encode(
    message: TRemindersCancelSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Ids !== undefined) {
      TStringSlot.encode(message.Ids, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRemindersCancelSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRemindersCancelSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Ids = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRemindersCancelSemanticFrame {
    return {
      Ids: isSet(object.ids) ? TStringSlot.fromJSON(object.ids) : undefined,
    };
  },

  toJSON(message: TRemindersCancelSemanticFrame): unknown {
    const obj: any = {};
    message.Ids !== undefined &&
      (obj.ids = message.Ids ? TStringSlot.toJSON(message.Ids) : undefined);
    return obj;
  },
};

function createBaseTRemindersListSemanticFrame(): TRemindersListSemanticFrame {
  return { Countdown: undefined };
}

export const TRemindersListSemanticFrame = {
  encode(
    message: TRemindersListSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Countdown !== undefined) {
      TUInt32Slot.encode(message.Countdown, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRemindersListSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRemindersListSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Countdown = TUInt32Slot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRemindersListSemanticFrame {
    return {
      Countdown: isSet(object.countdown)
        ? TUInt32Slot.fromJSON(object.countdown)
        : undefined,
    };
  },

  toJSON(message: TRemindersListSemanticFrame): unknown {
    const obj: any = {};
    message.Countdown !== undefined &&
      (obj.countdown = message.Countdown
        ? TUInt32Slot.toJSON(message.Countdown)
        : undefined);
    return obj;
  },
};

function createBaseTRemindersOnShootSemanticFrame(): TRemindersOnShootSemanticFrame {
  return {
    Id: undefined,
    Text: undefined,
    Epoch: undefined,
    TimeZone: undefined,
    OriginDeviceId: undefined,
  };
}

export const TRemindersOnShootSemanticFrame = {
  encode(
    message: TRemindersOnShootSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== undefined) {
      TStringSlot.encode(message.Id, writer.uint32(10).fork()).ldelim();
    }
    if (message.Text !== undefined) {
      TStringSlot.encode(message.Text, writer.uint32(18).fork()).ldelim();
    }
    if (message.Epoch !== undefined) {
      TStringSlot.encode(message.Epoch, writer.uint32(26).fork()).ldelim();
    }
    if (message.TimeZone !== undefined) {
      TStringSlot.encode(message.TimeZone, writer.uint32(34).fork()).ldelim();
    }
    if (message.OriginDeviceId !== undefined) {
      TStringSlot.encode(
        message.OriginDeviceId,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRemindersOnShootSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRemindersOnShootSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Text = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Epoch = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.TimeZone = TStringSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.OriginDeviceId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRemindersOnShootSemanticFrame {
    return {
      Id: isSet(object.id) ? TStringSlot.fromJSON(object.id) : undefined,
      Text: isSet(object.text) ? TStringSlot.fromJSON(object.text) : undefined,
      Epoch: isSet(object.epoch)
        ? TStringSlot.fromJSON(object.epoch)
        : undefined,
      TimeZone: isSet(object.timezone)
        ? TStringSlot.fromJSON(object.timezone)
        : undefined,
      OriginDeviceId: isSet(object.origin_device_id)
        ? TStringSlot.fromJSON(object.origin_device_id)
        : undefined,
    };
  },

  toJSON(message: TRemindersOnShootSemanticFrame): unknown {
    const obj: any = {};
    message.Id !== undefined &&
      (obj.id = message.Id ? TStringSlot.toJSON(message.Id) : undefined);
    message.Text !== undefined &&
      (obj.text = message.Text ? TStringSlot.toJSON(message.Text) : undefined);
    message.Epoch !== undefined &&
      (obj.epoch = message.Epoch
        ? TStringSlot.toJSON(message.Epoch)
        : undefined);
    message.TimeZone !== undefined &&
      (obj.timezone = message.TimeZone
        ? TStringSlot.toJSON(message.TimeZone)
        : undefined);
    message.OriginDeviceId !== undefined &&
      (obj.origin_device_id = message.OriginDeviceId
        ? TStringSlot.toJSON(message.OriginDeviceId)
        : undefined);
    return obj;
  },
};

function createBaseTRewindTypeSlot(): TRewindTypeSlot {
  return { StringValue: undefined, RewindTypeValue: undefined };
}

export const TRewindTypeSlot = {
  encode(message: TRewindTypeSlot, writer: Writer = Writer.create()): Writer {
    if (message.StringValue !== undefined) {
      writer.uint32(10).string(message.StringValue);
    }
    if (message.RewindTypeValue !== undefined) {
      writer.uint32(18).string(message.RewindTypeValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TRewindTypeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRewindTypeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.StringValue = reader.string();
          break;
        case 2:
          message.RewindTypeValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRewindTypeSlot {
    return {
      StringValue: isSet(object.string_value)
        ? String(object.string_value)
        : undefined,
      RewindTypeValue: isSet(object.rewind_type_value)
        ? String(object.rewind_type_value)
        : undefined,
    };
  },

  toJSON(message: TRewindTypeSlot): unknown {
    const obj: any = {};
    message.StringValue !== undefined &&
      (obj.string_value = message.StringValue);
    message.RewindTypeValue !== undefined &&
      (obj.rewind_type_value = message.RewindTypeValue);
    return obj;
  },
};

function createBaseTPlayerRewindSemanticFrame(): TPlayerRewindSemanticFrame {
  return { Time: undefined, RewindType: undefined };
}

export const TPlayerRewindSemanticFrame = {
  encode(
    message: TPlayerRewindSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Time !== undefined) {
      TUnitsTimeSlot.encode(message.Time, writer.uint32(10).fork()).ldelim();
    }
    if (message.RewindType !== undefined) {
      TRewindTypeSlot.encode(
        message.RewindType,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerRewindSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerRewindSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Time = TUnitsTimeSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.RewindType = TRewindTypeSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerRewindSemanticFrame {
    return {
      Time: isSet(object.time)
        ? TUnitsTimeSlot.fromJSON(object.time)
        : undefined,
      RewindType: isSet(object.rewind_type)
        ? TRewindTypeSlot.fromJSON(object.rewind_type)
        : undefined,
    };
  },

  toJSON(message: TPlayerRewindSemanticFrame): unknown {
    const obj: any = {};
    message.Time !== undefined &&
      (obj.time = message.Time
        ? TUnitsTimeSlot.toJSON(message.Time)
        : undefined);
    message.RewindType !== undefined &&
      (obj.rewind_type = message.RewindType
        ? TRewindTypeSlot.toJSON(message.RewindType)
        : undefined);
    return obj;
  },
};

function createBaseTRepeatModeSlot(): TRepeatModeSlot {
  return { EnumValue: undefined };
}

export const TRepeatModeSlot = {
  encode(message: TRepeatModeSlot, writer: Writer = Writer.create()): Writer {
    if (message.EnumValue !== undefined) {
      writer.uint32(8).int32(message.EnumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TRepeatModeSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRepeatModeSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnumValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRepeatModeSlot {
    return {
      EnumValue: isSet(object.enum_value)
        ? tRepeatModeSlot_EValueFromJSON(object.enum_value)
        : undefined,
    };
  },

  toJSON(message: TRepeatModeSlot): unknown {
    const obj: any = {};
    message.EnumValue !== undefined &&
      (obj.enum_value =
        message.EnumValue !== undefined
          ? tRepeatModeSlot_EValueToJSON(message.EnumValue)
          : undefined);
    return obj;
  },
};

function createBaseTPlayerRepeatSemanticFrame(): TPlayerRepeatSemanticFrame {
  return { DisableNlg: undefined, Mode: undefined };
}

export const TPlayerRepeatSemanticFrame = {
  encode(
    message: TPlayerRepeatSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DisableNlg !== undefined) {
      TBoolSlot.encode(message.DisableNlg, writer.uint32(10).fork()).ldelim();
    }
    if (message.Mode !== undefined) {
      TRepeatModeSlot.encode(message.Mode, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TPlayerRepeatSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPlayerRepeatSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DisableNlg = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Mode = TRepeatModeSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TPlayerRepeatSemanticFrame {
    return {
      DisableNlg: isSet(object.disable_nlg)
        ? TBoolSlot.fromJSON(object.disable_nlg)
        : undefined,
      Mode: isSet(object.mode)
        ? TRepeatModeSlot.fromJSON(object.mode)
        : undefined,
    };
  },

  toJSON(message: TPlayerRepeatSemanticFrame): unknown {
    const obj: any = {};
    message.DisableNlg !== undefined &&
      (obj.disable_nlg = message.DisableNlg
        ? TBoolSlot.toJSON(message.DisableNlg)
        : undefined);
    message.Mode !== undefined &&
      (obj.mode = message.Mode
        ? TRepeatModeSlot.toJSON(message.Mode)
        : undefined);
    return obj;
  },
};

function createBaseTDoNothingSemanticFrame(): TDoNothingSemanticFrame {
  return {};
}

export const TDoNothingSemanticFrame = {
  encode(_: TDoNothingSemanticFrame, writer: Writer = Writer.create()): Writer {
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TDoNothingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTDoNothingSemanticFrame();
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

  fromJSON(_: any): TDoNothingSemanticFrame {
    return {};
  },

  toJSON(_: TDoNothingSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTNotificationsSubscribeSemanticFrame(): TNotificationsSubscribeSemanticFrame {
  return { Accept: undefined, NotificationSubscription: undefined };
}

export const TNotificationsSubscribeSemanticFrame = {
  encode(
    message: TNotificationsSubscribeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Accept !== undefined) {
      TStringSlot.encode(message.Accept, writer.uint32(10).fork()).ldelim();
    }
    if (message.NotificationSubscription !== undefined) {
      TNotificationSlot.encode(
        message.NotificationSubscription,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TNotificationsSubscribeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTNotificationsSubscribeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Accept = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.NotificationSubscription = TNotificationSlot.decode(
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

  fromJSON(object: any): TNotificationsSubscribeSemanticFrame {
    return {
      Accept: isSet(object.accept)
        ? TStringSlot.fromJSON(object.accept)
        : undefined,
      NotificationSubscription: isSet(object.notification_subscription)
        ? TNotificationSlot.fromJSON(object.notification_subscription)
        : undefined,
    };
  },

  toJSON(message: TNotificationsSubscribeSemanticFrame): unknown {
    const obj: any = {};
    message.Accept !== undefined &&
      (obj.accept = message.Accept
        ? TStringSlot.toJSON(message.Accept)
        : undefined);
    message.NotificationSubscription !== undefined &&
      (obj.notification_subscription = message.NotificationSubscription
        ? TNotificationSlot.toJSON(message.NotificationSubscription)
        : undefined);
    return obj;
  },
};

function createBaseTVideoRaterSemanticFrame(): TVideoRaterSemanticFrame {
  return {};
}

export const TVideoRaterSemanticFrame = {
  encode(
    _: TVideoRaterSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoRaterSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoRaterSemanticFrame();
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

  fromJSON(_: any): TVideoRaterSemanticFrame {
    return {};
  },

  toJSON(_: TVideoRaterSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTSetupRcuStatusSemanticFrame(): TSetupRcuStatusSemanticFrame {
  return { Status: undefined };
}

export const TSetupRcuStatusSemanticFrame = {
  encode(
    message: TSetupRcuStatusSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Status !== undefined) {
      TSetupRcuStatusSlot.encode(
        message.Status,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSetupRcuStatusSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuStatusSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Status = TSetupRcuStatusSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSetupRcuStatusSemanticFrame {
    return {
      Status: isSet(object.status)
        ? TSetupRcuStatusSlot.fromJSON(object.status)
        : undefined,
    };
  },

  toJSON(message: TSetupRcuStatusSemanticFrame): unknown {
    const obj: any = {};
    message.Status !== undefined &&
      (obj.status = message.Status
        ? TSetupRcuStatusSlot.toJSON(message.Status)
        : undefined);
    return obj;
  },
};

function createBaseTSetupRcuAutoStatusSemanticFrame(): TSetupRcuAutoStatusSemanticFrame {
  return { Status: undefined };
}

export const TSetupRcuAutoStatusSemanticFrame = {
  encode(
    message: TSetupRcuAutoStatusSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Status !== undefined) {
      TSetupRcuStatusSlot.encode(
        message.Status,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSetupRcuAutoStatusSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuAutoStatusSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Status = TSetupRcuStatusSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSetupRcuAutoStatusSemanticFrame {
    return {
      Status: isSet(object.status)
        ? TSetupRcuStatusSlot.fromJSON(object.status)
        : undefined,
    };
  },

  toJSON(message: TSetupRcuAutoStatusSemanticFrame): unknown {
    const obj: any = {};
    message.Status !== undefined &&
      (obj.status = message.Status
        ? TSetupRcuStatusSlot.toJSON(message.Status)
        : undefined);
    return obj;
  },
};

function createBaseTSetupRcuCheckStatusSemanticFrame(): TSetupRcuCheckStatusSemanticFrame {
  return { Status: undefined };
}

export const TSetupRcuCheckStatusSemanticFrame = {
  encode(
    message: TSetupRcuCheckStatusSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Status !== undefined) {
      TSetupRcuStatusSlot.encode(
        message.Status,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSetupRcuCheckStatusSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuCheckStatusSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Status = TSetupRcuStatusSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSetupRcuCheckStatusSemanticFrame {
    return {
      Status: isSet(object.status)
        ? TSetupRcuStatusSlot.fromJSON(object.status)
        : undefined,
    };
  },

  toJSON(message: TSetupRcuCheckStatusSemanticFrame): unknown {
    const obj: any = {};
    message.Status !== undefined &&
      (obj.status = message.Status
        ? TSetupRcuStatusSlot.toJSON(message.Status)
        : undefined);
    return obj;
  },
};

function createBaseTSetupRcuAdvancedStatusSemanticFrame(): TSetupRcuAdvancedStatusSemanticFrame {
  return { Status: undefined };
}

export const TSetupRcuAdvancedStatusSemanticFrame = {
  encode(
    message: TSetupRcuAdvancedStatusSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Status !== undefined) {
      TSetupRcuStatusSlot.encode(
        message.Status,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSetupRcuAdvancedStatusSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuAdvancedStatusSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Status = TSetupRcuStatusSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSetupRcuAdvancedStatusSemanticFrame {
    return {
      Status: isSet(object.status)
        ? TSetupRcuStatusSlot.fromJSON(object.status)
        : undefined,
    };
  },

  toJSON(message: TSetupRcuAdvancedStatusSemanticFrame): unknown {
    const obj: any = {};
    message.Status !== undefined &&
      (obj.status = message.Status
        ? TSetupRcuStatusSlot.toJSON(message.Status)
        : undefined);
    return obj;
  },
};

function createBaseTSetupRcuManualStartSemanticFrame(): TSetupRcuManualStartSemanticFrame {
  return {};
}

export const TSetupRcuManualStartSemanticFrame = {
  encode(
    _: TSetupRcuManualStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSetupRcuManualStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuManualStartSemanticFrame();
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

  fromJSON(_: any): TSetupRcuManualStartSemanticFrame {
    return {};
  },

  toJSON(_: TSetupRcuManualStartSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTSetupRcuAutoStartSemanticFrame(): TSetupRcuAutoStartSemanticFrame {
  return { TvModel: undefined };
}

export const TSetupRcuAutoStartSemanticFrame = {
  encode(
    message: TSetupRcuAutoStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TvModel !== undefined) {
      TStringSlot.encode(message.TvModel, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSetupRcuAutoStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuAutoStartSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TvModel = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSetupRcuAutoStartSemanticFrame {
    return {
      TvModel: isSet(object.tv_model)
        ? TStringSlot.fromJSON(object.tv_model)
        : undefined,
    };
  },

  toJSON(message: TSetupRcuAutoStartSemanticFrame): unknown {
    const obj: any = {};
    message.TvModel !== undefined &&
      (obj.tv_model = message.TvModel
        ? TStringSlot.toJSON(message.TvModel)
        : undefined);
    return obj;
  },
};

function createBaseTSetupRcuStatusSlot(): TSetupRcuStatusSlot {
  return { EnumValue: undefined };
}

export const TSetupRcuStatusSlot = {
  encode(
    message: TSetupRcuStatusSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EnumValue !== undefined) {
      writer.uint32(8).int32(message.EnumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSetupRcuStatusSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSetupRcuStatusSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnumValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSetupRcuStatusSlot {
    return {
      EnumValue: isSet(object.enum_value)
        ? tSetupRcuStatusSlot_EValueFromJSON(object.enum_value)
        : undefined,
    };
  },

  toJSON(message: TSetupRcuStatusSlot): unknown {
    const obj: any = {};
    message.EnumValue !== undefined &&
      (obj.enum_value =
        message.EnumValue !== undefined
          ? tSetupRcuStatusSlot_EValueToJSON(message.EnumValue)
          : undefined);
    return obj;
  },
};

function createBaseTLinkARemoteSemanticFrame(): TLinkARemoteSemanticFrame {
  return { LinkType: undefined };
}

export const TLinkARemoteSemanticFrame = {
  encode(
    message: TLinkARemoteSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.LinkType !== undefined) {
      TStringSlot.encode(message.LinkType, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TLinkARemoteSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTLinkARemoteSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LinkType = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TLinkARemoteSemanticFrame {
    return {
      LinkType: isSet(object.link_type)
        ? TStringSlot.fromJSON(object.link_type)
        : undefined,
    };
  },

  toJSON(message: TLinkARemoteSemanticFrame): unknown {
    const obj: any = {};
    message.LinkType !== undefined &&
      (obj.link_type = message.LinkType
        ? TStringSlot.toJSON(message.LinkType)
        : undefined);
    return obj;
  },
};

function createBaseTRequestTechnicalSupportSemanticFrame(): TRequestTechnicalSupportSemanticFrame {
  return {};
}

export const TRequestTechnicalSupportSemanticFrame = {
  encode(
    _: TRequestTechnicalSupportSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRequestTechnicalSupportSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRequestTechnicalSupportSemanticFrame();
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

  fromJSON(_: any): TRequestTechnicalSupportSemanticFrame {
    return {};
  },

  toJSON(_: TRequestTechnicalSupportSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTOnboardingStartingCriticalUpdateSemanticFrame(): TOnboardingStartingCriticalUpdateSemanticFrame {
  return { IsFirstSetup: undefined };
}

export const TOnboardingStartingCriticalUpdateSemanticFrame = {
  encode(
    message: TOnboardingStartingCriticalUpdateSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.IsFirstSetup !== undefined) {
      TBoolSlot.encode(message.IsFirstSetup, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOnboardingStartingCriticalUpdateSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnboardingStartingCriticalUpdateSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.IsFirstSetup = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOnboardingStartingCriticalUpdateSemanticFrame {
    return {
      IsFirstSetup: isSet(object.is_first_setup)
        ? TBoolSlot.fromJSON(object.is_first_setup)
        : undefined,
    };
  },

  toJSON(message: TOnboardingStartingCriticalUpdateSemanticFrame): unknown {
    const obj: any = {};
    message.IsFirstSetup !== undefined &&
      (obj.is_first_setup = message.IsFirstSetup
        ? TBoolSlot.toJSON(message.IsFirstSetup)
        : undefined);
    return obj;
  },
};

function createBaseTOnboardingStartingConfigureSuccessSemanticFrame(): TOnboardingStartingConfigureSuccessSemanticFrame {
  return {};
}

export const TOnboardingStartingConfigureSuccessSemanticFrame = {
  encode(
    _: TOnboardingStartingConfigureSuccessSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOnboardingStartingConfigureSuccessSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message =
      createBaseTOnboardingStartingConfigureSuccessSemanticFrame();
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

  fromJSON(_: any): TOnboardingStartingConfigureSuccessSemanticFrame {
    return {};
  },

  toJSON(_: TOnboardingStartingConfigureSuccessSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTFmRadioStationSlot(): TFmRadioStationSlot {
  return { FmRadioValue: undefined };
}

export const TFmRadioStationSlot = {
  encode(
    message: TFmRadioStationSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FmRadioValue !== undefined) {
      writer.uint32(10).string(message.FmRadioValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFmRadioStationSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFmRadioStationSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FmRadioValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFmRadioStationSlot {
    return {
      FmRadioValue: isSet(object.fm_radio_value)
        ? String(object.fm_radio_value)
        : undefined,
    };
  },

  toJSON(message: TFmRadioStationSlot): unknown {
    const obj: any = {};
    message.FmRadioValue !== undefined &&
      (obj.fm_radio_value = message.FmRadioValue);
    return obj;
  },
};

function createBaseTFmRadioFreqSlot(): TFmRadioFreqSlot {
  return { FmRadioFreqValue: undefined };
}

export const TFmRadioFreqSlot = {
  encode(message: TFmRadioFreqSlot, writer: Writer = Writer.create()): Writer {
    if (message.FmRadioFreqValue !== undefined) {
      writer.uint32(10).string(message.FmRadioFreqValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFmRadioFreqSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFmRadioFreqSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FmRadioFreqValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFmRadioFreqSlot {
    return {
      FmRadioFreqValue: isSet(object.fm_radio_freq_value)
        ? String(object.fm_radio_freq_value)
        : undefined,
    };
  },

  toJSON(message: TFmRadioFreqSlot): unknown {
    const obj: any = {};
    message.FmRadioFreqValue !== undefined &&
      (obj.fm_radio_freq_value = message.FmRadioFreqValue);
    return obj;
  },
};

function createBaseTFmRadioInfoSlot(): TFmRadioInfoSlot {
  return { FmRadioInfoValue: undefined };
}

export const TFmRadioInfoSlot = {
  encode(message: TFmRadioInfoSlot, writer: Writer = Writer.create()): Writer {
    if (message.FmRadioInfoValue !== undefined) {
      TFmRadioInfo.encode(
        message.FmRadioInfoValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TFmRadioInfoSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFmRadioInfoSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FmRadioInfoValue = TFmRadioInfo.decode(
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

  fromJSON(object: any): TFmRadioInfoSlot {
    return {
      FmRadioInfoValue: isSet(object.fm_radio_info_value)
        ? TFmRadioInfo.fromJSON(object.fm_radio_info_value)
        : undefined,
    };
  },

  toJSON(message: TFmRadioInfoSlot): unknown {
    const obj: any = {};
    message.FmRadioInfoValue !== undefined &&
      (obj.fm_radio_info_value = message.FmRadioInfoValue
        ? TFmRadioInfo.toJSON(message.FmRadioInfoValue)
        : undefined);
    return obj;
  },
};

function createBaseTRadioPlaySemanticFrame(): TRadioPlaySemanticFrame {
  return {
    FmRadioStation: undefined,
    FmRadioFreq: undefined,
    FmRadioInfo: undefined,
    DisableNlg: undefined,
  };
}

export const TRadioPlaySemanticFrame = {
  encode(
    message: TRadioPlaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FmRadioStation !== undefined) {
      TFmRadioStationSlot.encode(
        message.FmRadioStation,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.FmRadioFreq !== undefined) {
      TFmRadioFreqSlot.encode(
        message.FmRadioFreq,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.FmRadioInfo !== undefined) {
      TFmRadioInfoSlot.encode(
        message.FmRadioInfo,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.DisableNlg !== undefined) {
      TBoolSlot.encode(message.DisableNlg, writer.uint32(34).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TRadioPlaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRadioPlaySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FmRadioStation = TFmRadioStationSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.FmRadioFreq = TFmRadioFreqSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.FmRadioInfo = TFmRadioInfoSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 4:
          message.DisableNlg = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRadioPlaySemanticFrame {
    return {
      FmRadioStation: isSet(object.fm_radio)
        ? TFmRadioStationSlot.fromJSON(object.fm_radio)
        : undefined,
      FmRadioFreq: isSet(object.fm_radio_freq)
        ? TFmRadioFreqSlot.fromJSON(object.fm_radio_freq)
        : undefined,
      FmRadioInfo: isSet(object.fm_radio_info)
        ? TFmRadioInfoSlot.fromJSON(object.fm_radio_info)
        : undefined,
      DisableNlg: isSet(object.disable_nlg)
        ? TBoolSlot.fromJSON(object.disable_nlg)
        : undefined,
    };
  },

  toJSON(message: TRadioPlaySemanticFrame): unknown {
    const obj: any = {};
    message.FmRadioStation !== undefined &&
      (obj.fm_radio = message.FmRadioStation
        ? TFmRadioStationSlot.toJSON(message.FmRadioStation)
        : undefined);
    message.FmRadioFreq !== undefined &&
      (obj.fm_radio_freq = message.FmRadioFreq
        ? TFmRadioFreqSlot.toJSON(message.FmRadioFreq)
        : undefined);
    message.FmRadioInfo !== undefined &&
      (obj.fm_radio_info = message.FmRadioInfo
        ? TFmRadioInfoSlot.toJSON(message.FmRadioInfo)
        : undefined);
    message.DisableNlg !== undefined &&
      (obj.disable_nlg = message.DisableNlg
        ? TBoolSlot.toJSON(message.DisableNlg)
        : undefined);
    return obj;
  },
};

function createBaseTFmRadioPlaySemanticFrame(): TFmRadioPlaySemanticFrame {
  return {
    FmRadioStation: undefined,
    FmRadioFreq: undefined,
    DisableNlg: undefined,
  };
}

export const TFmRadioPlaySemanticFrame = {
  encode(
    message: TFmRadioPlaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FmRadioStation !== undefined) {
      TFmRadioStationSlot.encode(
        message.FmRadioStation,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.FmRadioFreq !== undefined) {
      TFmRadioFreqSlot.encode(
        message.FmRadioFreq,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.DisableNlg !== undefined) {
      TBoolSlot.encode(message.DisableNlg, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TFmRadioPlaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTFmRadioPlaySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FmRadioStation = TFmRadioStationSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.FmRadioFreq = TFmRadioFreqSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.DisableNlg = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TFmRadioPlaySemanticFrame {
    return {
      FmRadioStation: isSet(object.fm_radio)
        ? TFmRadioStationSlot.fromJSON(object.fm_radio)
        : undefined,
      FmRadioFreq: isSet(object.fm_radio_freq)
        ? TFmRadioFreqSlot.fromJSON(object.fm_radio_freq)
        : undefined,
      DisableNlg: isSet(object.disable_nlg)
        ? TBoolSlot.fromJSON(object.disable_nlg)
        : undefined,
    };
  },

  toJSON(message: TFmRadioPlaySemanticFrame): unknown {
    const obj: any = {};
    message.FmRadioStation !== undefined &&
      (obj.fm_radio = message.FmRadioStation
        ? TFmRadioStationSlot.toJSON(message.FmRadioStation)
        : undefined);
    message.FmRadioFreq !== undefined &&
      (obj.fm_radio_freq = message.FmRadioFreq
        ? TFmRadioFreqSlot.toJSON(message.FmRadioFreq)
        : undefined);
    message.DisableNlg !== undefined &&
      (obj.disable_nlg = message.DisableNlg
        ? TBoolSlot.toJSON(message.DisableNlg)
        : undefined);
    return obj;
  },
};

function createBaseTSoundLouderSemanticFrame(): TSoundLouderSemanticFrame {
  return {};
}

export const TSoundLouderSemanticFrame = {
  encode(
    _: TSoundLouderSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSoundLouderSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSoundLouderSemanticFrame();
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

  fromJSON(_: any): TSoundLouderSemanticFrame {
    return {};
  },

  toJSON(_: TSoundLouderSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTSoundQuiterSemanticFrame(): TSoundQuiterSemanticFrame {
  return {};
}

export const TSoundQuiterSemanticFrame = {
  encode(
    _: TSoundQuiterSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSoundQuiterSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSoundQuiterSemanticFrame();
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

  fromJSON(_: any): TSoundQuiterSemanticFrame {
    return {};
  },

  toJSON(_: TSoundQuiterSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTSoundLevelSlot(): TSoundLevelSlot {
  return {
    NumLevelValue: undefined,
    FloatLevelValue: undefined,
    CustomLevelValue: undefined,
  };
}

export const TSoundLevelSlot = {
  encode(message: TSoundLevelSlot, writer: Writer = Writer.create()): Writer {
    if (message.NumLevelValue !== undefined) {
      writer.uint32(8).int32(message.NumLevelValue);
    }
    if (message.FloatLevelValue !== undefined) {
      writer.uint32(21).float(message.FloatLevelValue);
    }
    if (message.CustomLevelValue !== undefined) {
      writer.uint32(26).string(message.CustomLevelValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSoundLevelSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSoundLevelSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.NumLevelValue = reader.int32();
          break;
        case 2:
          message.FloatLevelValue = reader.float();
          break;
        case 3:
          message.CustomLevelValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSoundLevelSlot {
    return {
      NumLevelValue: isSet(object.num_level_value)
        ? Number(object.num_level_value)
        : undefined,
      FloatLevelValue: isSet(object.float_level_value)
        ? Number(object.float_level_value)
        : undefined,
      CustomLevelValue: isSet(object.custom_level_value)
        ? String(object.custom_level_value)
        : undefined,
    };
  },

  toJSON(message: TSoundLevelSlot): unknown {
    const obj: any = {};
    message.NumLevelValue !== undefined &&
      (obj.num_level_value = Math.round(message.NumLevelValue));
    message.FloatLevelValue !== undefined &&
      (obj.float_level_value = message.FloatLevelValue);
    message.CustomLevelValue !== undefined &&
      (obj.custom_level_value = message.CustomLevelValue);
    return obj;
  },
};

function createBaseTSoundSetLevelSemanticFrame(): TSoundSetLevelSemanticFrame {
  return { Level: undefined };
}

export const TSoundSetLevelSemanticFrame = {
  encode(
    message: TSoundSetLevelSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Level !== undefined) {
      TSoundLevelSlot.encode(message.Level, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSoundSetLevelSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSoundSetLevelSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Level = TSoundLevelSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSoundSetLevelSemanticFrame {
    return {
      Level: isSet(object.level)
        ? TSoundLevelSlot.fromJSON(object.level)
        : undefined,
    };
  },

  toJSON(message: TSoundSetLevelSemanticFrame): unknown {
    const obj: any = {};
    message.Level !== undefined &&
      (obj.level = message.Level
        ? TSoundLevelSlot.toJSON(message.Level)
        : undefined);
    return obj;
  },
};

function createBaseTGetTimeSemanticFrame(): TGetTimeSemanticFrame {
  return { Where: undefined };
}

export const TGetTimeSemanticFrame = {
  encode(
    message: TGetTimeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Where !== undefined) {
      TWhereSlot.encode(message.Where, writer.uint32(42).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TGetTimeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetTimeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 5:
          message.Where = TWhereSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetTimeSemanticFrame {
    return {
      Where: isSet(object.where)
        ? TWhereSlot.fromJSON(object.where)
        : undefined,
    };
  },

  toJSON(message: TGetTimeSemanticFrame): unknown {
    const obj: any = {};
    message.Where !== undefined &&
      (obj.where = message.Where
        ? TWhereSlot.toJSON(message.Where)
        : undefined);
    return obj;
  },
};

function createBaseTHowToSubscribeSemanticFrame(): THowToSubscribeSemanticFrame {
  return {};
}

export const THowToSubscribeSemanticFrame = {
  encode(
    _: THowToSubscribeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): THowToSubscribeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTHowToSubscribeSemanticFrame();
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

  fromJSON(_: any): THowToSubscribeSemanticFrame {
    return {};
  },

  toJSON(_: THowToSubscribeSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTGetSmartTvCategoriesSemanticFrame(): TGetSmartTvCategoriesSemanticFrame {
  return {};
}

export const TGetSmartTvCategoriesSemanticFrame = {
  encode(
    _: TGetSmartTvCategoriesSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetSmartTvCategoriesSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetSmartTvCategoriesSemanticFrame();
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

  fromJSON(_: any): TGetSmartTvCategoriesSemanticFrame {
    return {};
  },

  toJSON(_: TGetSmartTvCategoriesSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTGetSmartTvCarouselSemanticFrame(): TGetSmartTvCarouselSemanticFrame {
  return {
    CarouselId: undefined,
    DocCacheHash: undefined,
    CarouselType: undefined,
    Filter: undefined,
    Tag: undefined,
    AvailableOnly: undefined,
    MoreUrlLimit: undefined,
    Limit: undefined,
    Offset: undefined,
    KidMode: undefined,
    RestrictionAge: undefined,
  };
}

export const TGetSmartTvCarouselSemanticFrame = {
  encode(
    message: TGetSmartTvCarouselSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CarouselId !== undefined) {
      TStringSlot.encode(message.CarouselId, writer.uint32(10).fork()).ldelim();
    }
    if (message.DocCacheHash !== undefined) {
      TStringSlot.encode(
        message.DocCacheHash,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.CarouselType !== undefined) {
      TStringSlot.encode(
        message.CarouselType,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.Filter !== undefined) {
      TStringSlot.encode(message.Filter, writer.uint32(34).fork()).ldelim();
    }
    if (message.Tag !== undefined) {
      TStringSlot.encode(message.Tag, writer.uint32(42).fork()).ldelim();
    }
    if (message.AvailableOnly !== undefined) {
      TBoolSlot.encode(
        message.AvailableOnly,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.MoreUrlLimit !== undefined) {
      TNumSlot.encode(message.MoreUrlLimit, writer.uint32(58).fork()).ldelim();
    }
    if (message.Limit !== undefined) {
      TNumSlot.encode(message.Limit, writer.uint32(66).fork()).ldelim();
    }
    if (message.Offset !== undefined) {
      TNumSlot.encode(message.Offset, writer.uint32(74).fork()).ldelim();
    }
    if (message.KidMode !== undefined) {
      TBoolSlot.encode(message.KidMode, writer.uint32(82).fork()).ldelim();
    }
    if (message.RestrictionAge !== undefined) {
      TNumSlot.encode(
        message.RestrictionAge,
        writer.uint32(90).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetSmartTvCarouselSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetSmartTvCarouselSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CarouselId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.DocCacheHash = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.CarouselType = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Filter = TStringSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.Tag = TStringSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.AvailableOnly = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 7:
          message.MoreUrlLimit = TNumSlot.decode(reader, reader.uint32());
          break;
        case 8:
          message.Limit = TNumSlot.decode(reader, reader.uint32());
          break;
        case 9:
          message.Offset = TNumSlot.decode(reader, reader.uint32());
          break;
        case 10:
          message.KidMode = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 11:
          message.RestrictionAge = TNumSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetSmartTvCarouselSemanticFrame {
    return {
      CarouselId: isSet(object.carousel_id)
        ? TStringSlot.fromJSON(object.carousel_id)
        : undefined,
      DocCacheHash: isSet(object.docs_cache_hash)
        ? TStringSlot.fromJSON(object.docs_cache_hash)
        : undefined,
      CarouselType: isSet(object.carousel_type)
        ? TStringSlot.fromJSON(object.carousel_type)
        : undefined,
      Filter: isSet(object.filter)
        ? TStringSlot.fromJSON(object.filter)
        : undefined,
      Tag: isSet(object.tag) ? TStringSlot.fromJSON(object.tag) : undefined,
      AvailableOnly: isSet(object.available_only)
        ? TBoolSlot.fromJSON(object.available_only)
        : undefined,
      MoreUrlLimit: isSet(object.more_url_limit)
        ? TNumSlot.fromJSON(object.more_url_limit)
        : undefined,
      Limit: isSet(object.limit) ? TNumSlot.fromJSON(object.limit) : undefined,
      Offset: isSet(object.offset)
        ? TNumSlot.fromJSON(object.offset)
        : undefined,
      KidMode: isSet(object.kid_mode)
        ? TBoolSlot.fromJSON(object.kid_mode)
        : undefined,
      RestrictionAge: isSet(object.restriction_age)
        ? TNumSlot.fromJSON(object.restriction_age)
        : undefined,
    };
  },

  toJSON(message: TGetSmartTvCarouselSemanticFrame): unknown {
    const obj: any = {};
    message.CarouselId !== undefined &&
      (obj.carousel_id = message.CarouselId
        ? TStringSlot.toJSON(message.CarouselId)
        : undefined);
    message.DocCacheHash !== undefined &&
      (obj.docs_cache_hash = message.DocCacheHash
        ? TStringSlot.toJSON(message.DocCacheHash)
        : undefined);
    message.CarouselType !== undefined &&
      (obj.carousel_type = message.CarouselType
        ? TStringSlot.toJSON(message.CarouselType)
        : undefined);
    message.Filter !== undefined &&
      (obj.filter = message.Filter
        ? TStringSlot.toJSON(message.Filter)
        : undefined);
    message.Tag !== undefined &&
      (obj.tag = message.Tag ? TStringSlot.toJSON(message.Tag) : undefined);
    message.AvailableOnly !== undefined &&
      (obj.available_only = message.AvailableOnly
        ? TBoolSlot.toJSON(message.AvailableOnly)
        : undefined);
    message.MoreUrlLimit !== undefined &&
      (obj.more_url_limit = message.MoreUrlLimit
        ? TNumSlot.toJSON(message.MoreUrlLimit)
        : undefined);
    message.Limit !== undefined &&
      (obj.limit = message.Limit ? TNumSlot.toJSON(message.Limit) : undefined);
    message.Offset !== undefined &&
      (obj.offset = message.Offset
        ? TNumSlot.toJSON(message.Offset)
        : undefined);
    message.KidMode !== undefined &&
      (obj.kid_mode = message.KidMode
        ? TBoolSlot.toJSON(message.KidMode)
        : undefined);
    message.RestrictionAge !== undefined &&
      (obj.restriction_age = message.RestrictionAge
        ? TNumSlot.toJSON(message.RestrictionAge)
        : undefined);
    return obj;
  },
};

function createBaseTGetSmartTvCarouselsSemanticFrame(): TGetSmartTvCarouselsSemanticFrame {
  return {
    CategoryId: undefined,
    MaxItemsCount: undefined,
    CacheHash: undefined,
    PurchasesAvailableOnly: undefined,
    Limit: undefined,
    Offset: undefined,
    KidMode: undefined,
    RestrictionAge: undefined,
    ExternalCarouselOffset: undefined,
  };
}

export const TGetSmartTvCarouselsSemanticFrame = {
  encode(
    message: TGetSmartTvCarouselsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CategoryId !== undefined) {
      TStringSlot.encode(message.CategoryId, writer.uint32(10).fork()).ldelim();
    }
    if (message.MaxItemsCount !== undefined) {
      TNumSlot.encode(message.MaxItemsCount, writer.uint32(18).fork()).ldelim();
    }
    if (message.CacheHash !== undefined) {
      TStringSlot.encode(message.CacheHash, writer.uint32(26).fork()).ldelim();
    }
    if (message.PurchasesAvailableOnly !== undefined) {
      TStringSlot.encode(
        message.PurchasesAvailableOnly,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.Limit !== undefined) {
      TNumSlot.encode(message.Limit, writer.uint32(42).fork()).ldelim();
    }
    if (message.Offset !== undefined) {
      TNumSlot.encode(message.Offset, writer.uint32(50).fork()).ldelim();
    }
    if (message.KidMode !== undefined) {
      TBoolSlot.encode(message.KidMode, writer.uint32(58).fork()).ldelim();
    }
    if (message.RestrictionAge !== undefined) {
      TNumSlot.encode(
        message.RestrictionAge,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.ExternalCarouselOffset !== undefined) {
      TNumSlot.encode(
        message.ExternalCarouselOffset,
        writer.uint32(74).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetSmartTvCarouselsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetSmartTvCarouselsSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CategoryId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.MaxItemsCount = TNumSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.CacheHash = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.PurchasesAvailableOnly = TStringSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 5:
          message.Limit = TNumSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.Offset = TNumSlot.decode(reader, reader.uint32());
          break;
        case 7:
          message.KidMode = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 8:
          message.RestrictionAge = TNumSlot.decode(reader, reader.uint32());
          break;
        case 9:
          message.ExternalCarouselOffset = TNumSlot.decode(
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

  fromJSON(object: any): TGetSmartTvCarouselsSemanticFrame {
    return {
      CategoryId: isSet(object.category_id)
        ? TStringSlot.fromJSON(object.category_id)
        : undefined,
      MaxItemsCount: isSet(object.max_items_count)
        ? TNumSlot.fromJSON(object.max_items_count)
        : undefined,
      CacheHash: isSet(object.cache_hash)
        ? TStringSlot.fromJSON(object.cache_hash)
        : undefined,
      PurchasesAvailableOnly: isSet(object.purchases_available_only)
        ? TStringSlot.fromJSON(object.purchases_available_only)
        : undefined,
      Limit: isSet(object.limit) ? TNumSlot.fromJSON(object.limit) : undefined,
      Offset: isSet(object.offset)
        ? TNumSlot.fromJSON(object.offset)
        : undefined,
      KidMode: isSet(object.kid_mode)
        ? TBoolSlot.fromJSON(object.kid_mode)
        : undefined,
      RestrictionAge: isSet(object.restriction_age)
        ? TNumSlot.fromJSON(object.restriction_age)
        : undefined,
      ExternalCarouselOffset: isSet(object.external_carousel_offset)
        ? TNumSlot.fromJSON(object.external_carousel_offset)
        : undefined,
    };
  },

  toJSON(message: TGetSmartTvCarouselsSemanticFrame): unknown {
    const obj: any = {};
    message.CategoryId !== undefined &&
      (obj.category_id = message.CategoryId
        ? TStringSlot.toJSON(message.CategoryId)
        : undefined);
    message.MaxItemsCount !== undefined &&
      (obj.max_items_count = message.MaxItemsCount
        ? TNumSlot.toJSON(message.MaxItemsCount)
        : undefined);
    message.CacheHash !== undefined &&
      (obj.cache_hash = message.CacheHash
        ? TStringSlot.toJSON(message.CacheHash)
        : undefined);
    message.PurchasesAvailableOnly !== undefined &&
      (obj.purchases_available_only = message.PurchasesAvailableOnly
        ? TStringSlot.toJSON(message.PurchasesAvailableOnly)
        : undefined);
    message.Limit !== undefined &&
      (obj.limit = message.Limit ? TNumSlot.toJSON(message.Limit) : undefined);
    message.Offset !== undefined &&
      (obj.offset = message.Offset
        ? TNumSlot.toJSON(message.Offset)
        : undefined);
    message.KidMode !== undefined &&
      (obj.kid_mode = message.KidMode
        ? TBoolSlot.toJSON(message.KidMode)
        : undefined);
    message.RestrictionAge !== undefined &&
      (obj.restriction_age = message.RestrictionAge
        ? TNumSlot.toJSON(message.RestrictionAge)
        : undefined);
    message.ExternalCarouselOffset !== undefined &&
      (obj.external_carousel_offset = message.ExternalCarouselOffset
        ? TNumSlot.toJSON(message.ExternalCarouselOffset)
        : undefined);
    return obj;
  },
};

function createBaseTIoTScenariosPhraseActionSemanticFrame(): TIoTScenariosPhraseActionSemanticFrame {
  return { Phrase: undefined };
}

export const TIoTScenariosPhraseActionSemanticFrame = {
  encode(
    message: TIoTScenariosPhraseActionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Phrase !== undefined) {
      TStringSlot.encode(message.Phrase, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTScenariosPhraseActionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTScenariosPhraseActionSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Phrase = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTScenariosPhraseActionSemanticFrame {
    return {
      Phrase: isSet(object.phrase)
        ? TStringSlot.fromJSON(object.phrase)
        : undefined,
    };
  },

  toJSON(message: TIoTScenariosPhraseActionSemanticFrame): unknown {
    const obj: any = {};
    message.Phrase !== undefined &&
      (obj.phrase = message.Phrase
        ? TStringSlot.toJSON(message.Phrase)
        : undefined);
    return obj;
  },
};

function createBaseTIoTScenariosTextActionSemanticFrame(): TIoTScenariosTextActionSemanticFrame {
  return { Text: undefined };
}

export const TIoTScenariosTextActionSemanticFrame = {
  encode(
    message: TIoTScenariosTextActionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== undefined) {
      TStringSlot.encode(message.Text, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTScenariosTextActionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTScenariosTextActionSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTScenariosTextActionSemanticFrame {
    return {
      Text: isSet(object.text) ? TStringSlot.fromJSON(object.text) : undefined,
    };
  },

  toJSON(message: TIoTScenariosTextActionSemanticFrame): unknown {
    const obj: any = {};
    message.Text !== undefined &&
      (obj.text = message.Text ? TStringSlot.toJSON(message.Text) : undefined);
    return obj;
  },
};

function createBaseTIoTScenariosLaunchActionSemanticFrame(): TIoTScenariosLaunchActionSemanticFrame {
  return {
    LaunchID: undefined,
    StepIndex: undefined,
    Instance: undefined,
    Value: undefined,
  };
}

export const TIoTScenariosLaunchActionSemanticFrame = {
  encode(
    message: TIoTScenariosLaunchActionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.LaunchID !== undefined) {
      TStringSlot.encode(message.LaunchID, writer.uint32(10).fork()).ldelim();
    }
    if (message.StepIndex !== undefined) {
      TUInt32Slot.encode(message.StepIndex, writer.uint32(18).fork()).ldelim();
    }
    if (message.Instance !== undefined) {
      TStringSlot.encode(message.Instance, writer.uint32(26).fork()).ldelim();
    }
    if (message.Value !== undefined) {
      TStringSlot.encode(message.Value, writer.uint32(34).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTScenariosLaunchActionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTScenariosLaunchActionSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LaunchID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.StepIndex = TUInt32Slot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Instance = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Value = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TIoTScenariosLaunchActionSemanticFrame {
    return {
      LaunchID: isSet(object.launch_id)
        ? TStringSlot.fromJSON(object.launch_id)
        : undefined,
      StepIndex: isSet(object.step_index)
        ? TUInt32Slot.fromJSON(object.step_index)
        : undefined,
      Instance: isSet(object.instance)
        ? TStringSlot.fromJSON(object.instance)
        : undefined,
      Value: isSet(object.value)
        ? TStringSlot.fromJSON(object.value)
        : undefined,
    };
  },

  toJSON(message: TIoTScenariosLaunchActionSemanticFrame): unknown {
    const obj: any = {};
    message.LaunchID !== undefined &&
      (obj.launch_id = message.LaunchID
        ? TStringSlot.toJSON(message.LaunchID)
        : undefined);
    message.StepIndex !== undefined &&
      (obj.step_index = message.StepIndex
        ? TUInt32Slot.toJSON(message.StepIndex)
        : undefined);
    message.Instance !== undefined &&
      (obj.instance = message.Instance
        ? TStringSlot.toJSON(message.Instance)
        : undefined);
    message.Value !== undefined &&
      (obj.value = message.Value
        ? TStringSlot.toJSON(message.Value)
        : undefined);
    return obj;
  },
};

function createBaseTIoTCapabilityActionSlot(): TIoTCapabilityActionSlot {
  return { CapabilityActionValue: undefined };
}

export const TIoTCapabilityActionSlot = {
  encode(
    message: TIoTCapabilityActionSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CapabilityActionValue !== undefined) {
      TIoTCapabilityAction.encode(
        message.CapabilityActionValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTCapabilityActionSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTCapabilityActionSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CapabilityActionValue = TIoTCapabilityAction.decode(
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

  fromJSON(object: any): TIoTCapabilityActionSlot {
    return {
      CapabilityActionValue: isSet(object.capability_action_value)
        ? TIoTCapabilityAction.fromJSON(object.capability_action_value)
        : undefined,
    };
  },

  toJSON(message: TIoTCapabilityActionSlot): unknown {
    const obj: any = {};
    message.CapabilityActionValue !== undefined &&
      (obj.capability_action_value = message.CapabilityActionValue
        ? TIoTCapabilityAction.toJSON(message.CapabilityActionValue)
        : undefined);
    return obj;
  },
};

function createBaseTIoTScenarioSpeakerActionSemanticFrame(): TIoTScenarioSpeakerActionSemanticFrame {
  return {
    LaunchID: undefined,
    StepIndex: undefined,
    CapabilityAction: undefined,
  };
}

export const TIoTScenarioSpeakerActionSemanticFrame = {
  encode(
    message: TIoTScenarioSpeakerActionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.LaunchID !== undefined) {
      TStringSlot.encode(message.LaunchID, writer.uint32(10).fork()).ldelim();
    }
    if (message.StepIndex !== undefined) {
      TUInt32Slot.encode(message.StepIndex, writer.uint32(18).fork()).ldelim();
    }
    if (message.CapabilityAction !== undefined) {
      TIoTCapabilityActionSlot.encode(
        message.CapabilityAction,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTScenarioSpeakerActionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTScenarioSpeakerActionSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.LaunchID = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.StepIndex = TUInt32Slot.decode(reader, reader.uint32());
          break;
        case 3:
          message.CapabilityAction = TIoTCapabilityActionSlot.decode(
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

  fromJSON(object: any): TIoTScenarioSpeakerActionSemanticFrame {
    return {
      LaunchID: isSet(object.launch_id)
        ? TStringSlot.fromJSON(object.launch_id)
        : undefined,
      StepIndex: isSet(object.step_index)
        ? TUInt32Slot.fromJSON(object.step_index)
        : undefined,
      CapabilityAction: isSet(object.capability_action)
        ? TIoTCapabilityActionSlot.fromJSON(object.capability_action)
        : undefined,
    };
  },

  toJSON(message: TIoTScenarioSpeakerActionSemanticFrame): unknown {
    const obj: any = {};
    message.LaunchID !== undefined &&
      (obj.launch_id = message.LaunchID
        ? TStringSlot.toJSON(message.LaunchID)
        : undefined);
    message.StepIndex !== undefined &&
      (obj.step_index = message.StepIndex
        ? TUInt32Slot.toJSON(message.StepIndex)
        : undefined);
    message.CapabilityAction !== undefined &&
      (obj.capability_action = message.CapabilityAction
        ? TIoTCapabilityActionSlot.toJSON(message.CapabilityAction)
        : undefined);
    return obj;
  },
};

function createBaseTAlarmSetAliceShowSemanticFrame(): TAlarmSetAliceShowSemanticFrame {
  return {};
}

export const TAlarmSetAliceShowSemanticFrame = {
  encode(
    _: TAlarmSetAliceShowSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TAlarmSetAliceShowSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTAlarmSetAliceShowSemanticFrame();
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

  fromJSON(_: any): TAlarmSetAliceShowSemanticFrame {
    return {};
  },

  toJSON(_: TAlarmSetAliceShowSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTRepeatAfterMeSemanticFrame(): TRepeatAfterMeSemanticFrame {
  return { Text: undefined, Voice: undefined };
}

export const TRepeatAfterMeSemanticFrame = {
  encode(
    message: TRepeatAfterMeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Text !== undefined) {
      TStringSlot.encode(message.Text, writer.uint32(10).fork()).ldelim();
    }
    if (message.Voice !== undefined) {
      TStringSlot.encode(message.Voice, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TRepeatAfterMeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTRepeatAfterMeSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Text = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Voice = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TRepeatAfterMeSemanticFrame {
    return {
      Text: isSet(object.text) ? TStringSlot.fromJSON(object.text) : undefined,
      Voice: isSet(object.voice)
        ? TStringSlot.fromJSON(object.voice)
        : undefined,
    };
  },

  toJSON(message: TRepeatAfterMeSemanticFrame): unknown {
    const obj: any = {};
    message.Text !== undefined &&
      (obj.text = message.Text ? TStringSlot.toJSON(message.Text) : undefined);
    message.Voice !== undefined &&
      (obj.voice = message.Voice
        ? TStringSlot.toJSON(message.Voice)
        : undefined);
    return obj;
  },
};

function createBaseTMediaPlaySemanticFrame(): TMediaPlaySemanticFrame {
  return { TuneId: undefined, LocationId: undefined };
}

export const TMediaPlaySemanticFrame = {
  encode(
    message: TMediaPlaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TuneId !== undefined) {
      TStringSlot.encode(message.TuneId, writer.uint32(10).fork()).ldelim();
    }
    if (message.LocationId !== undefined) {
      TStringSlot.encode(message.LocationId, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TMediaPlaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMediaPlaySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TuneId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.LocationId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMediaPlaySemanticFrame {
    return {
      TuneId: isSet(object.tune_id)
        ? TStringSlot.fromJSON(object.tune_id)
        : undefined,
      LocationId: isSet(object.location_id)
        ? TStringSlot.fromJSON(object.location_id)
        : undefined,
    };
  },

  toJSON(message: TMediaPlaySemanticFrame): unknown {
    const obj: any = {};
    message.TuneId !== undefined &&
      (obj.tune_id = message.TuneId
        ? TStringSlot.toJSON(message.TuneId)
        : undefined);
    message.LocationId !== undefined &&
      (obj.location_id = message.LocationId
        ? TStringSlot.toJSON(message.LocationId)
        : undefined);
    return obj;
  },
};

function createBaseTZenContextSearchStartSemanticFrame(): TZenContextSearchStartSemanticFrame {
  return {};
}

export const TZenContextSearchStartSemanticFrame = {
  encode(
    _: TZenContextSearchStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TZenContextSearchStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTZenContextSearchStartSemanticFrame();
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

  fromJSON(_: any): TZenContextSearchStartSemanticFrame {
    return {};
  },

  toJSON(_: TZenContextSearchStartSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTurnClockFaceOnSemanticFrame(): TTurnClockFaceOnSemanticFrame {
  return {};
}

export const TTurnClockFaceOnSemanticFrame = {
  encode(
    _: TTurnClockFaceOnSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTurnClockFaceOnSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTurnClockFaceOnSemanticFrame();
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

  fromJSON(_: any): TTurnClockFaceOnSemanticFrame {
    return {};
  },

  toJSON(_: TTurnClockFaceOnSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTurnClockFaceOffSemanticFrame(): TTurnClockFaceOffSemanticFrame {
  return {};
}

export const TTurnClockFaceOffSemanticFrame = {
  encode(
    _: TTurnClockFaceOffSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTurnClockFaceOffSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTurnClockFaceOffSemanticFrame();
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

  fromJSON(_: any): TTurnClockFaceOffSemanticFrame {
    return {};
  },

  toJSON(_: TTurnClockFaceOffSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTIoTDeviceActionRequestSlot(): TIoTDeviceActionRequestSlot {
  return { RequestValue: undefined };
}

export const TIoTDeviceActionRequestSlot = {
  encode(
    message: TIoTDeviceActionRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TIoTDeviceActionRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTDeviceActionRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDeviceActionRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TIoTDeviceActionRequest.decode(
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

  fromJSON(object: any): TIoTDeviceActionRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TIoTDeviceActionRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TIoTDeviceActionRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TIoTDeviceActionRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTIoTDeviceActionSemanticFrame(): TIoTDeviceActionSemanticFrame {
  return { Request: undefined };
}

export const TIoTDeviceActionSemanticFrame = {
  encode(
    message: TIoTDeviceActionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Request !== undefined) {
      TIoTDeviceActionRequestSlot.encode(
        message.Request,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TIoTDeviceActionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTIoTDeviceActionSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Request = TIoTDeviceActionRequestSlot.decode(
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

  fromJSON(object: any): TIoTDeviceActionSemanticFrame {
    return {
      Request: isSet(object.request)
        ? TIoTDeviceActionRequestSlot.fromJSON(object.request)
        : undefined,
    };
  },

  toJSON(message: TIoTDeviceActionSemanticFrame): unknown {
    const obj: any = {};
    message.Request !== undefined &&
      (obj.request = message.Request
        ? TIoTDeviceActionRequestSlot.toJSON(message.Request)
        : undefined);
    return obj;
  },
};

function createBaseTWhisperSaySomethingSemanticFrame(): TWhisperSaySomethingSemanticFrame {
  return {};
}

export const TWhisperSaySomethingSemanticFrame = {
  encode(
    _: TWhisperSaySomethingSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWhisperSaySomethingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWhisperSaySomethingSemanticFrame();
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

  fromJSON(_: any): TWhisperSaySomethingSemanticFrame {
    return {};
  },

  toJSON(_: TWhisperSaySomethingSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTWhisperTurnOffSemanticFrame(): TWhisperTurnOffSemanticFrame {
  return {};
}

export const TWhisperTurnOffSemanticFrame = {
  encode(
    _: TWhisperTurnOffSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWhisperTurnOffSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWhisperTurnOffSemanticFrame();
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

  fromJSON(_: any): TWhisperTurnOffSemanticFrame {
    return {};
  },

  toJSON(_: TWhisperTurnOffSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTWhisperTurnOnSemanticFrame(): TWhisperTurnOnSemanticFrame {
  return {};
}

export const TWhisperTurnOnSemanticFrame = {
  encode(
    _: TWhisperTurnOnSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWhisperTurnOnSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWhisperTurnOnSemanticFrame();
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

  fromJSON(_: any): TWhisperTurnOnSemanticFrame {
    return {};
  },

  toJSON(_: TWhisperTurnOnSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTWhisperWhatIsItSemanticFrame(): TWhisperWhatIsItSemanticFrame {
  return {};
}

export const TWhisperWhatIsItSemanticFrame = {
  encode(
    _: TWhisperWhatIsItSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TWhisperWhatIsItSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTWhisperWhatIsItSemanticFrame();
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

  fromJSON(_: any): TWhisperWhatIsItSemanticFrame {
    return {};
  },

  toJSON(_: TWhisperWhatIsItSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTimeCapsuleNextStepSemanticFrame(): TTimeCapsuleNextStepSemanticFrame {
  return {};
}

export const TTimeCapsuleNextStepSemanticFrame = {
  encode(
    _: TTimeCapsuleNextStepSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTimeCapsuleNextStepSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTimeCapsuleNextStepSemanticFrame();
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

  fromJSON(_: any): TTimeCapsuleNextStepSemanticFrame {
    return {};
  },

  toJSON(_: TTimeCapsuleNextStepSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTimeCapsuleStopSemanticFrame(): TTimeCapsuleStopSemanticFrame {
  return {};
}

export const TTimeCapsuleStopSemanticFrame = {
  encode(
    _: TTimeCapsuleStopSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTimeCapsuleStopSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTimeCapsuleStopSemanticFrame();
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

  fromJSON(_: any): TTimeCapsuleStopSemanticFrame {
    return {};
  },

  toJSON(_: TTimeCapsuleStopSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTimeCapsuleStartSemanticFrame(): TTimeCapsuleStartSemanticFrame {
  return {};
}

export const TTimeCapsuleStartSemanticFrame = {
  encode(
    _: TTimeCapsuleStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTimeCapsuleStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTimeCapsuleStartSemanticFrame();
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

  fromJSON(_: any): TTimeCapsuleStartSemanticFrame {
    return {};
  },

  toJSON(_: TTimeCapsuleStartSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTimeCapsuleResumeSemanticFrame(): TTimeCapsuleResumeSemanticFrame {
  return {};
}

export const TTimeCapsuleResumeSemanticFrame = {
  encode(
    _: TTimeCapsuleResumeSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTimeCapsuleResumeSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTimeCapsuleResumeSemanticFrame();
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

  fromJSON(_: any): TTimeCapsuleResumeSemanticFrame {
    return {};
  },

  toJSON(_: TTimeCapsuleResumeSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTimeCapsuleSkipQuestionSemanticFrame(): TTimeCapsuleSkipQuestionSemanticFrame {
  return {};
}

export const TTimeCapsuleSkipQuestionSemanticFrame = {
  encode(
    _: TTimeCapsuleSkipQuestionSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTimeCapsuleSkipQuestionSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTimeCapsuleSkipQuestionSemanticFrame();
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

  fromJSON(_: any): TTimeCapsuleSkipQuestionSemanticFrame {
    return {};
  },

  toJSON(_: TTimeCapsuleSkipQuestionSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTActivateGenerativeTaleSemanticFrame(): TActivateGenerativeTaleSemanticFrame {
  return { Character: undefined };
}

export const TActivateGenerativeTaleSemanticFrame = {
  encode(
    message: TActivateGenerativeTaleSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Character !== undefined) {
      TStringSlot.encode(message.Character, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TActivateGenerativeTaleSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTActivateGenerativeTaleSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Character = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TActivateGenerativeTaleSemanticFrame {
    return {
      Character: isSet(object.generative_tale_character)
        ? TStringSlot.fromJSON(object.generative_tale_character)
        : undefined,
    };
  },

  toJSON(message: TActivateGenerativeTaleSemanticFrame): unknown {
    const obj: any = {};
    message.Character !== undefined &&
      (obj.generative_tale_character = message.Character
        ? TStringSlot.toJSON(message.Character)
        : undefined);
    return obj;
  },
};

function createBaseTHardcodedResponseSemanticFrame(): THardcodedResponseSemanticFrame {
  return { HardcodedResponseName: undefined };
}

export const THardcodedResponseSemanticFrame = {
  encode(
    message: THardcodedResponseSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.HardcodedResponseName !== undefined) {
      THardcodedResponseName.encode(
        message.HardcodedResponseName,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): THardcodedResponseSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTHardcodedResponseSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.HardcodedResponseName = THardcodedResponseName.decode(
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

  fromJSON(object: any): THardcodedResponseSemanticFrame {
    return {
      HardcodedResponseName: isSet(object.hardcoded_response_name)
        ? THardcodedResponseName.fromJSON(object.hardcoded_response_name)
        : undefined,
    };
  },

  toJSON(message: THardcodedResponseSemanticFrame): unknown {
    const obj: any = {};
    message.HardcodedResponseName !== undefined &&
      (obj.hardcoded_response_name = message.HardcodedResponseName
        ? THardcodedResponseName.toJSON(message.HardcodedResponseName)
        : undefined);
    return obj;
  },
};

function createBaseTSkillSessionRequestSemanticFrame(): TSkillSessionRequestSemanticFrame {
  return {};
}

export const TSkillSessionRequestSemanticFrame = {
  encode(
    _: TSkillSessionRequestSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSkillSessionRequestSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSkillSessionRequestSemanticFrame();
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

  fromJSON(_: any): TSkillSessionRequestSemanticFrame {
    return {};
  },

  toJSON(_: TSkillSessionRequestSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTUpdateContactsRequestSlot(): TUpdateContactsRequestSlot {
  return { RequestValue: undefined };
}

export const TUpdateContactsRequestSlot = {
  encode(
    message: TUpdateContactsRequestSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.RequestValue !== undefined) {
      TUpdateContactsRequest.encode(
        message.RequestValue,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TUpdateContactsRequestSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUpdateContactsRequestSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.RequestValue = TUpdateContactsRequest.decode(
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

  fromJSON(object: any): TUpdateContactsRequestSlot {
    return {
      RequestValue: isSet(object.request_value)
        ? TUpdateContactsRequest.fromJSON(object.request_value)
        : undefined,
    };
  },

  toJSON(message: TUpdateContactsRequestSlot): unknown {
    const obj: any = {};
    message.RequestValue !== undefined &&
      (obj.request_value = message.RequestValue
        ? TUpdateContactsRequest.toJSON(message.RequestValue)
        : undefined);
    return obj;
  },
};

function createBaseTUploadContactsRequestSemanticFrame(): TUploadContactsRequestSemanticFrame {
  return { UploadRequest: undefined };
}

export const TUploadContactsRequestSemanticFrame = {
  encode(
    message: TUploadContactsRequestSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UploadRequest !== undefined) {
      TUpdateContactsRequestSlot.encode(
        message.UploadRequest,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TUploadContactsRequestSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUploadContactsRequestSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          message.UploadRequest = TUpdateContactsRequestSlot.decode(
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

  fromJSON(object: any): TUploadContactsRequestSemanticFrame {
    return {
      UploadRequest: isSet(object.upload_request)
        ? TUpdateContactsRequestSlot.fromJSON(object.upload_request)
        : undefined,
    };
  },

  toJSON(message: TUploadContactsRequestSemanticFrame): unknown {
    const obj: any = {};
    message.UploadRequest !== undefined &&
      (obj.upload_request = message.UploadRequest
        ? TUpdateContactsRequestSlot.toJSON(message.UploadRequest)
        : undefined);
    return obj;
  },
};

function createBaseTUpdateContactsRequestSemanticFrame(): TUpdateContactsRequestSemanticFrame {
  return { UpdateRequest: undefined };
}

export const TUpdateContactsRequestSemanticFrame = {
  encode(
    message: TUpdateContactsRequestSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UpdateRequest !== undefined) {
      TUpdateContactsRequestSlot.encode(
        message.UpdateRequest,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TUpdateContactsRequestSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTUpdateContactsRequestSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UpdateRequest = TUpdateContactsRequestSlot.decode(
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

  fromJSON(object: any): TUpdateContactsRequestSemanticFrame {
    return {
      UpdateRequest: isSet(object.update_request)
        ? TUpdateContactsRequestSlot.fromJSON(object.update_request)
        : undefined,
    };
  },

  toJSON(message: TUpdateContactsRequestSemanticFrame): unknown {
    const obj: any = {};
    message.UpdateRequest !== undefined &&
      (obj.update_request = message.UpdateRequest
        ? TUpdateContactsRequestSlot.toJSON(message.UpdateRequest)
        : undefined);
    return obj;
  },
};

function createBaseTExternalSkillForceDeactivateSemanticFrame(): TExternalSkillForceDeactivateSemanticFrame {
  return { DialogId: undefined, SilentResponse: undefined };
}

export const TExternalSkillForceDeactivateSemanticFrame = {
  encode(
    message: TExternalSkillForceDeactivateSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.DialogId !== undefined) {
      TStringSlot.encode(message.DialogId, writer.uint32(10).fork()).ldelim();
    }
    if (message.SilentResponse !== undefined) {
      TBoolSlot.encode(
        message.SilentResponse,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TExternalSkillForceDeactivateSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTExternalSkillForceDeactivateSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.DialogId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.SilentResponse = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TExternalSkillForceDeactivateSemanticFrame {
    return {
      DialogId: isSet(object.dialog_id)
        ? TStringSlot.fromJSON(object.dialog_id)
        : undefined,
      SilentResponse: isSet(object.silent_response)
        ? TBoolSlot.fromJSON(object.silent_response)
        : undefined,
    };
  },

  toJSON(message: TExternalSkillForceDeactivateSemanticFrame): unknown {
    const obj: any = {};
    message.DialogId !== undefined &&
      (obj.dialog_id = message.DialogId
        ? TStringSlot.toJSON(message.DialogId)
        : undefined);
    message.SilentResponse !== undefined &&
      (obj.silent_response = message.SilentResponse
        ? TBoolSlot.toJSON(message.SilentResponse)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallProviderSlot(): TVideoCallProviderSlot {
  return { EnumValue: undefined };
}

export const TVideoCallProviderSlot = {
  encode(
    message: TVideoCallProviderSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.EnumValue !== undefined) {
      writer.uint32(8).int32(message.EnumValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TVideoCallProviderSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallProviderSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.EnumValue = reader.int32() as any;
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallProviderSlot {
    return {
      EnumValue: isSet(object.enum_value)
        ? tVideoCallProviderSlot_EValueFromJSON(object.enum_value)
        : undefined,
    };
  },

  toJSON(message: TVideoCallProviderSlot): unknown {
    const obj: any = {};
    message.EnumValue !== undefined &&
      (obj.enum_value =
        message.EnumValue !== undefined
          ? tVideoCallProviderSlot_EValueToJSON(message.EnumValue)
          : undefined);
    return obj;
  },
};

function createBaseTVideoCallLoginFailedSemanticFrame(): TVideoCallLoginFailedSemanticFrame {
  return { Provider: undefined };
}

export const TVideoCallLoginFailedSemanticFrame = {
  encode(
    message: TVideoCallLoginFailedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Provider !== undefined) {
      TVideoCallProviderSlot.encode(
        message.Provider,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallLoginFailedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallLoginFailedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Provider = TVideoCallProviderSlot.decode(
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

  fromJSON(object: any): TVideoCallLoginFailedSemanticFrame {
    return {
      Provider: isSet(object.provider)
        ? TVideoCallProviderSlot.fromJSON(object.provider)
        : undefined,
    };
  },

  toJSON(message: TVideoCallLoginFailedSemanticFrame): unknown {
    const obj: any = {};
    message.Provider !== undefined &&
      (obj.provider = message.Provider
        ? TVideoCallProviderSlot.toJSON(message.Provider)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallOutgoingAcceptedSemanticFrame(): TVideoCallOutgoingAcceptedSemanticFrame {
  return {
    Provider: undefined,
    CallId: undefined,
    UserId: undefined,
    Contact: undefined,
  };
}

export const TVideoCallOutgoingAcceptedSemanticFrame = {
  encode(
    message: TVideoCallOutgoingAcceptedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Provider !== undefined) {
      TVideoCallProviderSlot.encode(
        message.Provider,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.CallId !== undefined) {
      TStringSlot.encode(message.CallId, writer.uint32(18).fork()).ldelim();
    }
    if (message.UserId !== undefined) {
      TStringSlot.encode(message.UserId, writer.uint32(26).fork()).ldelim();
    }
    if (message.Contact !== undefined) {
      TProviderContactSlot.encode(
        message.Contact,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallOutgoingAcceptedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallOutgoingAcceptedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Provider = TVideoCallProviderSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.CallId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.UserId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Contact = TProviderContactSlot.decode(
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

  fromJSON(object: any): TVideoCallOutgoingAcceptedSemanticFrame {
    return {
      Provider: isSet(object.provider)
        ? TVideoCallProviderSlot.fromJSON(object.provider)
        : undefined,
      CallId: isSet(object.call_id)
        ? TStringSlot.fromJSON(object.call_id)
        : undefined,
      UserId: isSet(object.user_id)
        ? TStringSlot.fromJSON(object.user_id)
        : undefined,
      Contact: isSet(object.contact)
        ? TProviderContactSlot.fromJSON(object.contact)
        : undefined,
    };
  },

  toJSON(message: TVideoCallOutgoingAcceptedSemanticFrame): unknown {
    const obj: any = {};
    message.Provider !== undefined &&
      (obj.provider = message.Provider
        ? TVideoCallProviderSlot.toJSON(message.Provider)
        : undefined);
    message.CallId !== undefined &&
      (obj.call_id = message.CallId
        ? TStringSlot.toJSON(message.CallId)
        : undefined);
    message.UserId !== undefined &&
      (obj.user_id = message.UserId
        ? TStringSlot.toJSON(message.UserId)
        : undefined);
    message.Contact !== undefined &&
      (obj.contact = message.Contact
        ? TProviderContactSlot.toJSON(message.Contact)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallOutgoingFailedSemanticFrame(): TVideoCallOutgoingFailedSemanticFrame {
  return { Provider: undefined };
}

export const TVideoCallOutgoingFailedSemanticFrame = {
  encode(
    message: TVideoCallOutgoingFailedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Provider !== undefined) {
      TVideoCallProviderSlot.encode(
        message.Provider,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallOutgoingFailedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallOutgoingFailedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Provider = TVideoCallProviderSlot.decode(
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

  fromJSON(object: any): TVideoCallOutgoingFailedSemanticFrame {
    return {
      Provider: isSet(object.provider)
        ? TVideoCallProviderSlot.fromJSON(object.provider)
        : undefined,
    };
  },

  toJSON(message: TVideoCallOutgoingFailedSemanticFrame): unknown {
    const obj: any = {};
    message.Provider !== undefined &&
      (obj.provider = message.Provider
        ? TVideoCallProviderSlot.toJSON(message.Provider)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallIncomingAcceptFailedSemanticFrame(): TVideoCallIncomingAcceptFailedSemanticFrame {
  return { Provider: undefined, CallId: undefined };
}

export const TVideoCallIncomingAcceptFailedSemanticFrame = {
  encode(
    message: TVideoCallIncomingAcceptFailedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Provider !== undefined) {
      TVideoCallProviderSlot.encode(
        message.Provider,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.CallId !== undefined) {
      TStringSlot.encode(message.CallId, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallIncomingAcceptFailedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallIncomingAcceptFailedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Provider = TVideoCallProviderSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.CallId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallIncomingAcceptFailedSemanticFrame {
    return {
      Provider: isSet(object.provider)
        ? TVideoCallProviderSlot.fromJSON(object.provider)
        : undefined,
      CallId: isSet(object.call_id)
        ? TStringSlot.fromJSON(object.call_id)
        : undefined,
    };
  },

  toJSON(message: TVideoCallIncomingAcceptFailedSemanticFrame): unknown {
    const obj: any = {};
    message.Provider !== undefined &&
      (obj.provider = message.Provider
        ? TVideoCallProviderSlot.toJSON(message.Provider)
        : undefined);
    message.CallId !== undefined &&
      (obj.call_id = message.CallId
        ? TStringSlot.toJSON(message.CallId)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallItemNameSlot(): TVideoCallItemNameSlot {
  return { ItemName: undefined };
}

export const TVideoCallItemNameSlot = {
  encode(
    message: TVideoCallItemNameSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ItemName !== undefined) {
      writer.uint32(10).string(message.ItemName);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TVideoCallItemNameSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallItemNameSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ItemName = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallItemNameSlot {
    return {
      ItemName: isSet(object.item_name_value)
        ? String(object.item_name_value)
        : undefined,
    };
  },

  toJSON(message: TVideoCallItemNameSlot): unknown {
    const obj: any = {};
    message.ItemName !== undefined && (obj.item_name_value = message.ItemName);
    return obj;
  },
};

function createBaseTPhoneCallSemanticFrame(): TPhoneCallSemanticFrame {
  return { ItemName: undefined };
}

export const TPhoneCallSemanticFrame = {
  encode(
    message: TPhoneCallSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ItemName !== undefined) {
      TVideoCallItemNameSlot.encode(
        message.ItemName,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TPhoneCallSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTPhoneCallSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ItemName = TVideoCallItemNameSlot.decode(
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

  fromJSON(object: any): TPhoneCallSemanticFrame {
    return {
      ItemName: isSet(object.item_name)
        ? TVideoCallItemNameSlot.fromJSON(object.item_name)
        : undefined,
    };
  },

  toJSON(message: TPhoneCallSemanticFrame): unknown {
    const obj: any = {};
    message.ItemName !== undefined &&
      (obj.item_name = message.ItemName
        ? TVideoCallItemNameSlot.toJSON(message.ItemName)
        : undefined);
    return obj;
  },
};

function createBaseTProviderContactSlot(): TProviderContactSlot {
  return { ContactData: undefined };
}

export const TProviderContactSlot = {
  encode(
    message: TProviderContactSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContactData !== undefined) {
      TProviderContactData.encode(
        message.ContactData,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TProviderContactSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTProviderContactSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContactData = TProviderContactData.decode(
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

  fromJSON(object: any): TProviderContactSlot {
    return {
      ContactData: isSet(object.contact_data)
        ? TProviderContactData.fromJSON(object.contact_data)
        : undefined,
    };
  },

  toJSON(message: TProviderContactSlot): unknown {
    const obj: any = {};
    message.ContactData !== undefined &&
      (obj.contact_data = message.ContactData
        ? TProviderContactData.toJSON(message.ContactData)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallToSemanticFrame(): TVideoCallToSemanticFrame {
  return { FixedContact: undefined, VideoEnabled: undefined };
}

export const TVideoCallToSemanticFrame = {
  encode(
    message: TVideoCallToSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.FixedContact !== undefined) {
      TProviderContactSlot.encode(
        message.FixedContact,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.VideoEnabled !== undefined) {
      TBoolSlot.encode(message.VideoEnabled, writer.uint32(18).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallToSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallToSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.FixedContact = TProviderContactSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.VideoEnabled = TBoolSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallToSemanticFrame {
    return {
      FixedContact: isSet(object.fixed_contact)
        ? TProviderContactSlot.fromJSON(object.fixed_contact)
        : undefined,
      VideoEnabled: isSet(object.video_enabled)
        ? TBoolSlot.fromJSON(object.video_enabled)
        : undefined,
    };
  },

  toJSON(message: TVideoCallToSemanticFrame): unknown {
    const obj: any = {};
    message.FixedContact !== undefined &&
      (obj.fixed_contact = message.FixedContact
        ? TProviderContactSlot.toJSON(message.FixedContact)
        : undefined);
    message.VideoEnabled !== undefined &&
      (obj.video_enabled = message.VideoEnabled
        ? TBoolSlot.toJSON(message.VideoEnabled)
        : undefined);
    return obj;
  },
};

function createBaseTOpenAddressBookSemanticFrame(): TOpenAddressBookSemanticFrame {
  return {};
}

export const TOpenAddressBookSemanticFrame = {
  encode(
    _: TOpenAddressBookSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOpenAddressBookSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOpenAddressBookSemanticFrame();
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

  fromJSON(_: any): TOpenAddressBookSemanticFrame {
    return {};
  },

  toJSON(_: TOpenAddressBookSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTGetEqualizerSettingsSemanticFrame(): TGetEqualizerSettingsSemanticFrame {
  return {};
}

export const TGetEqualizerSettingsSemanticFrame = {
  encode(
    _: TGetEqualizerSettingsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetEqualizerSettingsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetEqualizerSettingsSemanticFrame();
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

  fromJSON(_: any): TGetEqualizerSettingsSemanticFrame {
    return {};
  },

  toJSON(_: TGetEqualizerSettingsSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTProviderContactListSlot(): TProviderContactListSlot {
  return { ContactList: undefined };
}

export const TProviderContactListSlot = {
  encode(
    message: TProviderContactListSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContactList !== undefined) {
      TProviderContactList.encode(
        message.ContactList,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TProviderContactListSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTProviderContactListSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContactList = TProviderContactList.decode(
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

  fromJSON(object: any): TProviderContactListSlot {
    return {
      ContactList: isSet(object.contact_list)
        ? TProviderContactList.fromJSON(object.contact_list)
        : undefined,
    };
  },

  toJSON(message: TProviderContactListSlot): unknown {
    const obj: any = {};
    message.ContactList !== undefined &&
      (obj.contact_list = message.ContactList
        ? TProviderContactList.toJSON(message.ContactList)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallSetFavoritesSemanticFrame(): TVideoCallSetFavoritesSemanticFrame {
  return { UserId: undefined, Favorites: undefined };
}

export const TVideoCallSetFavoritesSemanticFrame = {
  encode(
    message: TVideoCallSetFavoritesSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.UserId !== undefined) {
      TStringSlot.encode(message.UserId, writer.uint32(10).fork()).ldelim();
    }
    if (message.Favorites !== undefined) {
      TProviderContactListSlot.encode(
        message.Favorites,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallSetFavoritesSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallSetFavoritesSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.UserId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Favorites = TProviderContactListSlot.decode(
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

  fromJSON(object: any): TVideoCallSetFavoritesSemanticFrame {
    return {
      UserId: isSet(object.user_id)
        ? TStringSlot.fromJSON(object.user_id)
        : undefined,
      Favorites: isSet(object.favorites)
        ? TProviderContactListSlot.fromJSON(object.favorites)
        : undefined,
    };
  },

  toJSON(message: TVideoCallSetFavoritesSemanticFrame): unknown {
    const obj: any = {};
    message.UserId !== undefined &&
      (obj.user_id = message.UserId
        ? TStringSlot.toJSON(message.UserId)
        : undefined);
    message.Favorites !== undefined &&
      (obj.favorites = message.Favorites
        ? TProviderContactListSlot.toJSON(message.Favorites)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCallIncomingSemanticFrame(): TVideoCallIncomingSemanticFrame {
  return {
    Provider: undefined,
    CallId: undefined,
    UserId: undefined,
    Caller: undefined,
  };
}

export const TVideoCallIncomingSemanticFrame = {
  encode(
    message: TVideoCallIncomingSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Provider !== undefined) {
      TVideoCallProviderSlot.encode(
        message.Provider,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.CallId !== undefined) {
      TStringSlot.encode(message.CallId, writer.uint32(18).fork()).ldelim();
    }
    if (message.UserId !== undefined) {
      TStringSlot.encode(message.UserId, writer.uint32(26).fork()).ldelim();
    }
    if (message.Caller !== undefined) {
      TProviderContactSlot.encode(
        message.Caller,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCallIncomingSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCallIncomingSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Provider = TVideoCallProviderSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.CallId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.UserId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Caller = TProviderContactSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCallIncomingSemanticFrame {
    return {
      Provider: isSet(object.provider)
        ? TVideoCallProviderSlot.fromJSON(object.provider)
        : undefined,
      CallId: isSet(object.call_id)
        ? TStringSlot.fromJSON(object.call_id)
        : undefined,
      UserId: isSet(object.user_id)
        ? TStringSlot.fromJSON(object.user_id)
        : undefined,
      Caller: isSet(object.caller)
        ? TProviderContactSlot.fromJSON(object.caller)
        : undefined,
    };
  },

  toJSON(message: TVideoCallIncomingSemanticFrame): unknown {
    const obj: any = {};
    message.Provider !== undefined &&
      (obj.provider = message.Provider
        ? TVideoCallProviderSlot.toJSON(message.Provider)
        : undefined);
    message.CallId !== undefined &&
      (obj.call_id = message.CallId
        ? TStringSlot.toJSON(message.CallId)
        : undefined);
    message.UserId !== undefined &&
      (obj.user_id = message.UserId
        ? TStringSlot.toJSON(message.UserId)
        : undefined);
    message.Caller !== undefined &&
      (obj.caller = message.Caller
        ? TProviderContactSlot.toJSON(message.Caller)
        : undefined);
    return obj;
  },
};

function createBaseTMessengerCallAcceptSemanticFrame(): TMessengerCallAcceptSemanticFrame {
  return {};
}

export const TMessengerCallAcceptSemanticFrame = {
  encode(
    _: TMessengerCallAcceptSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMessengerCallAcceptSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMessengerCallAcceptSemanticFrame();
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

  fromJSON(_: any): TMessengerCallAcceptSemanticFrame {
    return {};
  },

  toJSON(_: TMessengerCallAcceptSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMessengerCallDiscardSemanticFrame(): TMessengerCallDiscardSemanticFrame {
  return {};
}

export const TMessengerCallDiscardSemanticFrame = {
  encode(
    _: TMessengerCallDiscardSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMessengerCallDiscardSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMessengerCallDiscardSemanticFrame();
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

  fromJSON(_: any): TMessengerCallDiscardSemanticFrame {
    return {};
  },

  toJSON(_: TMessengerCallDiscardSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTMessengerCallHangupSemanticFrame(): TMessengerCallHangupSemanticFrame {
  return {};
}

export const TMessengerCallHangupSemanticFrame = {
  encode(
    _: TMessengerCallHangupSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMessengerCallHangupSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMessengerCallHangupSemanticFrame();
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

  fromJSON(_: any): TMessengerCallHangupSemanticFrame {
    return {};
  },

  toJSON(_: TMessengerCallHangupSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTypedSemanticFrame(): TTypedSemanticFrame {
  return {
    SearchSemanticFrame: undefined,
    IoTBroadcastStartSemanticFrame: undefined,
    IoTBroadcastSuccessSemanticFrame: undefined,
    IoTBroadcastFailureSemanticFrame: undefined,
    MordoviaHomeScreenSemanticFrame: undefined,
    NewsSemanticFrame: undefined,
    GetCallerNameSemanticFrame: undefined,
    MusicPlaySemanticFrame: undefined,
    ExternalSkillActivateSemanticFrame: undefined,
    VideoPlaySemanticFrame: undefined,
    WeatherSemanticFrame: undefined,
    HardcodedMorningShowSemanticFrame: undefined,
    SelectVideoFromGallerySemanticFrame: undefined,
    OpenCurrentVideoSemanticFrame: undefined,
    VideoPaymentConfirmedSemanticFrame: undefined,
    PlayerNextTrackSemanticFrame: undefined,
    PlayerPrevTrackSemanticFrame: undefined,
    PlayerLikeSemanticFrame: undefined,
    PlayerDislikeSemanticFrame: undefined,
    DoNothingSemanticFrame: undefined,
    NotificationsSubscribeSemanticFrame: undefined,
    VideoRaterSemanticFrame: undefined,
    SetupRcuStatusSemanticFrame: undefined,
    SetupRcuAutoStatusSemanticFrame: undefined,
    SetupRcuCheckStatusSemanticFrame: undefined,
    SetupRcuAdvancedStatusSemanticFrame: undefined,
    SetupRcuManualStartSemanticFrame: undefined,
    SetupRcuAutoStartSemanticFrame: undefined,
    LinkARemoteSemanticFrame: undefined,
    RequestTechnicalSupportSemanticFrame: undefined,
    IoTDiscoveryStartSemanticFrame: undefined,
    IoTDiscoverySuccessSemanticFrame: undefined,
    IoTDiscoveryFailureSemanticFrame: undefined,
    ExternalSkillFixedActivateSemanticFrame: undefined,
    OpenCurrentTrailerSemanticFrame: undefined,
    OnboardingStartingCriticalUpdateSemanticFrame: undefined,
    OnboardingStartingConfigureSuccessSemanticFrame: undefined,
    RadioPlaySemanticFrame: undefined,
    CentaurCollectCardsSemanticFrame: undefined,
    CentaurGetCardSemanticFrame: undefined,
    VideoPlayerFinishedSemanticFrame: undefined,
    SoundLouderSemanticFrame: undefined,
    SoundQuiterSemanticFrame: undefined,
    SoundSetLevelSemanticFrame: undefined,
    GetPhotoFrameSemanticFrame: undefined,
    CentaurCollectMainScreenSemanticFrame: undefined,
    GetTimeSemanticFrame: undefined,
    HowToSubscribeSemanticFrame: undefined,
    MusicOnboardingSemanticFrame: undefined,
    MusicOnboardingArtistsSemanticFrame: undefined,
    MusicOnboardingGenresSemanticFrame: undefined,
    MusicOnboardingTracksSemanticFrame: undefined,
    PlayerContinueSemanticFrame: undefined,
    PlayerWhatIsPlayingSemanticFrame: undefined,
    PlayerShuffleSemanticFrame: undefined,
    PlayerReplaySemanticFrame: undefined,
    PlayerRewindSemanticFrame: undefined,
    PlayerRepeatSemanticFrame: undefined,
    GetSmartTvCategoriesSemanticFrame: undefined,
    IoTScenariosPhraseActionSemanticFrame: undefined,
    IoTScenariosTextActionSemanticFrame: undefined,
    IoTScenariosLaunchActionSemanticFrame: undefined,
    AlarmSetAliceShowSemanticFrame: undefined,
    PlayerUnshuffleSemanticFrame: undefined,
    RemindersOnShootSemanticFrame: undefined,
    RepeatAfterMeSemanticFrame: undefined,
    MediaPlaySemanticFrame: undefined,
    AliceShowActivateSemanticFrame: undefined,
    ZenContextSearchStartSemanticFrame: undefined,
    GetSmartTvCarouselSemanticFrame: undefined,
    GetSmartTvCarouselsSemanticFrame: undefined,
    AppsFixlistSemanticFrame: undefined,
    PlayerPauseSemanticFrame: undefined,
    IoTScenarioSpeakerActionSemanticFrame: undefined,
    VideoCardDetailSemanticFrame: undefined,
    TurnClockFaceOnSemanticFrame: undefined,
    TurnClockFaceOffSemanticFrame: undefined,
    RemindersOnCancelSemanticFrame: undefined,
    IoTDeviceActionSemanticFrame: undefined,
    VideoThinCardDetailSmanticFrame: undefined,
    WhisperSaySomethingSemanticFrame: undefined,
    WhisperTurnOffSemanticFrame: undefined,
    WhisperTurnOnSemanticFrame: undefined,
    WhisperWhatIsItSemanticFrame: undefined,
    TimeCapsuleNextStepSemanticFrame: undefined,
    TimeCapsuleStopSemanticFrame: undefined,
    ActivateGenerativeTaleSemanticFrame: undefined,
    StartIotDiscoverySemanticFrame: undefined,
    FinishIotDiscoverySemanticFrame: undefined,
    ForgetIotEndpointsSemanticFrame: undefined,
    HardcodedResponseSemanticFrame: undefined,
    IotYandexIOActionSemanticFrame: undefined,
    TimeCapsuleStartSemanticFrame: undefined,
    TimeCapsuleResumeSemanticFrame: undefined,
    AddAccountSemanticFrame: undefined,
    RemoveAccountSemanticFrame: undefined,
    EndpointStateUpdatesSemanticFrame: undefined,
    TimeCapsuleSkipQuestionSemanticFrame: undefined,
    SkillSessionRequestSemanticFrame: undefined,
    StartIotTuyaBroadcastSemanticFrame: undefined,
    OpenSmartDeviceExternalAppFrame: undefined,
    RestoreIotNetworksSemanticFrame: undefined,
    SaveIotNetworksSemanticFrame: undefined,
    GuestEnrollmentStartSemanticFrame: undefined,
    TvPromoTemplateRequestSemanticFrame: undefined,
    TvPromoTemplateShownReportSemanticFrame: undefined,
    UploadContactsRequestSemanticFrame: undefined,
    DeleteIotNetworksSemanticFrame: undefined,
    CentaurCollectWidgetGallerySemanticFrame: undefined,
    CentaurAddWidgetFromGallerySemanticFrame: undefined,
    UpdateContactsRequestSemanticFrame: undefined,
    RemindersListSemanticFrame: undefined,
    CapabilityEventSemanticFrame: undefined,
    ExternalSkillForceDeactivateSemanticFrame: undefined,
    GetVideoGalleries: undefined,
    EndpointCapabilityEventsSemanticFrame: undefined,
    MusicAnnounceDisableSemanticFrame: undefined,
    MusicAnnounceEnableSemanticFrame: undefined,
    GetTvSearchResult: undefined,
    SwitchTvChannelSemanticFrame: undefined,
    ConvertSemanticFrame: undefined,
    GetVideoGallerySemanticFrame: undefined,
    MusicOnboardingTracksReaskSemanticFrame: undefined,
    TestSemanticFrame: undefined,
    EndpointEventsBatchSemanticFrame: undefined,
    MediaSessionPlaySemanticFrame: undefined,
    MediaSessionPauseSemanticFrame: undefined,
    FmRadioPlaySemanticFrame: undefined,
    VideoCallLoginFailedSemanticFrame: undefined,
    VideoCallOutgoingAcceptedSemanticFrame: undefined,
    VideoCallOutgoingFailedSemanticFrame: undefined,
    VideoCallIncomingAcceptFailedSemanticFrame: undefined,
    IotScenarioStepActionsSemanticFrame: undefined,
    PhoneCallSemanticFrame: undefined,
    OpenAddressBookSemanticFrame: undefined,
    GetEqualizerSettingsSemanticFrame: undefined,
    VideoCallToSemanticFrame: undefined,
    OnboardingGetGreetingsSemanticFrame: undefined,
    OpenTandemSettingSemanticFrame: undefined,
    OpenSmartSpeakerSettingSemanticFrame: undefined,
    FinishIotSystemDiscoverySemanticFrame: undefined,
    VideoCallSetFavoritesSemanticFrame: undefined,
    VideoCallIncomingSemanticFrame: undefined,
    MessengerCallAcceptSemanticFrame: undefined,
    MessengerCallDiscardSemanticFrame: undefined,
    TvLongTapTutorialSemanticFrame: undefined,
    OnboardingWhatCanYouDoSemanticFrame: undefined,
    MessengerCallHangupSemanticFrame: undefined,
    PutMoneyOnPhoneSemanticFrame: undefined,
    OrderNotificationSemanticFrame: undefined,
    PlayerRemoveLikeSemanticFrame: undefined,
    PlayerRemoveDislikeSemanticFrame: undefined,
    CentaurCollectUpperShutterSemanticFrame: undefined,
    EnrollmentStatusSemanticFrame: undefined,
    GalleryVideoSelectSemanticFrame: undefined,
    PlayerSongsByThisArtistSemanticFrame: undefined,
    ExternalSkillEpisodeForShowRequestSemanticFrame: undefined,
    MusicPlayFixlistSemanticFrame: undefined,
    MusicPlayAnaphoraSemanticFrame: undefined,
    MusicPlayFairytaleSemanticFrame: undefined,
    StartMultiroomSemanticFrame: undefined,
    PlayerWhatIsThisSongAboutSemanticFrame: undefined,
    GuestEnrollmentFinishSemanticFrame: undefined,
    CentaurSetTeaserConfigurationSemanticFrame: undefined,
    CentaurCollectTeasersPreviewSemanticFrame: undefined,
  };
}

export const TTypedSemanticFrame = {
  encode(
    message: TTypedSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SearchSemanticFrame !== undefined) {
      TSearchSemanticFrame.encode(
        message.SearchSemanticFrame,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.IoTBroadcastStartSemanticFrame !== undefined) {
      TIoTBroadcastStartSemanticFrame.encode(
        message.IoTBroadcastStartSemanticFrame,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.IoTBroadcastSuccessSemanticFrame !== undefined) {
      TIoTBroadcastSuccessSemanticFrame.encode(
        message.IoTBroadcastSuccessSemanticFrame,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.IoTBroadcastFailureSemanticFrame !== undefined) {
      TIoTBroadcastFailureSemanticFrame.encode(
        message.IoTBroadcastFailureSemanticFrame,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.MordoviaHomeScreenSemanticFrame !== undefined) {
      TMordoviaHomeScreenSemanticFrame.encode(
        message.MordoviaHomeScreenSemanticFrame,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.NewsSemanticFrame !== undefined) {
      TNewsSemanticFrame.encode(
        message.NewsSemanticFrame,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.GetCallerNameSemanticFrame !== undefined) {
      TGetCallerNameSemanticFrame.encode(
        message.GetCallerNameSemanticFrame,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.MusicPlaySemanticFrame !== undefined) {
      TMusicPlaySemanticFrame.encode(
        message.MusicPlaySemanticFrame,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.ExternalSkillActivateSemanticFrame !== undefined) {
      TExternalSkillActivateSemanticFrame.encode(
        message.ExternalSkillActivateSemanticFrame,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.VideoPlaySemanticFrame !== undefined) {
      TVideoPlaySemanticFrame.encode(
        message.VideoPlaySemanticFrame,
        writer.uint32(82).fork()
      ).ldelim();
    }
    if (message.WeatherSemanticFrame !== undefined) {
      TWeatherSemanticFrame.encode(
        message.WeatherSemanticFrame,
        writer.uint32(90).fork()
      ).ldelim();
    }
    if (message.HardcodedMorningShowSemanticFrame !== undefined) {
      THardcodedMorningShowSemanticFrame.encode(
        message.HardcodedMorningShowSemanticFrame,
        writer.uint32(98).fork()
      ).ldelim();
    }
    if (message.SelectVideoFromGallerySemanticFrame !== undefined) {
      TSelectVideoFromGallerySemanticFrame.encode(
        message.SelectVideoFromGallerySemanticFrame,
        writer.uint32(106).fork()
      ).ldelim();
    }
    if (message.OpenCurrentVideoSemanticFrame !== undefined) {
      TOpenCurrentVideoSemanticFrame.encode(
        message.OpenCurrentVideoSemanticFrame,
        writer.uint32(114).fork()
      ).ldelim();
    }
    if (message.VideoPaymentConfirmedSemanticFrame !== undefined) {
      TVideoPaymentConfirmedSemanticFrame.encode(
        message.VideoPaymentConfirmedSemanticFrame,
        writer.uint32(122).fork()
      ).ldelim();
    }
    if (message.PlayerNextTrackSemanticFrame !== undefined) {
      TPlayerNextTrackSemanticFrame.encode(
        message.PlayerNextTrackSemanticFrame,
        writer.uint32(130).fork()
      ).ldelim();
    }
    if (message.PlayerPrevTrackSemanticFrame !== undefined) {
      TPlayerPrevTrackSemanticFrame.encode(
        message.PlayerPrevTrackSemanticFrame,
        writer.uint32(138).fork()
      ).ldelim();
    }
    if (message.PlayerLikeSemanticFrame !== undefined) {
      TPlayerLikeSemanticFrame.encode(
        message.PlayerLikeSemanticFrame,
        writer.uint32(146).fork()
      ).ldelim();
    }
    if (message.PlayerDislikeSemanticFrame !== undefined) {
      TPlayerDislikeSemanticFrame.encode(
        message.PlayerDislikeSemanticFrame,
        writer.uint32(154).fork()
      ).ldelim();
    }
    if (message.DoNothingSemanticFrame !== undefined) {
      TDoNothingSemanticFrame.encode(
        message.DoNothingSemanticFrame,
        writer.uint32(162).fork()
      ).ldelim();
    }
    if (message.NotificationsSubscribeSemanticFrame !== undefined) {
      TNotificationsSubscribeSemanticFrame.encode(
        message.NotificationsSubscribeSemanticFrame,
        writer.uint32(170).fork()
      ).ldelim();
    }
    if (message.VideoRaterSemanticFrame !== undefined) {
      TVideoRaterSemanticFrame.encode(
        message.VideoRaterSemanticFrame,
        writer.uint32(178).fork()
      ).ldelim();
    }
    if (message.SetupRcuStatusSemanticFrame !== undefined) {
      TSetupRcuStatusSemanticFrame.encode(
        message.SetupRcuStatusSemanticFrame,
        writer.uint32(186).fork()
      ).ldelim();
    }
    if (message.SetupRcuAutoStatusSemanticFrame !== undefined) {
      TSetupRcuAutoStatusSemanticFrame.encode(
        message.SetupRcuAutoStatusSemanticFrame,
        writer.uint32(194).fork()
      ).ldelim();
    }
    if (message.SetupRcuCheckStatusSemanticFrame !== undefined) {
      TSetupRcuCheckStatusSemanticFrame.encode(
        message.SetupRcuCheckStatusSemanticFrame,
        writer.uint32(202).fork()
      ).ldelim();
    }
    if (message.SetupRcuAdvancedStatusSemanticFrame !== undefined) {
      TSetupRcuAdvancedStatusSemanticFrame.encode(
        message.SetupRcuAdvancedStatusSemanticFrame,
        writer.uint32(210).fork()
      ).ldelim();
    }
    if (message.SetupRcuManualStartSemanticFrame !== undefined) {
      TSetupRcuManualStartSemanticFrame.encode(
        message.SetupRcuManualStartSemanticFrame,
        writer.uint32(218).fork()
      ).ldelim();
    }
    if (message.SetupRcuAutoStartSemanticFrame !== undefined) {
      TSetupRcuAutoStartSemanticFrame.encode(
        message.SetupRcuAutoStartSemanticFrame,
        writer.uint32(226).fork()
      ).ldelim();
    }
    if (message.LinkARemoteSemanticFrame !== undefined) {
      TLinkARemoteSemanticFrame.encode(
        message.LinkARemoteSemanticFrame,
        writer.uint32(234).fork()
      ).ldelim();
    }
    if (message.RequestTechnicalSupportSemanticFrame !== undefined) {
      TRequestTechnicalSupportSemanticFrame.encode(
        message.RequestTechnicalSupportSemanticFrame,
        writer.uint32(242).fork()
      ).ldelim();
    }
    if (message.IoTDiscoveryStartSemanticFrame !== undefined) {
      TIoTDiscoveryStartSemanticFrame.encode(
        message.IoTDiscoveryStartSemanticFrame,
        writer.uint32(250).fork()
      ).ldelim();
    }
    if (message.IoTDiscoverySuccessSemanticFrame !== undefined) {
      TIoTDiscoverySuccessSemanticFrame.encode(
        message.IoTDiscoverySuccessSemanticFrame,
        writer.uint32(258).fork()
      ).ldelim();
    }
    if (message.IoTDiscoveryFailureSemanticFrame !== undefined) {
      TIoTDiscoveryFailureSemanticFrame.encode(
        message.IoTDiscoveryFailureSemanticFrame,
        writer.uint32(266).fork()
      ).ldelim();
    }
    if (message.ExternalSkillFixedActivateSemanticFrame !== undefined) {
      TExternalSkillFixedActivateSemanticFrame.encode(
        message.ExternalSkillFixedActivateSemanticFrame,
        writer.uint32(274).fork()
      ).ldelim();
    }
    if (message.OpenCurrentTrailerSemanticFrame !== undefined) {
      TOpenCurrentTrailerSemanticFrame.encode(
        message.OpenCurrentTrailerSemanticFrame,
        writer.uint32(282).fork()
      ).ldelim();
    }
    if (message.OnboardingStartingCriticalUpdateSemanticFrame !== undefined) {
      TOnboardingStartingCriticalUpdateSemanticFrame.encode(
        message.OnboardingStartingCriticalUpdateSemanticFrame,
        writer.uint32(290).fork()
      ).ldelim();
    }
    if (message.OnboardingStartingConfigureSuccessSemanticFrame !== undefined) {
      TOnboardingStartingConfigureSuccessSemanticFrame.encode(
        message.OnboardingStartingConfigureSuccessSemanticFrame,
        writer.uint32(298).fork()
      ).ldelim();
    }
    if (message.RadioPlaySemanticFrame !== undefined) {
      TRadioPlaySemanticFrame.encode(
        message.RadioPlaySemanticFrame,
        writer.uint32(306).fork()
      ).ldelim();
    }
    if (message.CentaurCollectCardsSemanticFrame !== undefined) {
      TCentaurCollectCardsSemanticFrame.encode(
        message.CentaurCollectCardsSemanticFrame,
        writer.uint32(314).fork()
      ).ldelim();
    }
    if (message.CentaurGetCardSemanticFrame !== undefined) {
      TCentaurGetCardSemanticFrame.encode(
        message.CentaurGetCardSemanticFrame,
        writer.uint32(322).fork()
      ).ldelim();
    }
    if (message.VideoPlayerFinishedSemanticFrame !== undefined) {
      TVideoPlayerFinishedSemanticFrame.encode(
        message.VideoPlayerFinishedSemanticFrame,
        writer.uint32(330).fork()
      ).ldelim();
    }
    if (message.SoundLouderSemanticFrame !== undefined) {
      TSoundLouderSemanticFrame.encode(
        message.SoundLouderSemanticFrame,
        writer.uint32(338).fork()
      ).ldelim();
    }
    if (message.SoundQuiterSemanticFrame !== undefined) {
      TSoundQuiterSemanticFrame.encode(
        message.SoundQuiterSemanticFrame,
        writer.uint32(346).fork()
      ).ldelim();
    }
    if (message.SoundSetLevelSemanticFrame !== undefined) {
      TSoundSetLevelSemanticFrame.encode(
        message.SoundSetLevelSemanticFrame,
        writer.uint32(354).fork()
      ).ldelim();
    }
    if (message.GetPhotoFrameSemanticFrame !== undefined) {
      TGetPhotoFrameSemanticFrame.encode(
        message.GetPhotoFrameSemanticFrame,
        writer.uint32(362).fork()
      ).ldelim();
    }
    if (message.CentaurCollectMainScreenSemanticFrame !== undefined) {
      TCentaurCollectMainScreenSemanticFrame.encode(
        message.CentaurCollectMainScreenSemanticFrame,
        writer.uint32(370).fork()
      ).ldelim();
    }
    if (message.GetTimeSemanticFrame !== undefined) {
      TGetTimeSemanticFrame.encode(
        message.GetTimeSemanticFrame,
        writer.uint32(378).fork()
      ).ldelim();
    }
    if (message.HowToSubscribeSemanticFrame !== undefined) {
      THowToSubscribeSemanticFrame.encode(
        message.HowToSubscribeSemanticFrame,
        writer.uint32(386).fork()
      ).ldelim();
    }
    if (message.MusicOnboardingSemanticFrame !== undefined) {
      TMusicOnboardingSemanticFrame.encode(
        message.MusicOnboardingSemanticFrame,
        writer.uint32(394).fork()
      ).ldelim();
    }
    if (message.MusicOnboardingArtistsSemanticFrame !== undefined) {
      TMusicOnboardingArtistsSemanticFrame.encode(
        message.MusicOnboardingArtistsSemanticFrame,
        writer.uint32(402).fork()
      ).ldelim();
    }
    if (message.MusicOnboardingGenresSemanticFrame !== undefined) {
      TMusicOnboardingGenresSemanticFrame.encode(
        message.MusicOnboardingGenresSemanticFrame,
        writer.uint32(410).fork()
      ).ldelim();
    }
    if (message.MusicOnboardingTracksSemanticFrame !== undefined) {
      TMusicOnboardingTracksSemanticFrame.encode(
        message.MusicOnboardingTracksSemanticFrame,
        writer.uint32(418).fork()
      ).ldelim();
    }
    if (message.PlayerContinueSemanticFrame !== undefined) {
      TPlayerContinueSemanticFrame.encode(
        message.PlayerContinueSemanticFrame,
        writer.uint32(426).fork()
      ).ldelim();
    }
    if (message.PlayerWhatIsPlayingSemanticFrame !== undefined) {
      TPlayerWhatIsPlayingSemanticFrame.encode(
        message.PlayerWhatIsPlayingSemanticFrame,
        writer.uint32(434).fork()
      ).ldelim();
    }
    if (message.PlayerShuffleSemanticFrame !== undefined) {
      TPlayerShuffleSemanticFrame.encode(
        message.PlayerShuffleSemanticFrame,
        writer.uint32(442).fork()
      ).ldelim();
    }
    if (message.PlayerReplaySemanticFrame !== undefined) {
      TPlayerReplaySemanticFrame.encode(
        message.PlayerReplaySemanticFrame,
        writer.uint32(450).fork()
      ).ldelim();
    }
    if (message.PlayerRewindSemanticFrame !== undefined) {
      TPlayerRewindSemanticFrame.encode(
        message.PlayerRewindSemanticFrame,
        writer.uint32(458).fork()
      ).ldelim();
    }
    if (message.PlayerRepeatSemanticFrame !== undefined) {
      TPlayerRepeatSemanticFrame.encode(
        message.PlayerRepeatSemanticFrame,
        writer.uint32(466).fork()
      ).ldelim();
    }
    if (message.GetSmartTvCategoriesSemanticFrame !== undefined) {
      TGetSmartTvCategoriesSemanticFrame.encode(
        message.GetSmartTvCategoriesSemanticFrame,
        writer.uint32(474).fork()
      ).ldelim();
    }
    if (message.IoTScenariosPhraseActionSemanticFrame !== undefined) {
      TIoTScenariosPhraseActionSemanticFrame.encode(
        message.IoTScenariosPhraseActionSemanticFrame,
        writer.uint32(482).fork()
      ).ldelim();
    }
    if (message.IoTScenariosTextActionSemanticFrame !== undefined) {
      TIoTScenariosTextActionSemanticFrame.encode(
        message.IoTScenariosTextActionSemanticFrame,
        writer.uint32(490).fork()
      ).ldelim();
    }
    if (message.IoTScenariosLaunchActionSemanticFrame !== undefined) {
      TIoTScenariosLaunchActionSemanticFrame.encode(
        message.IoTScenariosLaunchActionSemanticFrame,
        writer.uint32(498).fork()
      ).ldelim();
    }
    if (message.AlarmSetAliceShowSemanticFrame !== undefined) {
      TAlarmSetAliceShowSemanticFrame.encode(
        message.AlarmSetAliceShowSemanticFrame,
        writer.uint32(506).fork()
      ).ldelim();
    }
    if (message.PlayerUnshuffleSemanticFrame !== undefined) {
      TPlayerUnshuffleSemanticFrame.encode(
        message.PlayerUnshuffleSemanticFrame,
        writer.uint32(514).fork()
      ).ldelim();
    }
    if (message.RemindersOnShootSemanticFrame !== undefined) {
      TRemindersOnShootSemanticFrame.encode(
        message.RemindersOnShootSemanticFrame,
        writer.uint32(530).fork()
      ).ldelim();
    }
    if (message.RepeatAfterMeSemanticFrame !== undefined) {
      TRepeatAfterMeSemanticFrame.encode(
        message.RepeatAfterMeSemanticFrame,
        writer.uint32(538).fork()
      ).ldelim();
    }
    if (message.MediaPlaySemanticFrame !== undefined) {
      TMediaPlaySemanticFrame.encode(
        message.MediaPlaySemanticFrame,
        writer.uint32(546).fork()
      ).ldelim();
    }
    if (message.AliceShowActivateSemanticFrame !== undefined) {
      TAliceShowActivateSemanticFrame.encode(
        message.AliceShowActivateSemanticFrame,
        writer.uint32(554).fork()
      ).ldelim();
    }
    if (message.ZenContextSearchStartSemanticFrame !== undefined) {
      TZenContextSearchStartSemanticFrame.encode(
        message.ZenContextSearchStartSemanticFrame,
        writer.uint32(562).fork()
      ).ldelim();
    }
    if (message.GetSmartTvCarouselSemanticFrame !== undefined) {
      TGetSmartTvCarouselSemanticFrame.encode(
        message.GetSmartTvCarouselSemanticFrame,
        writer.uint32(570).fork()
      ).ldelim();
    }
    if (message.GetSmartTvCarouselsSemanticFrame !== undefined) {
      TGetSmartTvCarouselsSemanticFrame.encode(
        message.GetSmartTvCarouselsSemanticFrame,
        writer.uint32(578).fork()
      ).ldelim();
    }
    if (message.AppsFixlistSemanticFrame !== undefined) {
      TAppsFixlistSemanticFrame.encode(
        message.AppsFixlistSemanticFrame,
        writer.uint32(586).fork()
      ).ldelim();
    }
    if (message.PlayerPauseSemanticFrame !== undefined) {
      TPlayerPauseSemanticFrame.encode(
        message.PlayerPauseSemanticFrame,
        writer.uint32(594).fork()
      ).ldelim();
    }
    if (message.IoTScenarioSpeakerActionSemanticFrame !== undefined) {
      TIoTScenarioSpeakerActionSemanticFrame.encode(
        message.IoTScenarioSpeakerActionSemanticFrame,
        writer.uint32(602).fork()
      ).ldelim();
    }
    if (message.VideoCardDetailSemanticFrame !== undefined) {
      TVideoCardDetailSemanticFrame.encode(
        message.VideoCardDetailSemanticFrame,
        writer.uint32(610).fork()
      ).ldelim();
    }
    if (message.TurnClockFaceOnSemanticFrame !== undefined) {
      TTurnClockFaceOnSemanticFrame.encode(
        message.TurnClockFaceOnSemanticFrame,
        writer.uint32(618).fork()
      ).ldelim();
    }
    if (message.TurnClockFaceOffSemanticFrame !== undefined) {
      TTurnClockFaceOffSemanticFrame.encode(
        message.TurnClockFaceOffSemanticFrame,
        writer.uint32(626).fork()
      ).ldelim();
    }
    if (message.RemindersOnCancelSemanticFrame !== undefined) {
      TRemindersCancelSemanticFrame.encode(
        message.RemindersOnCancelSemanticFrame,
        writer.uint32(634).fork()
      ).ldelim();
    }
    if (message.IoTDeviceActionSemanticFrame !== undefined) {
      TIoTDeviceActionSemanticFrame.encode(
        message.IoTDeviceActionSemanticFrame,
        writer.uint32(642).fork()
      ).ldelim();
    }
    if (message.VideoThinCardDetailSmanticFrame !== undefined) {
      TVideoThinCardDetailSemanticFrame.encode(
        message.VideoThinCardDetailSmanticFrame,
        writer.uint32(650).fork()
      ).ldelim();
    }
    if (message.WhisperSaySomethingSemanticFrame !== undefined) {
      TWhisperSaySomethingSemanticFrame.encode(
        message.WhisperSaySomethingSemanticFrame,
        writer.uint32(658).fork()
      ).ldelim();
    }
    if (message.WhisperTurnOffSemanticFrame !== undefined) {
      TWhisperTurnOffSemanticFrame.encode(
        message.WhisperTurnOffSemanticFrame,
        writer.uint32(666).fork()
      ).ldelim();
    }
    if (message.WhisperTurnOnSemanticFrame !== undefined) {
      TWhisperTurnOnSemanticFrame.encode(
        message.WhisperTurnOnSemanticFrame,
        writer.uint32(674).fork()
      ).ldelim();
    }
    if (message.WhisperWhatIsItSemanticFrame !== undefined) {
      TWhisperWhatIsItSemanticFrame.encode(
        message.WhisperWhatIsItSemanticFrame,
        writer.uint32(682).fork()
      ).ldelim();
    }
    if (message.TimeCapsuleNextStepSemanticFrame !== undefined) {
      TTimeCapsuleNextStepSemanticFrame.encode(
        message.TimeCapsuleNextStepSemanticFrame,
        writer.uint32(690).fork()
      ).ldelim();
    }
    if (message.TimeCapsuleStopSemanticFrame !== undefined) {
      TTimeCapsuleStopSemanticFrame.encode(
        message.TimeCapsuleStopSemanticFrame,
        writer.uint32(698).fork()
      ).ldelim();
    }
    if (message.ActivateGenerativeTaleSemanticFrame !== undefined) {
      TActivateGenerativeTaleSemanticFrame.encode(
        message.ActivateGenerativeTaleSemanticFrame,
        writer.uint32(706).fork()
      ).ldelim();
    }
    if (message.StartIotDiscoverySemanticFrame !== undefined) {
      TStartIotDiscoverySemanticFrame.encode(
        message.StartIotDiscoverySemanticFrame,
        writer.uint32(714).fork()
      ).ldelim();
    }
    if (message.FinishIotDiscoverySemanticFrame !== undefined) {
      TFinishIotDiscoverySemanticFrame.encode(
        message.FinishIotDiscoverySemanticFrame,
        writer.uint32(722).fork()
      ).ldelim();
    }
    if (message.ForgetIotEndpointsSemanticFrame !== undefined) {
      TForgetIotEndpointsSemanticFrame.encode(
        message.ForgetIotEndpointsSemanticFrame,
        writer.uint32(730).fork()
      ).ldelim();
    }
    if (message.HardcodedResponseSemanticFrame !== undefined) {
      THardcodedResponseSemanticFrame.encode(
        message.HardcodedResponseSemanticFrame,
        writer.uint32(738).fork()
      ).ldelim();
    }
    if (message.IotYandexIOActionSemanticFrame !== undefined) {
      TIotYandexIOActionSemanticFrame.encode(
        message.IotYandexIOActionSemanticFrame,
        writer.uint32(746).fork()
      ).ldelim();
    }
    if (message.TimeCapsuleStartSemanticFrame !== undefined) {
      TTimeCapsuleStartSemanticFrame.encode(
        message.TimeCapsuleStartSemanticFrame,
        writer.uint32(754).fork()
      ).ldelim();
    }
    if (message.TimeCapsuleResumeSemanticFrame !== undefined) {
      TTimeCapsuleResumeSemanticFrame.encode(
        message.TimeCapsuleResumeSemanticFrame,
        writer.uint32(762).fork()
      ).ldelim();
    }
    if (message.AddAccountSemanticFrame !== undefined) {
      TAddAccountSemanticFrame.encode(
        message.AddAccountSemanticFrame,
        writer.uint32(770).fork()
      ).ldelim();
    }
    if (message.RemoveAccountSemanticFrame !== undefined) {
      TRemoveAccountSemanticFrame.encode(
        message.RemoveAccountSemanticFrame,
        writer.uint32(778).fork()
      ).ldelim();
    }
    if (message.EndpointStateUpdatesSemanticFrame !== undefined) {
      TEndpointStateUpdatesSemanticFrame.encode(
        message.EndpointStateUpdatesSemanticFrame,
        writer.uint32(786).fork()
      ).ldelim();
    }
    if (message.TimeCapsuleSkipQuestionSemanticFrame !== undefined) {
      TTimeCapsuleSkipQuestionSemanticFrame.encode(
        message.TimeCapsuleSkipQuestionSemanticFrame,
        writer.uint32(802).fork()
      ).ldelim();
    }
    if (message.SkillSessionRequestSemanticFrame !== undefined) {
      TSkillSessionRequestSemanticFrame.encode(
        message.SkillSessionRequestSemanticFrame,
        writer.uint32(810).fork()
      ).ldelim();
    }
    if (message.StartIotTuyaBroadcastSemanticFrame !== undefined) {
      TStartIotTuyaBroadcastSemanticFrame.encode(
        message.StartIotTuyaBroadcastSemanticFrame,
        writer.uint32(818).fork()
      ).ldelim();
    }
    if (message.OpenSmartDeviceExternalAppFrame !== undefined) {
      TOpenSmartDeviceExternalAppFrame.encode(
        message.OpenSmartDeviceExternalAppFrame,
        writer.uint32(826).fork()
      ).ldelim();
    }
    if (message.RestoreIotNetworksSemanticFrame !== undefined) {
      TRestoreIotNetworksSemanticFrame.encode(
        message.RestoreIotNetworksSemanticFrame,
        writer.uint32(834).fork()
      ).ldelim();
    }
    if (message.SaveIotNetworksSemanticFrame !== undefined) {
      TSaveIotNetworksSemanticFrame.encode(
        message.SaveIotNetworksSemanticFrame,
        writer.uint32(842).fork()
      ).ldelim();
    }
    if (message.GuestEnrollmentStartSemanticFrame !== undefined) {
      TGuestEnrollmentStartSemanticFrame.encode(
        message.GuestEnrollmentStartSemanticFrame,
        writer.uint32(850).fork()
      ).ldelim();
    }
    if (message.TvPromoTemplateRequestSemanticFrame !== undefined) {
      TTvPromoTemplateRequestSemanticFrame.encode(
        message.TvPromoTemplateRequestSemanticFrame,
        writer.uint32(858).fork()
      ).ldelim();
    }
    if (message.TvPromoTemplateShownReportSemanticFrame !== undefined) {
      TTvPromoTemplateShownReportSemanticFrame.encode(
        message.TvPromoTemplateShownReportSemanticFrame,
        writer.uint32(866).fork()
      ).ldelim();
    }
    if (message.UploadContactsRequestSemanticFrame !== undefined) {
      TUploadContactsRequestSemanticFrame.encode(
        message.UploadContactsRequestSemanticFrame,
        writer.uint32(874).fork()
      ).ldelim();
    }
    if (message.DeleteIotNetworksSemanticFrame !== undefined) {
      TDeleteIotNetworksSemanticFrame.encode(
        message.DeleteIotNetworksSemanticFrame,
        writer.uint32(882).fork()
      ).ldelim();
    }
    if (message.CentaurCollectWidgetGallerySemanticFrame !== undefined) {
      TCentaurCollectWidgetGallerySemanticFrame.encode(
        message.CentaurCollectWidgetGallerySemanticFrame,
        writer.uint32(890).fork()
      ).ldelim();
    }
    if (message.CentaurAddWidgetFromGallerySemanticFrame !== undefined) {
      TCentaurAddWidgetFromGallerySemanticFrame.encode(
        message.CentaurAddWidgetFromGallerySemanticFrame,
        writer.uint32(898).fork()
      ).ldelim();
    }
    if (message.UpdateContactsRequestSemanticFrame !== undefined) {
      TUpdateContactsRequestSemanticFrame.encode(
        message.UpdateContactsRequestSemanticFrame,
        writer.uint32(906).fork()
      ).ldelim();
    }
    if (message.RemindersListSemanticFrame !== undefined) {
      TRemindersListSemanticFrame.encode(
        message.RemindersListSemanticFrame,
        writer.uint32(914).fork()
      ).ldelim();
    }
    if (message.CapabilityEventSemanticFrame !== undefined) {
      TCapabilityEventSemanticFrame.encode(
        message.CapabilityEventSemanticFrame,
        writer.uint32(922).fork()
      ).ldelim();
    }
    if (message.ExternalSkillForceDeactivateSemanticFrame !== undefined) {
      TExternalSkillForceDeactivateSemanticFrame.encode(
        message.ExternalSkillForceDeactivateSemanticFrame,
        writer.uint32(930).fork()
      ).ldelim();
    }
    if (message.GetVideoGalleries !== undefined) {
      TGetVideoGalleriesSemanticFrame.encode(
        message.GetVideoGalleries,
        writer.uint32(938).fork()
      ).ldelim();
    }
    if (message.EndpointCapabilityEventsSemanticFrame !== undefined) {
      TEndpointCapabilityEventsSemanticFrame.encode(
        message.EndpointCapabilityEventsSemanticFrame,
        writer.uint32(946).fork()
      ).ldelim();
    }
    if (message.MusicAnnounceDisableSemanticFrame !== undefined) {
      TMusicAnnounceDisableSemanticFrame.encode(
        message.MusicAnnounceDisableSemanticFrame,
        writer.uint32(954).fork()
      ).ldelim();
    }
    if (message.MusicAnnounceEnableSemanticFrame !== undefined) {
      TMusicAnnounceEnableSemanticFrame.encode(
        message.MusicAnnounceEnableSemanticFrame,
        writer.uint32(962).fork()
      ).ldelim();
    }
    if (message.GetTvSearchResult !== undefined) {
      TGetTvSearchResultSemanticFrame.encode(
        message.GetTvSearchResult,
        writer.uint32(970).fork()
      ).ldelim();
    }
    if (message.SwitchTvChannelSemanticFrame !== undefined) {
      TSwitchTvChannelSemanticFrame.encode(
        message.SwitchTvChannelSemanticFrame,
        writer.uint32(978).fork()
      ).ldelim();
    }
    if (message.ConvertSemanticFrame !== undefined) {
      TConvertSemanticFrame.encode(
        message.ConvertSemanticFrame,
        writer.uint32(986).fork()
      ).ldelim();
    }
    if (message.GetVideoGallerySemanticFrame !== undefined) {
      TGetVideoGallerySemanticFrame.encode(
        message.GetVideoGallerySemanticFrame,
        writer.uint32(994).fork()
      ).ldelim();
    }
    if (message.MusicOnboardingTracksReaskSemanticFrame !== undefined) {
      TMusicOnboardingTracksReaskSemanticFrame.encode(
        message.MusicOnboardingTracksReaskSemanticFrame,
        writer.uint32(1002).fork()
      ).ldelim();
    }
    if (message.TestSemanticFrame !== undefined) {
      TTestSemanticFrame.encode(
        message.TestSemanticFrame,
        writer.uint32(1010).fork()
      ).ldelim();
    }
    if (message.EndpointEventsBatchSemanticFrame !== undefined) {
      TEndpointEventsBatchSemanticFrame.encode(
        message.EndpointEventsBatchSemanticFrame,
        writer.uint32(1018).fork()
      ).ldelim();
    }
    if (message.MediaSessionPlaySemanticFrame !== undefined) {
      TMediaSessionPlaySemanticFrame.encode(
        message.MediaSessionPlaySemanticFrame,
        writer.uint32(1026).fork()
      ).ldelim();
    }
    if (message.MediaSessionPauseSemanticFrame !== undefined) {
      TMediaSessionPauseSemanticFrame.encode(
        message.MediaSessionPauseSemanticFrame,
        writer.uint32(1034).fork()
      ).ldelim();
    }
    if (message.FmRadioPlaySemanticFrame !== undefined) {
      TFmRadioPlaySemanticFrame.encode(
        message.FmRadioPlaySemanticFrame,
        writer.uint32(1042).fork()
      ).ldelim();
    }
    if (message.VideoCallLoginFailedSemanticFrame !== undefined) {
      TVideoCallLoginFailedSemanticFrame.encode(
        message.VideoCallLoginFailedSemanticFrame,
        writer.uint32(1050).fork()
      ).ldelim();
    }
    if (message.VideoCallOutgoingAcceptedSemanticFrame !== undefined) {
      TVideoCallOutgoingAcceptedSemanticFrame.encode(
        message.VideoCallOutgoingAcceptedSemanticFrame,
        writer.uint32(1058).fork()
      ).ldelim();
    }
    if (message.VideoCallOutgoingFailedSemanticFrame !== undefined) {
      TVideoCallOutgoingFailedSemanticFrame.encode(
        message.VideoCallOutgoingFailedSemanticFrame,
        writer.uint32(1066).fork()
      ).ldelim();
    }
    if (message.VideoCallIncomingAcceptFailedSemanticFrame !== undefined) {
      TVideoCallIncomingAcceptFailedSemanticFrame.encode(
        message.VideoCallIncomingAcceptFailedSemanticFrame,
        writer.uint32(1074).fork()
      ).ldelim();
    }
    if (message.IotScenarioStepActionsSemanticFrame !== undefined) {
      TIotScenarioStepActionsSemanticFrame.encode(
        message.IotScenarioStepActionsSemanticFrame,
        writer.uint32(1082).fork()
      ).ldelim();
    }
    if (message.PhoneCallSemanticFrame !== undefined) {
      TPhoneCallSemanticFrame.encode(
        message.PhoneCallSemanticFrame,
        writer.uint32(1090).fork()
      ).ldelim();
    }
    if (message.OpenAddressBookSemanticFrame !== undefined) {
      TOpenAddressBookSemanticFrame.encode(
        message.OpenAddressBookSemanticFrame,
        writer.uint32(1098).fork()
      ).ldelim();
    }
    if (message.GetEqualizerSettingsSemanticFrame !== undefined) {
      TGetEqualizerSettingsSemanticFrame.encode(
        message.GetEqualizerSettingsSemanticFrame,
        writer.uint32(1106).fork()
      ).ldelim();
    }
    if (message.VideoCallToSemanticFrame !== undefined) {
      TVideoCallToSemanticFrame.encode(
        message.VideoCallToSemanticFrame,
        writer.uint32(1114).fork()
      ).ldelim();
    }
    if (message.OnboardingGetGreetingsSemanticFrame !== undefined) {
      TOnboardingGetGreetingsSemanticFrame.encode(
        message.OnboardingGetGreetingsSemanticFrame,
        writer.uint32(1122).fork()
      ).ldelim();
    }
    if (message.OpenTandemSettingSemanticFrame !== undefined) {
      TOpenTandemSettingSemanticFrame.encode(
        message.OpenTandemSettingSemanticFrame,
        writer.uint32(1130).fork()
      ).ldelim();
    }
    if (message.OpenSmartSpeakerSettingSemanticFrame !== undefined) {
      TOpenSmartSpeakerSettingSemanticFrame.encode(
        message.OpenSmartSpeakerSettingSemanticFrame,
        writer.uint32(1138).fork()
      ).ldelim();
    }
    if (message.FinishIotSystemDiscoverySemanticFrame !== undefined) {
      TFinishIotSystemDiscoverySemanticFrame.encode(
        message.FinishIotSystemDiscoverySemanticFrame,
        writer.uint32(1146).fork()
      ).ldelim();
    }
    if (message.VideoCallSetFavoritesSemanticFrame !== undefined) {
      TVideoCallSetFavoritesSemanticFrame.encode(
        message.VideoCallSetFavoritesSemanticFrame,
        writer.uint32(1154).fork()
      ).ldelim();
    }
    if (message.VideoCallIncomingSemanticFrame !== undefined) {
      TVideoCallIncomingSemanticFrame.encode(
        message.VideoCallIncomingSemanticFrame,
        writer.uint32(1162).fork()
      ).ldelim();
    }
    if (message.MessengerCallAcceptSemanticFrame !== undefined) {
      TMessengerCallAcceptSemanticFrame.encode(
        message.MessengerCallAcceptSemanticFrame,
        writer.uint32(1170).fork()
      ).ldelim();
    }
    if (message.MessengerCallDiscardSemanticFrame !== undefined) {
      TMessengerCallDiscardSemanticFrame.encode(
        message.MessengerCallDiscardSemanticFrame,
        writer.uint32(1178).fork()
      ).ldelim();
    }
    if (message.TvLongTapTutorialSemanticFrame !== undefined) {
      TTvLongTapTutorialSemanticFrame.encode(
        message.TvLongTapTutorialSemanticFrame,
        writer.uint32(1186).fork()
      ).ldelim();
    }
    if (message.OnboardingWhatCanYouDoSemanticFrame !== undefined) {
      TOnboardingWhatCanYouDoSemanticFrame.encode(
        message.OnboardingWhatCanYouDoSemanticFrame,
        writer.uint32(1194).fork()
      ).ldelim();
    }
    if (message.MessengerCallHangupSemanticFrame !== undefined) {
      TMessengerCallHangupSemanticFrame.encode(
        message.MessengerCallHangupSemanticFrame,
        writer.uint32(1202).fork()
      ).ldelim();
    }
    if (message.PutMoneyOnPhoneSemanticFrame !== undefined) {
      TPutMoneyOnPhoneSemanticFrame.encode(
        message.PutMoneyOnPhoneSemanticFrame,
        writer.uint32(1210).fork()
      ).ldelim();
    }
    if (message.OrderNotificationSemanticFrame !== undefined) {
      TOrderNotificationSemanticFrame.encode(
        message.OrderNotificationSemanticFrame,
        writer.uint32(1218).fork()
      ).ldelim();
    }
    if (message.PlayerRemoveLikeSemanticFrame !== undefined) {
      TPlayerRemoveLikeSemanticFrame.encode(
        message.PlayerRemoveLikeSemanticFrame,
        writer.uint32(1226).fork()
      ).ldelim();
    }
    if (message.PlayerRemoveDislikeSemanticFrame !== undefined) {
      TPlayerRemoveDislikeSemanticFrame.encode(
        message.PlayerRemoveDislikeSemanticFrame,
        writer.uint32(1234).fork()
      ).ldelim();
    }
    if (message.CentaurCollectUpperShutterSemanticFrame !== undefined) {
      TCentaurCollectUpperShutterSemanticFrame.encode(
        message.CentaurCollectUpperShutterSemanticFrame,
        writer.uint32(1242).fork()
      ).ldelim();
    }
    if (message.EnrollmentStatusSemanticFrame !== undefined) {
      TEnrollmentStatusSemanticFrame.encode(
        message.EnrollmentStatusSemanticFrame,
        writer.uint32(1250).fork()
      ).ldelim();
    }
    if (message.GalleryVideoSelectSemanticFrame !== undefined) {
      TGalleryVideoSelectSemanticFrame.encode(
        message.GalleryVideoSelectSemanticFrame,
        writer.uint32(1258).fork()
      ).ldelim();
    }
    if (message.PlayerSongsByThisArtistSemanticFrame !== undefined) {
      TPlayerSongsByThisArtistSemanticFrame.encode(
        message.PlayerSongsByThisArtistSemanticFrame,
        writer.uint32(1266).fork()
      ).ldelim();
    }
    if (message.ExternalSkillEpisodeForShowRequestSemanticFrame !== undefined) {
      TExternalSkillEpisodeForShowRequestSemanticFrame.encode(
        message.ExternalSkillEpisodeForShowRequestSemanticFrame,
        writer.uint32(1274).fork()
      ).ldelim();
    }
    if (message.MusicPlayFixlistSemanticFrame !== undefined) {
      TMusicPlayFixlistSemanticFrame.encode(
        message.MusicPlayFixlistSemanticFrame,
        writer.uint32(1282).fork()
      ).ldelim();
    }
    if (message.MusicPlayAnaphoraSemanticFrame !== undefined) {
      TMusicPlayAnaphoraSemanticFrame.encode(
        message.MusicPlayAnaphoraSemanticFrame,
        writer.uint32(1290).fork()
      ).ldelim();
    }
    if (message.MusicPlayFairytaleSemanticFrame !== undefined) {
      TMusicPlayFairytaleSemanticFrame.encode(
        message.MusicPlayFairytaleSemanticFrame,
        writer.uint32(1298).fork()
      ).ldelim();
    }
    if (message.StartMultiroomSemanticFrame !== undefined) {
      TStartMultiroomSemanticFrame.encode(
        message.StartMultiroomSemanticFrame,
        writer.uint32(1306).fork()
      ).ldelim();
    }
    if (message.PlayerWhatIsThisSongAboutSemanticFrame !== undefined) {
      TPlayerWhatIsThisSongAboutSemanticFrame.encode(
        message.PlayerWhatIsThisSongAboutSemanticFrame,
        writer.uint32(1314).fork()
      ).ldelim();
    }
    if (message.GuestEnrollmentFinishSemanticFrame !== undefined) {
      TGuestEnrollmentFinishSemanticFrame.encode(
        message.GuestEnrollmentFinishSemanticFrame,
        writer.uint32(1322).fork()
      ).ldelim();
    }
    if (message.CentaurSetTeaserConfigurationSemanticFrame !== undefined) {
      TCentaurSetTeaserConfigurationSemanticFrame.encode(
        message.CentaurSetTeaserConfigurationSemanticFrame,
        writer.uint32(1330).fork()
      ).ldelim();
    }
    if (message.CentaurCollectTeasersPreviewSemanticFrame !== undefined) {
      TCentaurCollectTeasersPreviewSemanticFrame.encode(
        message.CentaurCollectTeasersPreviewSemanticFrame,
        writer.uint32(1338).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTypedSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTypedSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SearchSemanticFrame = TSearchSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.IoTBroadcastStartSemanticFrame =
            TIoTBroadcastStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 3:
          message.IoTBroadcastSuccessSemanticFrame =
            TIoTBroadcastSuccessSemanticFrame.decode(reader, reader.uint32());
          break;
        case 4:
          message.IoTBroadcastFailureSemanticFrame =
            TIoTBroadcastFailureSemanticFrame.decode(reader, reader.uint32());
          break;
        case 5:
          message.MordoviaHomeScreenSemanticFrame =
            TMordoviaHomeScreenSemanticFrame.decode(reader, reader.uint32());
          break;
        case 6:
          message.NewsSemanticFrame = TNewsSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 7:
          message.GetCallerNameSemanticFrame =
            TGetCallerNameSemanticFrame.decode(reader, reader.uint32());
          break;
        case 8:
          message.MusicPlaySemanticFrame = TMusicPlaySemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 9:
          message.ExternalSkillActivateSemanticFrame =
            TExternalSkillActivateSemanticFrame.decode(reader, reader.uint32());
          break;
        case 10:
          message.VideoPlaySemanticFrame = TVideoPlaySemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 11:
          message.WeatherSemanticFrame = TWeatherSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 12:
          message.HardcodedMorningShowSemanticFrame =
            THardcodedMorningShowSemanticFrame.decode(reader, reader.uint32());
          break;
        case 13:
          message.SelectVideoFromGallerySemanticFrame =
            TSelectVideoFromGallerySemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 14:
          message.OpenCurrentVideoSemanticFrame =
            TOpenCurrentVideoSemanticFrame.decode(reader, reader.uint32());
          break;
        case 15:
          message.VideoPaymentConfirmedSemanticFrame =
            TVideoPaymentConfirmedSemanticFrame.decode(reader, reader.uint32());
          break;
        case 16:
          message.PlayerNextTrackSemanticFrame =
            TPlayerNextTrackSemanticFrame.decode(reader, reader.uint32());
          break;
        case 17:
          message.PlayerPrevTrackSemanticFrame =
            TPlayerPrevTrackSemanticFrame.decode(reader, reader.uint32());
          break;
        case 18:
          message.PlayerLikeSemanticFrame = TPlayerLikeSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 19:
          message.PlayerDislikeSemanticFrame =
            TPlayerDislikeSemanticFrame.decode(reader, reader.uint32());
          break;
        case 20:
          message.DoNothingSemanticFrame = TDoNothingSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 21:
          message.NotificationsSubscribeSemanticFrame =
            TNotificationsSubscribeSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 22:
          message.VideoRaterSemanticFrame = TVideoRaterSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 23:
          message.SetupRcuStatusSemanticFrame =
            TSetupRcuStatusSemanticFrame.decode(reader, reader.uint32());
          break;
        case 24:
          message.SetupRcuAutoStatusSemanticFrame =
            TSetupRcuAutoStatusSemanticFrame.decode(reader, reader.uint32());
          break;
        case 25:
          message.SetupRcuCheckStatusSemanticFrame =
            TSetupRcuCheckStatusSemanticFrame.decode(reader, reader.uint32());
          break;
        case 26:
          message.SetupRcuAdvancedStatusSemanticFrame =
            TSetupRcuAdvancedStatusSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 27:
          message.SetupRcuManualStartSemanticFrame =
            TSetupRcuManualStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 28:
          message.SetupRcuAutoStartSemanticFrame =
            TSetupRcuAutoStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 29:
          message.LinkARemoteSemanticFrame = TLinkARemoteSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 30:
          message.RequestTechnicalSupportSemanticFrame =
            TRequestTechnicalSupportSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 31:
          message.IoTDiscoveryStartSemanticFrame =
            TIoTDiscoveryStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 32:
          message.IoTDiscoverySuccessSemanticFrame =
            TIoTDiscoverySuccessSemanticFrame.decode(reader, reader.uint32());
          break;
        case 33:
          message.IoTDiscoveryFailureSemanticFrame =
            TIoTDiscoveryFailureSemanticFrame.decode(reader, reader.uint32());
          break;
        case 34:
          message.ExternalSkillFixedActivateSemanticFrame =
            TExternalSkillFixedActivateSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 35:
          message.OpenCurrentTrailerSemanticFrame =
            TOpenCurrentTrailerSemanticFrame.decode(reader, reader.uint32());
          break;
        case 36:
          message.OnboardingStartingCriticalUpdateSemanticFrame =
            TOnboardingStartingCriticalUpdateSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 37:
          message.OnboardingStartingConfigureSuccessSemanticFrame =
            TOnboardingStartingConfigureSuccessSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 38:
          message.RadioPlaySemanticFrame = TRadioPlaySemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 39:
          message.CentaurCollectCardsSemanticFrame =
            TCentaurCollectCardsSemanticFrame.decode(reader, reader.uint32());
          break;
        case 40:
          message.CentaurGetCardSemanticFrame =
            TCentaurGetCardSemanticFrame.decode(reader, reader.uint32());
          break;
        case 41:
          message.VideoPlayerFinishedSemanticFrame =
            TVideoPlayerFinishedSemanticFrame.decode(reader, reader.uint32());
          break;
        case 42:
          message.SoundLouderSemanticFrame = TSoundLouderSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 43:
          message.SoundQuiterSemanticFrame = TSoundQuiterSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 44:
          message.SoundSetLevelSemanticFrame =
            TSoundSetLevelSemanticFrame.decode(reader, reader.uint32());
          break;
        case 45:
          message.GetPhotoFrameSemanticFrame =
            TGetPhotoFrameSemanticFrame.decode(reader, reader.uint32());
          break;
        case 46:
          message.CentaurCollectMainScreenSemanticFrame =
            TCentaurCollectMainScreenSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 47:
          message.GetTimeSemanticFrame = TGetTimeSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 48:
          message.HowToSubscribeSemanticFrame =
            THowToSubscribeSemanticFrame.decode(reader, reader.uint32());
          break;
        case 49:
          message.MusicOnboardingSemanticFrame =
            TMusicOnboardingSemanticFrame.decode(reader, reader.uint32());
          break;
        case 50:
          message.MusicOnboardingArtistsSemanticFrame =
            TMusicOnboardingArtistsSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 51:
          message.MusicOnboardingGenresSemanticFrame =
            TMusicOnboardingGenresSemanticFrame.decode(reader, reader.uint32());
          break;
        case 52:
          message.MusicOnboardingTracksSemanticFrame =
            TMusicOnboardingTracksSemanticFrame.decode(reader, reader.uint32());
          break;
        case 53:
          message.PlayerContinueSemanticFrame =
            TPlayerContinueSemanticFrame.decode(reader, reader.uint32());
          break;
        case 54:
          message.PlayerWhatIsPlayingSemanticFrame =
            TPlayerWhatIsPlayingSemanticFrame.decode(reader, reader.uint32());
          break;
        case 55:
          message.PlayerShuffleSemanticFrame =
            TPlayerShuffleSemanticFrame.decode(reader, reader.uint32());
          break;
        case 56:
          message.PlayerReplaySemanticFrame = TPlayerReplaySemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 57:
          message.PlayerRewindSemanticFrame = TPlayerRewindSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 58:
          message.PlayerRepeatSemanticFrame = TPlayerRepeatSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 59:
          message.GetSmartTvCategoriesSemanticFrame =
            TGetSmartTvCategoriesSemanticFrame.decode(reader, reader.uint32());
          break;
        case 60:
          message.IoTScenariosPhraseActionSemanticFrame =
            TIoTScenariosPhraseActionSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 61:
          message.IoTScenariosTextActionSemanticFrame =
            TIoTScenariosTextActionSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 62:
          message.IoTScenariosLaunchActionSemanticFrame =
            TIoTScenariosLaunchActionSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 63:
          message.AlarmSetAliceShowSemanticFrame =
            TAlarmSetAliceShowSemanticFrame.decode(reader, reader.uint32());
          break;
        case 64:
          message.PlayerUnshuffleSemanticFrame =
            TPlayerUnshuffleSemanticFrame.decode(reader, reader.uint32());
          break;
        case 66:
          message.RemindersOnShootSemanticFrame =
            TRemindersOnShootSemanticFrame.decode(reader, reader.uint32());
          break;
        case 67:
          message.RepeatAfterMeSemanticFrame =
            TRepeatAfterMeSemanticFrame.decode(reader, reader.uint32());
          break;
        case 68:
          message.MediaPlaySemanticFrame = TMediaPlaySemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 69:
          message.AliceShowActivateSemanticFrame =
            TAliceShowActivateSemanticFrame.decode(reader, reader.uint32());
          break;
        case 70:
          message.ZenContextSearchStartSemanticFrame =
            TZenContextSearchStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 71:
          message.GetSmartTvCarouselSemanticFrame =
            TGetSmartTvCarouselSemanticFrame.decode(reader, reader.uint32());
          break;
        case 72:
          message.GetSmartTvCarouselsSemanticFrame =
            TGetSmartTvCarouselsSemanticFrame.decode(reader, reader.uint32());
          break;
        case 73:
          message.AppsFixlistSemanticFrame = TAppsFixlistSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 74:
          message.PlayerPauseSemanticFrame = TPlayerPauseSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 75:
          message.IoTScenarioSpeakerActionSemanticFrame =
            TIoTScenarioSpeakerActionSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 76:
          message.VideoCardDetailSemanticFrame =
            TVideoCardDetailSemanticFrame.decode(reader, reader.uint32());
          break;
        case 77:
          message.TurnClockFaceOnSemanticFrame =
            TTurnClockFaceOnSemanticFrame.decode(reader, reader.uint32());
          break;
        case 78:
          message.TurnClockFaceOffSemanticFrame =
            TTurnClockFaceOffSemanticFrame.decode(reader, reader.uint32());
          break;
        case 79:
          message.RemindersOnCancelSemanticFrame =
            TRemindersCancelSemanticFrame.decode(reader, reader.uint32());
          break;
        case 80:
          message.IoTDeviceActionSemanticFrame =
            TIoTDeviceActionSemanticFrame.decode(reader, reader.uint32());
          break;
        case 81:
          message.VideoThinCardDetailSmanticFrame =
            TVideoThinCardDetailSemanticFrame.decode(reader, reader.uint32());
          break;
        case 82:
          message.WhisperSaySomethingSemanticFrame =
            TWhisperSaySomethingSemanticFrame.decode(reader, reader.uint32());
          break;
        case 83:
          message.WhisperTurnOffSemanticFrame =
            TWhisperTurnOffSemanticFrame.decode(reader, reader.uint32());
          break;
        case 84:
          message.WhisperTurnOnSemanticFrame =
            TWhisperTurnOnSemanticFrame.decode(reader, reader.uint32());
          break;
        case 85:
          message.WhisperWhatIsItSemanticFrame =
            TWhisperWhatIsItSemanticFrame.decode(reader, reader.uint32());
          break;
        case 86:
          message.TimeCapsuleNextStepSemanticFrame =
            TTimeCapsuleNextStepSemanticFrame.decode(reader, reader.uint32());
          break;
        case 87:
          message.TimeCapsuleStopSemanticFrame =
            TTimeCapsuleStopSemanticFrame.decode(reader, reader.uint32());
          break;
        case 88:
          message.ActivateGenerativeTaleSemanticFrame =
            TActivateGenerativeTaleSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 89:
          message.StartIotDiscoverySemanticFrame =
            TStartIotDiscoverySemanticFrame.decode(reader, reader.uint32());
          break;
        case 90:
          message.FinishIotDiscoverySemanticFrame =
            TFinishIotDiscoverySemanticFrame.decode(reader, reader.uint32());
          break;
        case 91:
          message.ForgetIotEndpointsSemanticFrame =
            TForgetIotEndpointsSemanticFrame.decode(reader, reader.uint32());
          break;
        case 92:
          message.HardcodedResponseSemanticFrame =
            THardcodedResponseSemanticFrame.decode(reader, reader.uint32());
          break;
        case 93:
          message.IotYandexIOActionSemanticFrame =
            TIotYandexIOActionSemanticFrame.decode(reader, reader.uint32());
          break;
        case 94:
          message.TimeCapsuleStartSemanticFrame =
            TTimeCapsuleStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 95:
          message.TimeCapsuleResumeSemanticFrame =
            TTimeCapsuleResumeSemanticFrame.decode(reader, reader.uint32());
          break;
        case 96:
          message.AddAccountSemanticFrame = TAddAccountSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 97:
          message.RemoveAccountSemanticFrame =
            TRemoveAccountSemanticFrame.decode(reader, reader.uint32());
          break;
        case 98:
          message.EndpointStateUpdatesSemanticFrame =
            TEndpointStateUpdatesSemanticFrame.decode(reader, reader.uint32());
          break;
        case 100:
          message.TimeCapsuleSkipQuestionSemanticFrame =
            TTimeCapsuleSkipQuestionSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 101:
          message.SkillSessionRequestSemanticFrame =
            TSkillSessionRequestSemanticFrame.decode(reader, reader.uint32());
          break;
        case 102:
          message.StartIotTuyaBroadcastSemanticFrame =
            TStartIotTuyaBroadcastSemanticFrame.decode(reader, reader.uint32());
          break;
        case 103:
          message.OpenSmartDeviceExternalAppFrame =
            TOpenSmartDeviceExternalAppFrame.decode(reader, reader.uint32());
          break;
        case 104:
          message.RestoreIotNetworksSemanticFrame =
            TRestoreIotNetworksSemanticFrame.decode(reader, reader.uint32());
          break;
        case 105:
          message.SaveIotNetworksSemanticFrame =
            TSaveIotNetworksSemanticFrame.decode(reader, reader.uint32());
          break;
        case 106:
          message.GuestEnrollmentStartSemanticFrame =
            TGuestEnrollmentStartSemanticFrame.decode(reader, reader.uint32());
          break;
        case 107:
          message.TvPromoTemplateRequestSemanticFrame =
            TTvPromoTemplateRequestSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 108:
          message.TvPromoTemplateShownReportSemanticFrame =
            TTvPromoTemplateShownReportSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 109:
          message.UploadContactsRequestSemanticFrame =
            TUploadContactsRequestSemanticFrame.decode(reader, reader.uint32());
          break;
        case 110:
          message.DeleteIotNetworksSemanticFrame =
            TDeleteIotNetworksSemanticFrame.decode(reader, reader.uint32());
          break;
        case 111:
          message.CentaurCollectWidgetGallerySemanticFrame =
            TCentaurCollectWidgetGallerySemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 112:
          message.CentaurAddWidgetFromGallerySemanticFrame =
            TCentaurAddWidgetFromGallerySemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 113:
          message.UpdateContactsRequestSemanticFrame =
            TUpdateContactsRequestSemanticFrame.decode(reader, reader.uint32());
          break;
        case 114:
          message.RemindersListSemanticFrame =
            TRemindersListSemanticFrame.decode(reader, reader.uint32());
          break;
        case 115:
          message.CapabilityEventSemanticFrame =
            TCapabilityEventSemanticFrame.decode(reader, reader.uint32());
          break;
        case 116:
          message.ExternalSkillForceDeactivateSemanticFrame =
            TExternalSkillForceDeactivateSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 117:
          message.GetVideoGalleries = TGetVideoGalleriesSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 118:
          message.EndpointCapabilityEventsSemanticFrame =
            TEndpointCapabilityEventsSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 119:
          message.MusicAnnounceDisableSemanticFrame =
            TMusicAnnounceDisableSemanticFrame.decode(reader, reader.uint32());
          break;
        case 120:
          message.MusicAnnounceEnableSemanticFrame =
            TMusicAnnounceEnableSemanticFrame.decode(reader, reader.uint32());
          break;
        case 121:
          message.GetTvSearchResult = TGetTvSearchResultSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 122:
          message.SwitchTvChannelSemanticFrame =
            TSwitchTvChannelSemanticFrame.decode(reader, reader.uint32());
          break;
        case 123:
          message.ConvertSemanticFrame = TConvertSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 124:
          message.GetVideoGallerySemanticFrame =
            TGetVideoGallerySemanticFrame.decode(reader, reader.uint32());
          break;
        case 125:
          message.MusicOnboardingTracksReaskSemanticFrame =
            TMusicOnboardingTracksReaskSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 126:
          message.TestSemanticFrame = TTestSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 127:
          message.EndpointEventsBatchSemanticFrame =
            TEndpointEventsBatchSemanticFrame.decode(reader, reader.uint32());
          break;
        case 128:
          message.MediaSessionPlaySemanticFrame =
            TMediaSessionPlaySemanticFrame.decode(reader, reader.uint32());
          break;
        case 129:
          message.MediaSessionPauseSemanticFrame =
            TMediaSessionPauseSemanticFrame.decode(reader, reader.uint32());
          break;
        case 130:
          message.FmRadioPlaySemanticFrame = TFmRadioPlaySemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 131:
          message.VideoCallLoginFailedSemanticFrame =
            TVideoCallLoginFailedSemanticFrame.decode(reader, reader.uint32());
          break;
        case 132:
          message.VideoCallOutgoingAcceptedSemanticFrame =
            TVideoCallOutgoingAcceptedSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 133:
          message.VideoCallOutgoingFailedSemanticFrame =
            TVideoCallOutgoingFailedSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 134:
          message.VideoCallIncomingAcceptFailedSemanticFrame =
            TVideoCallIncomingAcceptFailedSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 135:
          message.IotScenarioStepActionsSemanticFrame =
            TIotScenarioStepActionsSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 136:
          message.PhoneCallSemanticFrame = TPhoneCallSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 137:
          message.OpenAddressBookSemanticFrame =
            TOpenAddressBookSemanticFrame.decode(reader, reader.uint32());
          break;
        case 138:
          message.GetEqualizerSettingsSemanticFrame =
            TGetEqualizerSettingsSemanticFrame.decode(reader, reader.uint32());
          break;
        case 139:
          message.VideoCallToSemanticFrame = TVideoCallToSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 140:
          message.OnboardingGetGreetingsSemanticFrame =
            TOnboardingGetGreetingsSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 141:
          message.OpenTandemSettingSemanticFrame =
            TOpenTandemSettingSemanticFrame.decode(reader, reader.uint32());
          break;
        case 142:
          message.OpenSmartSpeakerSettingSemanticFrame =
            TOpenSmartSpeakerSettingSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 143:
          message.FinishIotSystemDiscoverySemanticFrame =
            TFinishIotSystemDiscoverySemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 144:
          message.VideoCallSetFavoritesSemanticFrame =
            TVideoCallSetFavoritesSemanticFrame.decode(reader, reader.uint32());
          break;
        case 145:
          message.VideoCallIncomingSemanticFrame =
            TVideoCallIncomingSemanticFrame.decode(reader, reader.uint32());
          break;
        case 146:
          message.MessengerCallAcceptSemanticFrame =
            TMessengerCallAcceptSemanticFrame.decode(reader, reader.uint32());
          break;
        case 147:
          message.MessengerCallDiscardSemanticFrame =
            TMessengerCallDiscardSemanticFrame.decode(reader, reader.uint32());
          break;
        case 148:
          message.TvLongTapTutorialSemanticFrame =
            TTvLongTapTutorialSemanticFrame.decode(reader, reader.uint32());
          break;
        case 149:
          message.OnboardingWhatCanYouDoSemanticFrame =
            TOnboardingWhatCanYouDoSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 150:
          message.MessengerCallHangupSemanticFrame =
            TMessengerCallHangupSemanticFrame.decode(reader, reader.uint32());
          break;
        case 151:
          message.PutMoneyOnPhoneSemanticFrame =
            TPutMoneyOnPhoneSemanticFrame.decode(reader, reader.uint32());
          break;
        case 152:
          message.OrderNotificationSemanticFrame =
            TOrderNotificationSemanticFrame.decode(reader, reader.uint32());
          break;
        case 153:
          message.PlayerRemoveLikeSemanticFrame =
            TPlayerRemoveLikeSemanticFrame.decode(reader, reader.uint32());
          break;
        case 154:
          message.PlayerRemoveDislikeSemanticFrame =
            TPlayerRemoveDislikeSemanticFrame.decode(reader, reader.uint32());
          break;
        case 155:
          message.CentaurCollectUpperShutterSemanticFrame =
            TCentaurCollectUpperShutterSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 156:
          message.EnrollmentStatusSemanticFrame =
            TEnrollmentStatusSemanticFrame.decode(reader, reader.uint32());
          break;
        case 157:
          message.GalleryVideoSelectSemanticFrame =
            TGalleryVideoSelectSemanticFrame.decode(reader, reader.uint32());
          break;
        case 158:
          message.PlayerSongsByThisArtistSemanticFrame =
            TPlayerSongsByThisArtistSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 159:
          message.ExternalSkillEpisodeForShowRequestSemanticFrame =
            TExternalSkillEpisodeForShowRequestSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 160:
          message.MusicPlayFixlistSemanticFrame =
            TMusicPlayFixlistSemanticFrame.decode(reader, reader.uint32());
          break;
        case 161:
          message.MusicPlayAnaphoraSemanticFrame =
            TMusicPlayAnaphoraSemanticFrame.decode(reader, reader.uint32());
          break;
        case 162:
          message.MusicPlayFairytaleSemanticFrame =
            TMusicPlayFairytaleSemanticFrame.decode(reader, reader.uint32());
          break;
        case 163:
          message.StartMultiroomSemanticFrame =
            TStartMultiroomSemanticFrame.decode(reader, reader.uint32());
          break;
        case 164:
          message.PlayerWhatIsThisSongAboutSemanticFrame =
            TPlayerWhatIsThisSongAboutSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 165:
          message.GuestEnrollmentFinishSemanticFrame =
            TGuestEnrollmentFinishSemanticFrame.decode(reader, reader.uint32());
          break;
        case 166:
          message.CentaurSetTeaserConfigurationSemanticFrame =
            TCentaurSetTeaserConfigurationSemanticFrame.decode(
              reader,
              reader.uint32()
            );
          break;
        case 167:
          message.CentaurCollectTeasersPreviewSemanticFrame =
            TCentaurCollectTeasersPreviewSemanticFrame.decode(
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

  fromJSON(object: any): TTypedSemanticFrame {
    return {
      SearchSemanticFrame: isSet(object.search_semantic_frame)
        ? TSearchSemanticFrame.fromJSON(object.search_semantic_frame)
        : undefined,
      IoTBroadcastStartSemanticFrame: isSet(object.iot_broadcast_start)
        ? TIoTBroadcastStartSemanticFrame.fromJSON(object.iot_broadcast_start)
        : undefined,
      IoTBroadcastSuccessSemanticFrame: isSet(object.iot_broadcast_success)
        ? TIoTBroadcastSuccessSemanticFrame.fromJSON(
            object.iot_broadcast_success
          )
        : undefined,
      IoTBroadcastFailureSemanticFrame: isSet(object.iot_broadcast_failure)
        ? TIoTBroadcastFailureSemanticFrame.fromJSON(
            object.iot_broadcast_failure
          )
        : undefined,
      MordoviaHomeScreenSemanticFrame: isSet(object.mordovia_home_screen)
        ? TMordoviaHomeScreenSemanticFrame.fromJSON(object.mordovia_home_screen)
        : undefined,
      NewsSemanticFrame: isSet(object.news_semantic_frame)
        ? TNewsSemanticFrame.fromJSON(object.news_semantic_frame)
        : undefined,
      GetCallerNameSemanticFrame: isSet(object.get_caller_name)
        ? TGetCallerNameSemanticFrame.fromJSON(object.get_caller_name)
        : undefined,
      MusicPlaySemanticFrame: isSet(object.music_play_semantic_frame)
        ? TMusicPlaySemanticFrame.fromJSON(object.music_play_semantic_frame)
        : undefined,
      ExternalSkillActivateSemanticFrame: isSet(
        object.external_skill_activate_semantic_frame
      )
        ? TExternalSkillActivateSemanticFrame.fromJSON(
            object.external_skill_activate_semantic_frame
          )
        : undefined,
      VideoPlaySemanticFrame: isSet(object.video_play_semantic_frame)
        ? TVideoPlaySemanticFrame.fromJSON(object.video_play_semantic_frame)
        : undefined,
      WeatherSemanticFrame: isSet(object.weather_semantic_frame)
        ? TWeatherSemanticFrame.fromJSON(object.weather_semantic_frame)
        : undefined,
      HardcodedMorningShowSemanticFrame: isSet(
        object.morning_show_semantic_frame
      )
        ? THardcodedMorningShowSemanticFrame.fromJSON(
            object.morning_show_semantic_frame
          )
        : undefined,
      SelectVideoFromGallerySemanticFrame: isSet(object.select_video)
        ? TSelectVideoFromGallerySemanticFrame.fromJSON(object.select_video)
        : undefined,
      OpenCurrentVideoSemanticFrame: isSet(object.open_current_video)
        ? TOpenCurrentVideoSemanticFrame.fromJSON(object.open_current_video)
        : undefined,
      VideoPaymentConfirmedSemanticFrame: isSet(object.video_payment_confirmed)
        ? TVideoPaymentConfirmedSemanticFrame.fromJSON(
            object.video_payment_confirmed
          )
        : undefined,
      PlayerNextTrackSemanticFrame: isSet(
        object.player_next_track_semantic_frame
      )
        ? TPlayerNextTrackSemanticFrame.fromJSON(
            object.player_next_track_semantic_frame
          )
        : undefined,
      PlayerPrevTrackSemanticFrame: isSet(
        object.player_prev_track_semantic_frame
      )
        ? TPlayerPrevTrackSemanticFrame.fromJSON(
            object.player_prev_track_semantic_frame
          )
        : undefined,
      PlayerLikeSemanticFrame: isSet(object.player_like_semantic_frame)
        ? TPlayerLikeSemanticFrame.fromJSON(object.player_like_semantic_frame)
        : undefined,
      PlayerDislikeSemanticFrame: isSet(object.player_dislike_semantic_frame)
        ? TPlayerDislikeSemanticFrame.fromJSON(
            object.player_dislike_semantic_frame
          )
        : undefined,
      DoNothingSemanticFrame: isSet(object.do_nothing_semantic_frame)
        ? TDoNothingSemanticFrame.fromJSON(object.do_nothing_semantic_frame)
        : undefined,
      NotificationsSubscribeSemanticFrame: isSet(
        object.notifications_subscribe_semantic_frame
      )
        ? TNotificationsSubscribeSemanticFrame.fromJSON(
            object.notifications_subscribe_semantic_frame
          )
        : undefined,
      VideoRaterSemanticFrame: isSet(object.video_rater_semantic_frame)
        ? TVideoRaterSemanticFrame.fromJSON(object.video_rater_semantic_frame)
        : undefined,
      SetupRcuStatusSemanticFrame: isSet(object["setup_rcu.status"])
        ? TSetupRcuStatusSemanticFrame.fromJSON(object["setup_rcu.status"])
        : undefined,
      SetupRcuAutoStatusSemanticFrame: isSet(object["setup_rcu_auto.status"])
        ? TSetupRcuAutoStatusSemanticFrame.fromJSON(
            object["setup_rcu_auto.status"]
          )
        : undefined,
      SetupRcuCheckStatusSemanticFrame: isSet(object["setup_rcu_check.status"])
        ? TSetupRcuCheckStatusSemanticFrame.fromJSON(
            object["setup_rcu_check.status"]
          )
        : undefined,
      SetupRcuAdvancedStatusSemanticFrame: isSet(
        object["setup_rcu_advanced.status"]
      )
        ? TSetupRcuAdvancedStatusSemanticFrame.fromJSON(
            object["setup_rcu_advanced.status"]
          )
        : undefined,
      SetupRcuManualStartSemanticFrame: isSet(object["setup_rcu_manual.start"])
        ? TSetupRcuManualStartSemanticFrame.fromJSON(
            object["setup_rcu_manual.start"]
          )
        : undefined,
      SetupRcuAutoStartSemanticFrame: isSet(object["setup_rcu_auto.start"])
        ? TSetupRcuAutoStartSemanticFrame.fromJSON(
            object["setup_rcu_auto.start"]
          )
        : undefined,
      LinkARemoteSemanticFrame: isSet(object.link_a_remote)
        ? TLinkARemoteSemanticFrame.fromJSON(object.link_a_remote)
        : undefined,
      RequestTechnicalSupportSemanticFrame: isSet(
        object.request_technical_support
      )
        ? TRequestTechnicalSupportSemanticFrame.fromJSON(
            object.request_technical_support
          )
        : undefined,
      IoTDiscoveryStartSemanticFrame: isSet(object.iot_discovery_start)
        ? TIoTDiscoveryStartSemanticFrame.fromJSON(object.iot_discovery_start)
        : undefined,
      IoTDiscoverySuccessSemanticFrame: isSet(object.iot_discovery_success)
        ? TIoTDiscoverySuccessSemanticFrame.fromJSON(
            object.iot_discovery_success
          )
        : undefined,
      IoTDiscoveryFailureSemanticFrame: isSet(object.iot_discovery_failure)
        ? TIoTDiscoveryFailureSemanticFrame.fromJSON(
            object.iot_discovery_failure
          )
        : undefined,
      ExternalSkillFixedActivateSemanticFrame: isSet(
        object.external_skill_fixed_activate_semantic_frame
      )
        ? TExternalSkillFixedActivateSemanticFrame.fromJSON(
            object.external_skill_fixed_activate_semantic_frame
          )
        : undefined,
      OpenCurrentTrailerSemanticFrame: isSet(object.open_current_trailer)
        ? TOpenCurrentTrailerSemanticFrame.fromJSON(object.open_current_trailer)
        : undefined,
      OnboardingStartingCriticalUpdateSemanticFrame: isSet(
        object.onboarding_starting_critical_update
      )
        ? TOnboardingStartingCriticalUpdateSemanticFrame.fromJSON(
            object.onboarding_starting_critical_update
          )
        : undefined,
      OnboardingStartingConfigureSuccessSemanticFrame: isSet(
        object.onboarding_starting_configure_success
      )
        ? TOnboardingStartingConfigureSuccessSemanticFrame.fromJSON(
            object.onboarding_starting_configure_success
          )
        : undefined,
      RadioPlaySemanticFrame: isSet(object.radio_play_semantic_frame)
        ? TRadioPlaySemanticFrame.fromJSON(object.radio_play_semantic_frame)
        : undefined,
      CentaurCollectCardsSemanticFrame: isSet(object.centaur_collect_cards)
        ? TCentaurCollectCardsSemanticFrame.fromJSON(
            object.centaur_collect_cards
          )
        : undefined,
      CentaurGetCardSemanticFrame: isSet(object.centaur_get_card)
        ? TCentaurGetCardSemanticFrame.fromJSON(object.centaur_get_card)
        : undefined,
      VideoPlayerFinishedSemanticFrame: isSet(
        object["quasar.video_player.finished"]
      )
        ? TVideoPlayerFinishedSemanticFrame.fromJSON(
            object["quasar.video_player.finished"]
          )
        : undefined,
      SoundLouderSemanticFrame: isSet(object.sound_louder_semantic_frame)
        ? TSoundLouderSemanticFrame.fromJSON(object.sound_louder_semantic_frame)
        : undefined,
      SoundQuiterSemanticFrame: isSet(object.sound_quiter_semantic_frame)
        ? TSoundQuiterSemanticFrame.fromJSON(object.sound_quiter_semantic_frame)
        : undefined,
      SoundSetLevelSemanticFrame: isSet(object.sound_set_level_semantic_frame)
        ? TSoundSetLevelSemanticFrame.fromJSON(
            object.sound_set_level_semantic_frame
          )
        : undefined,
      GetPhotoFrameSemanticFrame: isSet(object.get_photo_frame)
        ? TGetPhotoFrameSemanticFrame.fromJSON(object.get_photo_frame)
        : undefined,
      CentaurCollectMainScreenSemanticFrame: isSet(
        object.centaur_collect_main_screen
      )
        ? TCentaurCollectMainScreenSemanticFrame.fromJSON(
            object.centaur_collect_main_screen
          )
        : undefined,
      GetTimeSemanticFrame: isSet(object.get_time_semantic_frame)
        ? TGetTimeSemanticFrame.fromJSON(object.get_time_semantic_frame)
        : undefined,
      HowToSubscribeSemanticFrame: isSet(object.how_to_subscribe_semantic_frame)
        ? THowToSubscribeSemanticFrame.fromJSON(
            object.how_to_subscribe_semantic_frame
          )
        : undefined,
      MusicOnboardingSemanticFrame: isSet(
        object.music_onboarding_semantic_frame
      )
        ? TMusicOnboardingSemanticFrame.fromJSON(
            object.music_onboarding_semantic_frame
          )
        : undefined,
      MusicOnboardingArtistsSemanticFrame: isSet(
        object.music_onboarding_artists_semantic_frame
      )
        ? TMusicOnboardingArtistsSemanticFrame.fromJSON(
            object.music_onboarding_artists_semantic_frame
          )
        : undefined,
      MusicOnboardingGenresSemanticFrame: isSet(
        object.music_onboarding_genres_semantic_frame
      )
        ? TMusicOnboardingGenresSemanticFrame.fromJSON(
            object.music_onboarding_genres_semantic_frame
          )
        : undefined,
      MusicOnboardingTracksSemanticFrame: isSet(
        object.music_onboarding_tracks_semantic_frame
      )
        ? TMusicOnboardingTracksSemanticFrame.fromJSON(
            object.music_onboarding_tracks_semantic_frame
          )
        : undefined,
      PlayerContinueSemanticFrame: isSet(object.player_continue_semantic_frame)
        ? TPlayerContinueSemanticFrame.fromJSON(
            object.player_continue_semantic_frame
          )
        : undefined,
      PlayerWhatIsPlayingSemanticFrame: isSet(
        object.player_what_is_playing_semantic_frame
      )
        ? TPlayerWhatIsPlayingSemanticFrame.fromJSON(
            object.player_what_is_playing_semantic_frame
          )
        : undefined,
      PlayerShuffleSemanticFrame: isSet(object.player_shuffle_semantic_frame)
        ? TPlayerShuffleSemanticFrame.fromJSON(
            object.player_shuffle_semantic_frame
          )
        : undefined,
      PlayerReplaySemanticFrame: isSet(object.player_replay_semantic_frame)
        ? TPlayerReplaySemanticFrame.fromJSON(
            object.player_replay_semantic_frame
          )
        : undefined,
      PlayerRewindSemanticFrame: isSet(object.player_rewind_semantic_frame)
        ? TPlayerRewindSemanticFrame.fromJSON(
            object.player_rewind_semantic_frame
          )
        : undefined,
      PlayerRepeatSemanticFrame: isSet(object.player_repeat_semantic_frame)
        ? TPlayerRepeatSemanticFrame.fromJSON(
            object.player_repeat_semantic_frame
          )
        : undefined,
      GetSmartTvCategoriesSemanticFrame: isSet(
        object.get_smart_tv_categories_semantic_frame
      )
        ? TGetSmartTvCategoriesSemanticFrame.fromJSON(
            object.get_smart_tv_categories_semantic_frame
          )
        : undefined,
      IoTScenariosPhraseActionSemanticFrame: isSet(
        object.iot_scenarios_phrase_action_semantic_frame
      )
        ? TIoTScenariosPhraseActionSemanticFrame.fromJSON(
            object.iot_scenarios_phrase_action_semantic_frame
          )
        : undefined,
      IoTScenariosTextActionSemanticFrame: isSet(
        object.iot_scenarios_text_action_semantic_frame
      )
        ? TIoTScenariosTextActionSemanticFrame.fromJSON(
            object.iot_scenarios_text_action_semantic_frame
          )
        : undefined,
      IoTScenariosLaunchActionSemanticFrame: isSet(
        object.iot_scenarios_launch_action_semantic_frame
      )
        ? TIoTScenariosLaunchActionSemanticFrame.fromJSON(
            object.iot_scenarios_launch_action_semantic_frame
          )
        : undefined,
      AlarmSetAliceShowSemanticFrame: isSet(
        object.alarm_set_alice_show_semantic_frame
      )
        ? TAlarmSetAliceShowSemanticFrame.fromJSON(
            object.alarm_set_alice_show_semantic_frame
          )
        : undefined,
      PlayerUnshuffleSemanticFrame: isSet(
        object.player_unshuffle_semantic_frame
      )
        ? TPlayerUnshuffleSemanticFrame.fromJSON(
            object.player_unshuffle_semantic_frame
          )
        : undefined,
      RemindersOnShootSemanticFrame: isSet(
        object.reminders_on_shoot_semantic_frame
      )
        ? TRemindersOnShootSemanticFrame.fromJSON(
            object.reminders_on_shoot_semantic_frame
          )
        : undefined,
      RepeatAfterMeSemanticFrame: isSet(object.repeat_after_me_semantic_frame)
        ? TRepeatAfterMeSemanticFrame.fromJSON(
            object.repeat_after_me_semantic_frame
          )
        : undefined,
      MediaPlaySemanticFrame: isSet(object.media_play_semantic_frame)
        ? TMediaPlaySemanticFrame.fromJSON(object.media_play_semantic_frame)
        : undefined,
      AliceShowActivateSemanticFrame: isSet(
        object.alice_show_activate_semantic_frame
      )
        ? TAliceShowActivateSemanticFrame.fromJSON(
            object.alice_show_activate_semantic_frame
          )
        : undefined,
      ZenContextSearchStartSemanticFrame: isSet(
        object.zen_context_search_start_semantic_frame
      )
        ? TZenContextSearchStartSemanticFrame.fromJSON(
            object.zen_context_search_start_semantic_frame
          )
        : undefined,
      GetSmartTvCarouselSemanticFrame: isSet(
        object.get_smart_tv_carousel_semantic_frame
      )
        ? TGetSmartTvCarouselSemanticFrame.fromJSON(
            object.get_smart_tv_carousel_semantic_frame
          )
        : undefined,
      GetSmartTvCarouselsSemanticFrame: isSet(
        object.get_smart_tv_carousels_semantic_frame
      )
        ? TGetSmartTvCarouselsSemanticFrame.fromJSON(
            object.get_smart_tv_carousels_semantic_frame
          )
        : undefined,
      AppsFixlistSemanticFrame: isSet(object.apps_fixlist_semantic_frame)
        ? TAppsFixlistSemanticFrame.fromJSON(object.apps_fixlist_semantic_frame)
        : undefined,
      PlayerPauseSemanticFrame: isSet(object.player_pause_semantic_frame)
        ? TPlayerPauseSemanticFrame.fromJSON(object.player_pause_semantic_frame)
        : undefined,
      IoTScenarioSpeakerActionSemanticFrame: isSet(
        object.iot_scenario_speaker_action_semantic_frame
      )
        ? TIoTScenarioSpeakerActionSemanticFrame.fromJSON(
            object.iot_scenario_speaker_action_semantic_frame
          )
        : undefined,
      VideoCardDetailSemanticFrame: isSet(
        object.get_video_card_detail_semantic_frame
      )
        ? TVideoCardDetailSemanticFrame.fromJSON(
            object.get_video_card_detail_semantic_frame
          )
        : undefined,
      TurnClockFaceOnSemanticFrame: isSet(
        object.turn_clock_face_on_semantic_frame
      )
        ? TTurnClockFaceOnSemanticFrame.fromJSON(
            object.turn_clock_face_on_semantic_frame
          )
        : undefined,
      TurnClockFaceOffSemanticFrame: isSet(
        object.turn_clock_face_off_semantic_frame
      )
        ? TTurnClockFaceOffSemanticFrame.fromJSON(
            object.turn_clock_face_off_semantic_frame
          )
        : undefined,
      RemindersOnCancelSemanticFrame: isSet(
        object.reminders_on_cancel_semantic_frame
      )
        ? TRemindersCancelSemanticFrame.fromJSON(
            object.reminders_on_cancel_semantic_frame
          )
        : undefined,
      IoTDeviceActionSemanticFrame: isSet(
        object.iot_device_action_semantic_frame
      )
        ? TIoTDeviceActionSemanticFrame.fromJSON(
            object.iot_device_action_semantic_frame
          )
        : undefined,
      VideoThinCardDetailSmanticFrame: isSet(
        object.get_video_thin_card_detail_semantic_frame
      )
        ? TVideoThinCardDetailSemanticFrame.fromJSON(
            object.get_video_thin_card_detail_semantic_frame
          )
        : undefined,
      WhisperSaySomethingSemanticFrame: isSet(
        object.whisper_say_something_semantic_frame
      )
        ? TWhisperSaySomethingSemanticFrame.fromJSON(
            object.whisper_say_something_semantic_frame
          )
        : undefined,
      WhisperTurnOffSemanticFrame: isSet(object.whisper_turn_off_semantic_frame)
        ? TWhisperTurnOffSemanticFrame.fromJSON(
            object.whisper_turn_off_semantic_frame
          )
        : undefined,
      WhisperTurnOnSemanticFrame: isSet(object.whisper_turn_on_semantic_frame)
        ? TWhisperTurnOnSemanticFrame.fromJSON(
            object.whisper_turn_on_semantic_frame
          )
        : undefined,
      WhisperWhatIsItSemanticFrame: isSet(
        object.whisper_what_is_it_semantic_frame
      )
        ? TWhisperWhatIsItSemanticFrame.fromJSON(
            object.whisper_what_is_it_semantic_frame
          )
        : undefined,
      TimeCapsuleNextStepSemanticFrame: isSet(
        object.time_capsule_next_step_semantic_frame
      )
        ? TTimeCapsuleNextStepSemanticFrame.fromJSON(
            object.time_capsule_next_step_semantic_frame
          )
        : undefined,
      TimeCapsuleStopSemanticFrame: isSet(
        object.time_capsule_stop_semantic_frame
      )
        ? TTimeCapsuleStopSemanticFrame.fromJSON(
            object.time_capsule_stop_semantic_frame
          )
        : undefined,
      ActivateGenerativeTaleSemanticFrame: isSet(
        object.activate_generative_tale_semantic_frame
      )
        ? TActivateGenerativeTaleSemanticFrame.fromJSON(
            object.activate_generative_tale_semantic_frame
          )
        : undefined,
      StartIotDiscoverySemanticFrame: isSet(
        object.start_iot_discovery_semantic_frame
      )
        ? TStartIotDiscoverySemanticFrame.fromJSON(
            object.start_iot_discovery_semantic_frame
          )
        : undefined,
      FinishIotDiscoverySemanticFrame: isSet(
        object.finish_iot_discovery_semantic_frame
      )
        ? TFinishIotDiscoverySemanticFrame.fromJSON(
            object.finish_iot_discovery_semantic_frame
          )
        : undefined,
      ForgetIotEndpointsSemanticFrame: isSet(
        object.forget_iot_endpoints_semantic_frame
      )
        ? TForgetIotEndpointsSemanticFrame.fromJSON(
            object.forget_iot_endpoints_semantic_frame
          )
        : undefined,
      HardcodedResponseSemanticFrame: isSet(
        object.hardcoded_response_semantic_frame
      )
        ? THardcodedResponseSemanticFrame.fromJSON(
            object.hardcoded_response_semantic_frame
          )
        : undefined,
      IotYandexIOActionSemanticFrame: isSet(
        object.iot_yandex_io_action_semantic_frame
      )
        ? TIotYandexIOActionSemanticFrame.fromJSON(
            object.iot_yandex_io_action_semantic_frame
          )
        : undefined,
      TimeCapsuleStartSemanticFrame: isSet(
        object.time_capsule_start_semantic_frame
      )
        ? TTimeCapsuleStartSemanticFrame.fromJSON(
            object.time_capsule_start_semantic_frame
          )
        : undefined,
      TimeCapsuleResumeSemanticFrame: isSet(
        object.time_capsule_resume_semantic_frame
      )
        ? TTimeCapsuleResumeSemanticFrame.fromJSON(
            object.time_capsule_resume_semantic_frame
          )
        : undefined,
      AddAccountSemanticFrame: isSet(
        object.multiaccount_add_account_semantic_frame
      )
        ? TAddAccountSemanticFrame.fromJSON(
            object.multiaccount_add_account_semantic_frame
          )
        : undefined,
      RemoveAccountSemanticFrame: isSet(
        object.multiaccount_remove_account_semantic_frame
      )
        ? TRemoveAccountSemanticFrame.fromJSON(
            object.multiaccount_remove_account_semantic_frame
          )
        : undefined,
      EndpointStateUpdatesSemanticFrame: isSet(
        object.endpoint_state_updates_semantic_frame
      )
        ? TEndpointStateUpdatesSemanticFrame.fromJSON(
            object.endpoint_state_updates_semantic_frame
          )
        : undefined,
      TimeCapsuleSkipQuestionSemanticFrame: isSet(
        object.time_capsule_skip_question_semantic_frame
      )
        ? TTimeCapsuleSkipQuestionSemanticFrame.fromJSON(
            object.time_capsule_skip_question_semantic_frame
          )
        : undefined,
      SkillSessionRequestSemanticFrame: isSet(
        object.skill_session_request_semantic_frame
      )
        ? TSkillSessionRequestSemanticFrame.fromJSON(
            object.skill_session_request_semantic_frame
          )
        : undefined,
      StartIotTuyaBroadcastSemanticFrame: isSet(
        object.start_iot_tuya_broadcast_semantic_frame
      )
        ? TStartIotTuyaBroadcastSemanticFrame.fromJSON(
            object.start_iot_tuya_broadcast_semantic_frame
          )
        : undefined,
      OpenSmartDeviceExternalAppFrame: isSet(
        object.open_smart_device_external_app_frame
      )
        ? TOpenSmartDeviceExternalAppFrame.fromJSON(
            object.open_smart_device_external_app_frame
          )
        : undefined,
      RestoreIotNetworksSemanticFrame: isSet(
        object.restore_iot_networks_semantic_frame
      )
        ? TRestoreIotNetworksSemanticFrame.fromJSON(
            object.restore_iot_networks_semantic_frame
          )
        : undefined,
      SaveIotNetworksSemanticFrame: isSet(
        object.save_iot_networks_semantic_frame
      )
        ? TSaveIotNetworksSemanticFrame.fromJSON(
            object.save_iot_networks_semantic_frame
          )
        : undefined,
      GuestEnrollmentStartSemanticFrame: isSet(
        object.guest_enrollment_start_semantic_frame
      )
        ? TGuestEnrollmentStartSemanticFrame.fromJSON(
            object.guest_enrollment_start_semantic_frame
          )
        : undefined,
      TvPromoTemplateRequestSemanticFrame: isSet(
        object.tv_promo_request_semantic_frame
      )
        ? TTvPromoTemplateRequestSemanticFrame.fromJSON(
            object.tv_promo_request_semantic_frame
          )
        : undefined,
      TvPromoTemplateShownReportSemanticFrame: isSet(
        object.tv_promo_template_shown_report_semantic_frame
      )
        ? TTvPromoTemplateShownReportSemanticFrame.fromJSON(
            object.tv_promo_template_shown_report_semantic_frame
          )
        : undefined,
      UploadContactsRequestSemanticFrame: isSet(
        object.upload_contacts_request_semantic_frame
      )
        ? TUploadContactsRequestSemanticFrame.fromJSON(
            object.upload_contacts_request_semantic_frame
          )
        : undefined,
      DeleteIotNetworksSemanticFrame: isSet(
        object.delete_iot_networks_semantic_frame
      )
        ? TDeleteIotNetworksSemanticFrame.fromJSON(
            object.delete_iot_networks_semantic_frame
          )
        : undefined,
      CentaurCollectWidgetGallerySemanticFrame: isSet(
        object.centaur_collect_widget_gallery_semantic_frame
      )
        ? TCentaurCollectWidgetGallerySemanticFrame.fromJSON(
            object.centaur_collect_widget_gallery_semantic_frame
          )
        : undefined,
      CentaurAddWidgetFromGallerySemanticFrame: isSet(
        object.centaur_add_widget_from_gallery_semantic_frame
      )
        ? TCentaurAddWidgetFromGallerySemanticFrame.fromJSON(
            object.centaur_add_widget_from_gallery_semantic_frame
          )
        : undefined,
      UpdateContactsRequestSemanticFrame: isSet(
        object.update_contacts_request_semantic_frame
      )
        ? TUpdateContactsRequestSemanticFrame.fromJSON(
            object.update_contacts_request_semantic_frame
          )
        : undefined,
      RemindersListSemanticFrame: isSet(object.reminders_list_semantic_frame)
        ? TRemindersListSemanticFrame.fromJSON(
            object.reminders_list_semantic_frame
          )
        : undefined,
      CapabilityEventSemanticFrame: isSet(
        object.capability_event_semantic_frame
      )
        ? TCapabilityEventSemanticFrame.fromJSON(
            object.capability_event_semantic_frame
          )
        : undefined,
      ExternalSkillForceDeactivateSemanticFrame: isSet(
        object.external_skill_force_deactivate_semantic_frame
      )
        ? TExternalSkillForceDeactivateSemanticFrame.fromJSON(
            object.external_skill_force_deactivate_semantic_frame
          )
        : undefined,
      GetVideoGalleries: isSet(object.get_video_galleries_semantic_frame)
        ? TGetVideoGalleriesSemanticFrame.fromJSON(
            object.get_video_galleries_semantic_frame
          )
        : undefined,
      EndpointCapabilityEventsSemanticFrame: isSet(
        object.endpoint_capability_events_semantic_frame
      )
        ? TEndpointCapabilityEventsSemanticFrame.fromJSON(
            object.endpoint_capability_events_semantic_frame
          )
        : undefined,
      MusicAnnounceDisableSemanticFrame: isSet(
        object.music_announce_disable_semantic_frame
      )
        ? TMusicAnnounceDisableSemanticFrame.fromJSON(
            object.music_announce_disable_semantic_frame
          )
        : undefined,
      MusicAnnounceEnableSemanticFrame: isSet(
        object.music_announce_enable_semantic_frame
      )
        ? TMusicAnnounceEnableSemanticFrame.fromJSON(
            object.music_announce_enable_semantic_frame
          )
        : undefined,
      GetTvSearchResult: isSet(object.get_tv_search_result)
        ? TGetTvSearchResultSemanticFrame.fromJSON(object.get_tv_search_result)
        : undefined,
      SwitchTvChannelSemanticFrame: isSet(
        object.switch_tv_channel_semantic_frame
      )
        ? TSwitchTvChannelSemanticFrame.fromJSON(
            object.switch_tv_channel_semantic_frame
          )
        : undefined,
      ConvertSemanticFrame: isSet(object.convert_semantic_frame)
        ? TConvertSemanticFrame.fromJSON(object.convert_semantic_frame)
        : undefined,
      GetVideoGallerySemanticFrame: isSet(
        object.get_video_gallery_semantic_frame
      )
        ? TGetVideoGallerySemanticFrame.fromJSON(
            object.get_video_gallery_semantic_frame
          )
        : undefined,
      MusicOnboardingTracksReaskSemanticFrame: isSet(
        object.music_onboarding_tracks_reask_semantic_frame
      )
        ? TMusicOnboardingTracksReaskSemanticFrame.fromJSON(
            object.music_onboarding_tracks_reask_semantic_frame
          )
        : undefined,
      TestSemanticFrame: isSet(object.test_semantic_frame)
        ? TTestSemanticFrame.fromJSON(object.test_semantic_frame)
        : undefined,
      EndpointEventsBatchSemanticFrame: isSet(
        object.endpoint_events_batch_semantic_frame
      )
        ? TEndpointEventsBatchSemanticFrame.fromJSON(
            object.endpoint_events_batch_semantic_frame
          )
        : undefined,
      MediaSessionPlaySemanticFrame: isSet(
        object.media_session_play_semantic_frame
      )
        ? TMediaSessionPlaySemanticFrame.fromJSON(
            object.media_session_play_semantic_frame
          )
        : undefined,
      MediaSessionPauseSemanticFrame: isSet(
        object.media_session_pause_semantic_frame
      )
        ? TMediaSessionPauseSemanticFrame.fromJSON(
            object.media_session_pause_semantic_frame
          )
        : undefined,
      FmRadioPlaySemanticFrame: isSet(object.fm_radio_play_semantic_frame)
        ? TFmRadioPlaySemanticFrame.fromJSON(
            object.fm_radio_play_semantic_frame
          )
        : undefined,
      VideoCallLoginFailedSemanticFrame: isSet(
        object.video_call_login_failed_semantic_frame
      )
        ? TVideoCallLoginFailedSemanticFrame.fromJSON(
            object.video_call_login_failed_semantic_frame
          )
        : undefined,
      VideoCallOutgoingAcceptedSemanticFrame: isSet(
        object.video_call_outgoing_accepted_semantic_frame
      )
        ? TVideoCallOutgoingAcceptedSemanticFrame.fromJSON(
            object.video_call_outgoing_accepted_semantic_frame
          )
        : undefined,
      VideoCallOutgoingFailedSemanticFrame: isSet(
        object.video_call_outgoing_failed_semantic_frame
      )
        ? TVideoCallOutgoingFailedSemanticFrame.fromJSON(
            object.video_call_outgoing_failed_semantic_frame
          )
        : undefined,
      VideoCallIncomingAcceptFailedSemanticFrame: isSet(
        object.video_call_incoming_accept_failed_semantic_frame
      )
        ? TVideoCallIncomingAcceptFailedSemanticFrame.fromJSON(
            object.video_call_incoming_accept_failed_semantic_frame
          )
        : undefined,
      IotScenarioStepActionsSemanticFrame: isSet(
        object.iot_scenario_step_actions_semantic_frame
      )
        ? TIotScenarioStepActionsSemanticFrame.fromJSON(
            object.iot_scenario_step_actions_semantic_frame
          )
        : undefined,
      PhoneCallSemanticFrame: isSet(object.phone_call_semantic_frame)
        ? TPhoneCallSemanticFrame.fromJSON(object.phone_call_semantic_frame)
        : undefined,
      OpenAddressBookSemanticFrame: isSet(
        object.open_address_book_semantic_frame
      )
        ? TOpenAddressBookSemanticFrame.fromJSON(
            object.open_address_book_semantic_frame
          )
        : undefined,
      GetEqualizerSettingsSemanticFrame: isSet(
        object.get_equalizer_settings_semantic_frame
      )
        ? TGetEqualizerSettingsSemanticFrame.fromJSON(
            object.get_equalizer_settings_semantic_frame
          )
        : undefined,
      VideoCallToSemanticFrame: isSet(object.video_call_to_semantic_frame)
        ? TVideoCallToSemanticFrame.fromJSON(
            object.video_call_to_semantic_frame
          )
        : undefined,
      OnboardingGetGreetingsSemanticFrame: isSet(
        object.onboarding_get_greetings_semantic_frame
      )
        ? TOnboardingGetGreetingsSemanticFrame.fromJSON(
            object.onboarding_get_greetings_semantic_frame
          )
        : undefined,
      OpenTandemSettingSemanticFrame: isSet(
        object.open_tandem_setting_semantic_frame
      )
        ? TOpenTandemSettingSemanticFrame.fromJSON(
            object.open_tandem_setting_semantic_frame
          )
        : undefined,
      OpenSmartSpeakerSettingSemanticFrame: isSet(
        object.open_smart_speaker_setting_semantic_frame
      )
        ? TOpenSmartSpeakerSettingSemanticFrame.fromJSON(
            object.open_smart_speaker_setting_semantic_frame
          )
        : undefined,
      FinishIotSystemDiscoverySemanticFrame: isSet(
        object.finish_iot_system_discovery_semantic_frame
      )
        ? TFinishIotSystemDiscoverySemanticFrame.fromJSON(
            object.finish_iot_system_discovery_semantic_frame
          )
        : undefined,
      VideoCallSetFavoritesSemanticFrame: isSet(
        object.video_call_set_favorites_semantic_frame
      )
        ? TVideoCallSetFavoritesSemanticFrame.fromJSON(
            object.video_call_set_favorites_semantic_frame
          )
        : undefined,
      VideoCallIncomingSemanticFrame: isSet(
        object.video_call_incoming_semantic_frame
      )
        ? TVideoCallIncomingSemanticFrame.fromJSON(
            object.video_call_incoming_semantic_frame
          )
        : undefined,
      MessengerCallAcceptSemanticFrame: isSet(
        object.messenger_call_accept_semantic_frame
      )
        ? TMessengerCallAcceptSemanticFrame.fromJSON(
            object.messenger_call_accept_semantic_frame
          )
        : undefined,
      MessengerCallDiscardSemanticFrame: isSet(
        object.messenger_call_discard_semantic_frame
      )
        ? TMessengerCallDiscardSemanticFrame.fromJSON(
            object.messenger_call_discard_semantic_frame
          )
        : undefined,
      TvLongTapTutorialSemanticFrame: isSet(
        object.tv_long_tap_tutorial_semantic_frame
      )
        ? TTvLongTapTutorialSemanticFrame.fromJSON(
            object.tv_long_tap_tutorial_semantic_frame
          )
        : undefined,
      OnboardingWhatCanYouDoSemanticFrame: isSet(
        object.onboarding_what_can_you_do_semantic_frame
      )
        ? TOnboardingWhatCanYouDoSemanticFrame.fromJSON(
            object.onboarding_what_can_you_do_semantic_frame
          )
        : undefined,
      MessengerCallHangupSemanticFrame: isSet(
        object.messenger_call_hangup_semantic_frame
      )
        ? TMessengerCallHangupSemanticFrame.fromJSON(
            object.messenger_call_hangup_semantic_frame
          )
        : undefined,
      PutMoneyOnPhoneSemanticFrame: isSet(
        object.put_money_on_phone_semantic_frame
      )
        ? TPutMoneyOnPhoneSemanticFrame.fromJSON(
            object.put_money_on_phone_semantic_frame
          )
        : undefined,
      OrderNotificationSemanticFrame: isSet(
        object.order_notification_semantic_frame
      )
        ? TOrderNotificationSemanticFrame.fromJSON(
            object.order_notification_semantic_frame
          )
        : undefined,
      PlayerRemoveLikeSemanticFrame: isSet(
        object.player_remove_like_semantic_frame
      )
        ? TPlayerRemoveLikeSemanticFrame.fromJSON(
            object.player_remove_like_semantic_frame
          )
        : undefined,
      PlayerRemoveDislikeSemanticFrame: isSet(
        object.player_remove_dislike_semantic_frame
      )
        ? TPlayerRemoveDislikeSemanticFrame.fromJSON(
            object.player_remove_dislike_semantic_frame
          )
        : undefined,
      CentaurCollectUpperShutterSemanticFrame: isSet(
        object.centaur_collect_upper_shutter
      )
        ? TCentaurCollectUpperShutterSemanticFrame.fromJSON(
            object.centaur_collect_upper_shutter
          )
        : undefined,
      EnrollmentStatusSemanticFrame: isSet(
        object.multiaccount_enrollment_status_semantic_frame
      )
        ? TEnrollmentStatusSemanticFrame.fromJSON(
            object.multiaccount_enrollment_status_semantic_frame
          )
        : undefined,
      GalleryVideoSelectSemanticFrame: isSet(
        object.gallery_video_select_semantic_frame
      )
        ? TGalleryVideoSelectSemanticFrame.fromJSON(
            object.gallery_video_select_semantic_frame
          )
        : undefined,
      PlayerSongsByThisArtistSemanticFrame: isSet(
        object.player_songs_by_this_artist_semantic_frame
      )
        ? TPlayerSongsByThisArtistSemanticFrame.fromJSON(
            object.player_songs_by_this_artist_semantic_frame
          )
        : undefined,
      ExternalSkillEpisodeForShowRequestSemanticFrame: isSet(
        object.external_skill_episode_for_show_request_semantic_frame
      )
        ? TExternalSkillEpisodeForShowRequestSemanticFrame.fromJSON(
            object.external_skill_episode_for_show_request_semantic_frame
          )
        : undefined,
      MusicPlayFixlistSemanticFrame: isSet(
        object.music_play_fixlist_semantic_frame
      )
        ? TMusicPlayFixlistSemanticFrame.fromJSON(
            object.music_play_fixlist_semantic_frame
          )
        : undefined,
      MusicPlayAnaphoraSemanticFrame: isSet(
        object.music_play_anaphora_semantic_frame
      )
        ? TMusicPlayAnaphoraSemanticFrame.fromJSON(
            object.music_play_anaphora_semantic_frame
          )
        : undefined,
      MusicPlayFairytaleSemanticFrame: isSet(
        object.music_play_fairytale_semantic_frame
      )
        ? TMusicPlayFairytaleSemanticFrame.fromJSON(
            object.music_play_fairytale_semantic_frame
          )
        : undefined,
      StartMultiroomSemanticFrame: isSet(object.start_multiroom_semantic_frame)
        ? TStartMultiroomSemanticFrame.fromJSON(
            object.start_multiroom_semantic_frame
          )
        : undefined,
      PlayerWhatIsThisSongAboutSemanticFrame: isSet(
        object.player_what_is_this_song_about_semantic_frame
      )
        ? TPlayerWhatIsThisSongAboutSemanticFrame.fromJSON(
            object.player_what_is_this_song_about_semantic_frame
          )
        : undefined,
      GuestEnrollmentFinishSemanticFrame: isSet(
        object.guest_enrollment_finish_semantic_frame
      )
        ? TGuestEnrollmentFinishSemanticFrame.fromJSON(
            object.guest_enrollment_finish_semantic_frame
          )
        : undefined,
      CentaurSetTeaserConfigurationSemanticFrame: isSet(
        object.centaur_set_teaser_configuration
      )
        ? TCentaurSetTeaserConfigurationSemanticFrame.fromJSON(
            object.centaur_set_teaser_configuration
          )
        : undefined,
      CentaurCollectTeasersPreviewSemanticFrame: isSet(
        object.centaur_collect_teasers_preview
      )
        ? TCentaurCollectTeasersPreviewSemanticFrame.fromJSON(
            object.centaur_collect_teasers_preview
          )
        : undefined,
    };
  },

  toJSON(message: TTypedSemanticFrame): unknown {
    const obj: any = {};
    message.SearchSemanticFrame !== undefined &&
      (obj.search_semantic_frame = message.SearchSemanticFrame
        ? TSearchSemanticFrame.toJSON(message.SearchSemanticFrame)
        : undefined);
    message.IoTBroadcastStartSemanticFrame !== undefined &&
      (obj.iot_broadcast_start = message.IoTBroadcastStartSemanticFrame
        ? TIoTBroadcastStartSemanticFrame.toJSON(
            message.IoTBroadcastStartSemanticFrame
          )
        : undefined);
    message.IoTBroadcastSuccessSemanticFrame !== undefined &&
      (obj.iot_broadcast_success = message.IoTBroadcastSuccessSemanticFrame
        ? TIoTBroadcastSuccessSemanticFrame.toJSON(
            message.IoTBroadcastSuccessSemanticFrame
          )
        : undefined);
    message.IoTBroadcastFailureSemanticFrame !== undefined &&
      (obj.iot_broadcast_failure = message.IoTBroadcastFailureSemanticFrame
        ? TIoTBroadcastFailureSemanticFrame.toJSON(
            message.IoTBroadcastFailureSemanticFrame
          )
        : undefined);
    message.MordoviaHomeScreenSemanticFrame !== undefined &&
      (obj.mordovia_home_screen = message.MordoviaHomeScreenSemanticFrame
        ? TMordoviaHomeScreenSemanticFrame.toJSON(
            message.MordoviaHomeScreenSemanticFrame
          )
        : undefined);
    message.NewsSemanticFrame !== undefined &&
      (obj.news_semantic_frame = message.NewsSemanticFrame
        ? TNewsSemanticFrame.toJSON(message.NewsSemanticFrame)
        : undefined);
    message.GetCallerNameSemanticFrame !== undefined &&
      (obj.get_caller_name = message.GetCallerNameSemanticFrame
        ? TGetCallerNameSemanticFrame.toJSON(message.GetCallerNameSemanticFrame)
        : undefined);
    message.MusicPlaySemanticFrame !== undefined &&
      (obj.music_play_semantic_frame = message.MusicPlaySemanticFrame
        ? TMusicPlaySemanticFrame.toJSON(message.MusicPlaySemanticFrame)
        : undefined);
    message.ExternalSkillActivateSemanticFrame !== undefined &&
      (obj.external_skill_activate_semantic_frame =
        message.ExternalSkillActivateSemanticFrame
          ? TExternalSkillActivateSemanticFrame.toJSON(
              message.ExternalSkillActivateSemanticFrame
            )
          : undefined);
    message.VideoPlaySemanticFrame !== undefined &&
      (obj.video_play_semantic_frame = message.VideoPlaySemanticFrame
        ? TVideoPlaySemanticFrame.toJSON(message.VideoPlaySemanticFrame)
        : undefined);
    message.WeatherSemanticFrame !== undefined &&
      (obj.weather_semantic_frame = message.WeatherSemanticFrame
        ? TWeatherSemanticFrame.toJSON(message.WeatherSemanticFrame)
        : undefined);
    message.HardcodedMorningShowSemanticFrame !== undefined &&
      (obj.morning_show_semantic_frame =
        message.HardcodedMorningShowSemanticFrame
          ? THardcodedMorningShowSemanticFrame.toJSON(
              message.HardcodedMorningShowSemanticFrame
            )
          : undefined);
    message.SelectVideoFromGallerySemanticFrame !== undefined &&
      (obj.select_video = message.SelectVideoFromGallerySemanticFrame
        ? TSelectVideoFromGallerySemanticFrame.toJSON(
            message.SelectVideoFromGallerySemanticFrame
          )
        : undefined);
    message.OpenCurrentVideoSemanticFrame !== undefined &&
      (obj.open_current_video = message.OpenCurrentVideoSemanticFrame
        ? TOpenCurrentVideoSemanticFrame.toJSON(
            message.OpenCurrentVideoSemanticFrame
          )
        : undefined);
    message.VideoPaymentConfirmedSemanticFrame !== undefined &&
      (obj.video_payment_confirmed = message.VideoPaymentConfirmedSemanticFrame
        ? TVideoPaymentConfirmedSemanticFrame.toJSON(
            message.VideoPaymentConfirmedSemanticFrame
          )
        : undefined);
    message.PlayerNextTrackSemanticFrame !== undefined &&
      (obj.player_next_track_semantic_frame =
        message.PlayerNextTrackSemanticFrame
          ? TPlayerNextTrackSemanticFrame.toJSON(
              message.PlayerNextTrackSemanticFrame
            )
          : undefined);
    message.PlayerPrevTrackSemanticFrame !== undefined &&
      (obj.player_prev_track_semantic_frame =
        message.PlayerPrevTrackSemanticFrame
          ? TPlayerPrevTrackSemanticFrame.toJSON(
              message.PlayerPrevTrackSemanticFrame
            )
          : undefined);
    message.PlayerLikeSemanticFrame !== undefined &&
      (obj.player_like_semantic_frame = message.PlayerLikeSemanticFrame
        ? TPlayerLikeSemanticFrame.toJSON(message.PlayerLikeSemanticFrame)
        : undefined);
    message.PlayerDislikeSemanticFrame !== undefined &&
      (obj.player_dislike_semantic_frame = message.PlayerDislikeSemanticFrame
        ? TPlayerDislikeSemanticFrame.toJSON(message.PlayerDislikeSemanticFrame)
        : undefined);
    message.DoNothingSemanticFrame !== undefined &&
      (obj.do_nothing_semantic_frame = message.DoNothingSemanticFrame
        ? TDoNothingSemanticFrame.toJSON(message.DoNothingSemanticFrame)
        : undefined);
    message.NotificationsSubscribeSemanticFrame !== undefined &&
      (obj.notifications_subscribe_semantic_frame =
        message.NotificationsSubscribeSemanticFrame
          ? TNotificationsSubscribeSemanticFrame.toJSON(
              message.NotificationsSubscribeSemanticFrame
            )
          : undefined);
    message.VideoRaterSemanticFrame !== undefined &&
      (obj.video_rater_semantic_frame = message.VideoRaterSemanticFrame
        ? TVideoRaterSemanticFrame.toJSON(message.VideoRaterSemanticFrame)
        : undefined);
    message.SetupRcuStatusSemanticFrame !== undefined &&
      (obj["setup_rcu.status"] = message.SetupRcuStatusSemanticFrame
        ? TSetupRcuStatusSemanticFrame.toJSON(
            message.SetupRcuStatusSemanticFrame
          )
        : undefined);
    message.SetupRcuAutoStatusSemanticFrame !== undefined &&
      (obj["setup_rcu_auto.status"] = message.SetupRcuAutoStatusSemanticFrame
        ? TSetupRcuAutoStatusSemanticFrame.toJSON(
            message.SetupRcuAutoStatusSemanticFrame
          )
        : undefined);
    message.SetupRcuCheckStatusSemanticFrame !== undefined &&
      (obj["setup_rcu_check.status"] = message.SetupRcuCheckStatusSemanticFrame
        ? TSetupRcuCheckStatusSemanticFrame.toJSON(
            message.SetupRcuCheckStatusSemanticFrame
          )
        : undefined);
    message.SetupRcuAdvancedStatusSemanticFrame !== undefined &&
      (obj["setup_rcu_advanced.status"] =
        message.SetupRcuAdvancedStatusSemanticFrame
          ? TSetupRcuAdvancedStatusSemanticFrame.toJSON(
              message.SetupRcuAdvancedStatusSemanticFrame
            )
          : undefined);
    message.SetupRcuManualStartSemanticFrame !== undefined &&
      (obj["setup_rcu_manual.start"] = message.SetupRcuManualStartSemanticFrame
        ? TSetupRcuManualStartSemanticFrame.toJSON(
            message.SetupRcuManualStartSemanticFrame
          )
        : undefined);
    message.SetupRcuAutoStartSemanticFrame !== undefined &&
      (obj["setup_rcu_auto.start"] = message.SetupRcuAutoStartSemanticFrame
        ? TSetupRcuAutoStartSemanticFrame.toJSON(
            message.SetupRcuAutoStartSemanticFrame
          )
        : undefined);
    message.LinkARemoteSemanticFrame !== undefined &&
      (obj.link_a_remote = message.LinkARemoteSemanticFrame
        ? TLinkARemoteSemanticFrame.toJSON(message.LinkARemoteSemanticFrame)
        : undefined);
    message.RequestTechnicalSupportSemanticFrame !== undefined &&
      (obj.request_technical_support =
        message.RequestTechnicalSupportSemanticFrame
          ? TRequestTechnicalSupportSemanticFrame.toJSON(
              message.RequestTechnicalSupportSemanticFrame
            )
          : undefined);
    message.IoTDiscoveryStartSemanticFrame !== undefined &&
      (obj.iot_discovery_start = message.IoTDiscoveryStartSemanticFrame
        ? TIoTDiscoveryStartSemanticFrame.toJSON(
            message.IoTDiscoveryStartSemanticFrame
          )
        : undefined);
    message.IoTDiscoverySuccessSemanticFrame !== undefined &&
      (obj.iot_discovery_success = message.IoTDiscoverySuccessSemanticFrame
        ? TIoTDiscoverySuccessSemanticFrame.toJSON(
            message.IoTDiscoverySuccessSemanticFrame
          )
        : undefined);
    message.IoTDiscoveryFailureSemanticFrame !== undefined &&
      (obj.iot_discovery_failure = message.IoTDiscoveryFailureSemanticFrame
        ? TIoTDiscoveryFailureSemanticFrame.toJSON(
            message.IoTDiscoveryFailureSemanticFrame
          )
        : undefined);
    message.ExternalSkillFixedActivateSemanticFrame !== undefined &&
      (obj.external_skill_fixed_activate_semantic_frame =
        message.ExternalSkillFixedActivateSemanticFrame
          ? TExternalSkillFixedActivateSemanticFrame.toJSON(
              message.ExternalSkillFixedActivateSemanticFrame
            )
          : undefined);
    message.OpenCurrentTrailerSemanticFrame !== undefined &&
      (obj.open_current_trailer = message.OpenCurrentTrailerSemanticFrame
        ? TOpenCurrentTrailerSemanticFrame.toJSON(
            message.OpenCurrentTrailerSemanticFrame
          )
        : undefined);
    message.OnboardingStartingCriticalUpdateSemanticFrame !== undefined &&
      (obj.onboarding_starting_critical_update =
        message.OnboardingStartingCriticalUpdateSemanticFrame
          ? TOnboardingStartingCriticalUpdateSemanticFrame.toJSON(
              message.OnboardingStartingCriticalUpdateSemanticFrame
            )
          : undefined);
    message.OnboardingStartingConfigureSuccessSemanticFrame !== undefined &&
      (obj.onboarding_starting_configure_success =
        message.OnboardingStartingConfigureSuccessSemanticFrame
          ? TOnboardingStartingConfigureSuccessSemanticFrame.toJSON(
              message.OnboardingStartingConfigureSuccessSemanticFrame
            )
          : undefined);
    message.RadioPlaySemanticFrame !== undefined &&
      (obj.radio_play_semantic_frame = message.RadioPlaySemanticFrame
        ? TRadioPlaySemanticFrame.toJSON(message.RadioPlaySemanticFrame)
        : undefined);
    message.CentaurCollectCardsSemanticFrame !== undefined &&
      (obj.centaur_collect_cards = message.CentaurCollectCardsSemanticFrame
        ? TCentaurCollectCardsSemanticFrame.toJSON(
            message.CentaurCollectCardsSemanticFrame
          )
        : undefined);
    message.CentaurGetCardSemanticFrame !== undefined &&
      (obj.centaur_get_card = message.CentaurGetCardSemanticFrame
        ? TCentaurGetCardSemanticFrame.toJSON(
            message.CentaurGetCardSemanticFrame
          )
        : undefined);
    message.VideoPlayerFinishedSemanticFrame !== undefined &&
      (obj["quasar.video_player.finished"] =
        message.VideoPlayerFinishedSemanticFrame
          ? TVideoPlayerFinishedSemanticFrame.toJSON(
              message.VideoPlayerFinishedSemanticFrame
            )
          : undefined);
    message.SoundLouderSemanticFrame !== undefined &&
      (obj.sound_louder_semantic_frame = message.SoundLouderSemanticFrame
        ? TSoundLouderSemanticFrame.toJSON(message.SoundLouderSemanticFrame)
        : undefined);
    message.SoundQuiterSemanticFrame !== undefined &&
      (obj.sound_quiter_semantic_frame = message.SoundQuiterSemanticFrame
        ? TSoundQuiterSemanticFrame.toJSON(message.SoundQuiterSemanticFrame)
        : undefined);
    message.SoundSetLevelSemanticFrame !== undefined &&
      (obj.sound_set_level_semantic_frame = message.SoundSetLevelSemanticFrame
        ? TSoundSetLevelSemanticFrame.toJSON(message.SoundSetLevelSemanticFrame)
        : undefined);
    message.GetPhotoFrameSemanticFrame !== undefined &&
      (obj.get_photo_frame = message.GetPhotoFrameSemanticFrame
        ? TGetPhotoFrameSemanticFrame.toJSON(message.GetPhotoFrameSemanticFrame)
        : undefined);
    message.CentaurCollectMainScreenSemanticFrame !== undefined &&
      (obj.centaur_collect_main_screen =
        message.CentaurCollectMainScreenSemanticFrame
          ? TCentaurCollectMainScreenSemanticFrame.toJSON(
              message.CentaurCollectMainScreenSemanticFrame
            )
          : undefined);
    message.GetTimeSemanticFrame !== undefined &&
      (obj.get_time_semantic_frame = message.GetTimeSemanticFrame
        ? TGetTimeSemanticFrame.toJSON(message.GetTimeSemanticFrame)
        : undefined);
    message.HowToSubscribeSemanticFrame !== undefined &&
      (obj.how_to_subscribe_semantic_frame = message.HowToSubscribeSemanticFrame
        ? THowToSubscribeSemanticFrame.toJSON(
            message.HowToSubscribeSemanticFrame
          )
        : undefined);
    message.MusicOnboardingSemanticFrame !== undefined &&
      (obj.music_onboarding_semantic_frame =
        message.MusicOnboardingSemanticFrame
          ? TMusicOnboardingSemanticFrame.toJSON(
              message.MusicOnboardingSemanticFrame
            )
          : undefined);
    message.MusicOnboardingArtistsSemanticFrame !== undefined &&
      (obj.music_onboarding_artists_semantic_frame =
        message.MusicOnboardingArtistsSemanticFrame
          ? TMusicOnboardingArtistsSemanticFrame.toJSON(
              message.MusicOnboardingArtistsSemanticFrame
            )
          : undefined);
    message.MusicOnboardingGenresSemanticFrame !== undefined &&
      (obj.music_onboarding_genres_semantic_frame =
        message.MusicOnboardingGenresSemanticFrame
          ? TMusicOnboardingGenresSemanticFrame.toJSON(
              message.MusicOnboardingGenresSemanticFrame
            )
          : undefined);
    message.MusicOnboardingTracksSemanticFrame !== undefined &&
      (obj.music_onboarding_tracks_semantic_frame =
        message.MusicOnboardingTracksSemanticFrame
          ? TMusicOnboardingTracksSemanticFrame.toJSON(
              message.MusicOnboardingTracksSemanticFrame
            )
          : undefined);
    message.PlayerContinueSemanticFrame !== undefined &&
      (obj.player_continue_semantic_frame = message.PlayerContinueSemanticFrame
        ? TPlayerContinueSemanticFrame.toJSON(
            message.PlayerContinueSemanticFrame
          )
        : undefined);
    message.PlayerWhatIsPlayingSemanticFrame !== undefined &&
      (obj.player_what_is_playing_semantic_frame =
        message.PlayerWhatIsPlayingSemanticFrame
          ? TPlayerWhatIsPlayingSemanticFrame.toJSON(
              message.PlayerWhatIsPlayingSemanticFrame
            )
          : undefined);
    message.PlayerShuffleSemanticFrame !== undefined &&
      (obj.player_shuffle_semantic_frame = message.PlayerShuffleSemanticFrame
        ? TPlayerShuffleSemanticFrame.toJSON(message.PlayerShuffleSemanticFrame)
        : undefined);
    message.PlayerReplaySemanticFrame !== undefined &&
      (obj.player_replay_semantic_frame = message.PlayerReplaySemanticFrame
        ? TPlayerReplaySemanticFrame.toJSON(message.PlayerReplaySemanticFrame)
        : undefined);
    message.PlayerRewindSemanticFrame !== undefined &&
      (obj.player_rewind_semantic_frame = message.PlayerRewindSemanticFrame
        ? TPlayerRewindSemanticFrame.toJSON(message.PlayerRewindSemanticFrame)
        : undefined);
    message.PlayerRepeatSemanticFrame !== undefined &&
      (obj.player_repeat_semantic_frame = message.PlayerRepeatSemanticFrame
        ? TPlayerRepeatSemanticFrame.toJSON(message.PlayerRepeatSemanticFrame)
        : undefined);
    message.GetSmartTvCategoriesSemanticFrame !== undefined &&
      (obj.get_smart_tv_categories_semantic_frame =
        message.GetSmartTvCategoriesSemanticFrame
          ? TGetSmartTvCategoriesSemanticFrame.toJSON(
              message.GetSmartTvCategoriesSemanticFrame
            )
          : undefined);
    message.IoTScenariosPhraseActionSemanticFrame !== undefined &&
      (obj.iot_scenarios_phrase_action_semantic_frame =
        message.IoTScenariosPhraseActionSemanticFrame
          ? TIoTScenariosPhraseActionSemanticFrame.toJSON(
              message.IoTScenariosPhraseActionSemanticFrame
            )
          : undefined);
    message.IoTScenariosTextActionSemanticFrame !== undefined &&
      (obj.iot_scenarios_text_action_semantic_frame =
        message.IoTScenariosTextActionSemanticFrame
          ? TIoTScenariosTextActionSemanticFrame.toJSON(
              message.IoTScenariosTextActionSemanticFrame
            )
          : undefined);
    message.IoTScenariosLaunchActionSemanticFrame !== undefined &&
      (obj.iot_scenarios_launch_action_semantic_frame =
        message.IoTScenariosLaunchActionSemanticFrame
          ? TIoTScenariosLaunchActionSemanticFrame.toJSON(
              message.IoTScenariosLaunchActionSemanticFrame
            )
          : undefined);
    message.AlarmSetAliceShowSemanticFrame !== undefined &&
      (obj.alarm_set_alice_show_semantic_frame =
        message.AlarmSetAliceShowSemanticFrame
          ? TAlarmSetAliceShowSemanticFrame.toJSON(
              message.AlarmSetAliceShowSemanticFrame
            )
          : undefined);
    message.PlayerUnshuffleSemanticFrame !== undefined &&
      (obj.player_unshuffle_semantic_frame =
        message.PlayerUnshuffleSemanticFrame
          ? TPlayerUnshuffleSemanticFrame.toJSON(
              message.PlayerUnshuffleSemanticFrame
            )
          : undefined);
    message.RemindersOnShootSemanticFrame !== undefined &&
      (obj.reminders_on_shoot_semantic_frame =
        message.RemindersOnShootSemanticFrame
          ? TRemindersOnShootSemanticFrame.toJSON(
              message.RemindersOnShootSemanticFrame
            )
          : undefined);
    message.RepeatAfterMeSemanticFrame !== undefined &&
      (obj.repeat_after_me_semantic_frame = message.RepeatAfterMeSemanticFrame
        ? TRepeatAfterMeSemanticFrame.toJSON(message.RepeatAfterMeSemanticFrame)
        : undefined);
    message.MediaPlaySemanticFrame !== undefined &&
      (obj.media_play_semantic_frame = message.MediaPlaySemanticFrame
        ? TMediaPlaySemanticFrame.toJSON(message.MediaPlaySemanticFrame)
        : undefined);
    message.AliceShowActivateSemanticFrame !== undefined &&
      (obj.alice_show_activate_semantic_frame =
        message.AliceShowActivateSemanticFrame
          ? TAliceShowActivateSemanticFrame.toJSON(
              message.AliceShowActivateSemanticFrame
            )
          : undefined);
    message.ZenContextSearchStartSemanticFrame !== undefined &&
      (obj.zen_context_search_start_semantic_frame =
        message.ZenContextSearchStartSemanticFrame
          ? TZenContextSearchStartSemanticFrame.toJSON(
              message.ZenContextSearchStartSemanticFrame
            )
          : undefined);
    message.GetSmartTvCarouselSemanticFrame !== undefined &&
      (obj.get_smart_tv_carousel_semantic_frame =
        message.GetSmartTvCarouselSemanticFrame
          ? TGetSmartTvCarouselSemanticFrame.toJSON(
              message.GetSmartTvCarouselSemanticFrame
            )
          : undefined);
    message.GetSmartTvCarouselsSemanticFrame !== undefined &&
      (obj.get_smart_tv_carousels_semantic_frame =
        message.GetSmartTvCarouselsSemanticFrame
          ? TGetSmartTvCarouselsSemanticFrame.toJSON(
              message.GetSmartTvCarouselsSemanticFrame
            )
          : undefined);
    message.AppsFixlistSemanticFrame !== undefined &&
      (obj.apps_fixlist_semantic_frame = message.AppsFixlistSemanticFrame
        ? TAppsFixlistSemanticFrame.toJSON(message.AppsFixlistSemanticFrame)
        : undefined);
    message.PlayerPauseSemanticFrame !== undefined &&
      (obj.player_pause_semantic_frame = message.PlayerPauseSemanticFrame
        ? TPlayerPauseSemanticFrame.toJSON(message.PlayerPauseSemanticFrame)
        : undefined);
    message.IoTScenarioSpeakerActionSemanticFrame !== undefined &&
      (obj.iot_scenario_speaker_action_semantic_frame =
        message.IoTScenarioSpeakerActionSemanticFrame
          ? TIoTScenarioSpeakerActionSemanticFrame.toJSON(
              message.IoTScenarioSpeakerActionSemanticFrame
            )
          : undefined);
    message.VideoCardDetailSemanticFrame !== undefined &&
      (obj.get_video_card_detail_semantic_frame =
        message.VideoCardDetailSemanticFrame
          ? TVideoCardDetailSemanticFrame.toJSON(
              message.VideoCardDetailSemanticFrame
            )
          : undefined);
    message.TurnClockFaceOnSemanticFrame !== undefined &&
      (obj.turn_clock_face_on_semantic_frame =
        message.TurnClockFaceOnSemanticFrame
          ? TTurnClockFaceOnSemanticFrame.toJSON(
              message.TurnClockFaceOnSemanticFrame
            )
          : undefined);
    message.TurnClockFaceOffSemanticFrame !== undefined &&
      (obj.turn_clock_face_off_semantic_frame =
        message.TurnClockFaceOffSemanticFrame
          ? TTurnClockFaceOffSemanticFrame.toJSON(
              message.TurnClockFaceOffSemanticFrame
            )
          : undefined);
    message.RemindersOnCancelSemanticFrame !== undefined &&
      (obj.reminders_on_cancel_semantic_frame =
        message.RemindersOnCancelSemanticFrame
          ? TRemindersCancelSemanticFrame.toJSON(
              message.RemindersOnCancelSemanticFrame
            )
          : undefined);
    message.IoTDeviceActionSemanticFrame !== undefined &&
      (obj.iot_device_action_semantic_frame =
        message.IoTDeviceActionSemanticFrame
          ? TIoTDeviceActionSemanticFrame.toJSON(
              message.IoTDeviceActionSemanticFrame
            )
          : undefined);
    message.VideoThinCardDetailSmanticFrame !== undefined &&
      (obj.get_video_thin_card_detail_semantic_frame =
        message.VideoThinCardDetailSmanticFrame
          ? TVideoThinCardDetailSemanticFrame.toJSON(
              message.VideoThinCardDetailSmanticFrame
            )
          : undefined);
    message.WhisperSaySomethingSemanticFrame !== undefined &&
      (obj.whisper_say_something_semantic_frame =
        message.WhisperSaySomethingSemanticFrame
          ? TWhisperSaySomethingSemanticFrame.toJSON(
              message.WhisperSaySomethingSemanticFrame
            )
          : undefined);
    message.WhisperTurnOffSemanticFrame !== undefined &&
      (obj.whisper_turn_off_semantic_frame = message.WhisperTurnOffSemanticFrame
        ? TWhisperTurnOffSemanticFrame.toJSON(
            message.WhisperTurnOffSemanticFrame
          )
        : undefined);
    message.WhisperTurnOnSemanticFrame !== undefined &&
      (obj.whisper_turn_on_semantic_frame = message.WhisperTurnOnSemanticFrame
        ? TWhisperTurnOnSemanticFrame.toJSON(message.WhisperTurnOnSemanticFrame)
        : undefined);
    message.WhisperWhatIsItSemanticFrame !== undefined &&
      (obj.whisper_what_is_it_semantic_frame =
        message.WhisperWhatIsItSemanticFrame
          ? TWhisperWhatIsItSemanticFrame.toJSON(
              message.WhisperWhatIsItSemanticFrame
            )
          : undefined);
    message.TimeCapsuleNextStepSemanticFrame !== undefined &&
      (obj.time_capsule_next_step_semantic_frame =
        message.TimeCapsuleNextStepSemanticFrame
          ? TTimeCapsuleNextStepSemanticFrame.toJSON(
              message.TimeCapsuleNextStepSemanticFrame
            )
          : undefined);
    message.TimeCapsuleStopSemanticFrame !== undefined &&
      (obj.time_capsule_stop_semantic_frame =
        message.TimeCapsuleStopSemanticFrame
          ? TTimeCapsuleStopSemanticFrame.toJSON(
              message.TimeCapsuleStopSemanticFrame
            )
          : undefined);
    message.ActivateGenerativeTaleSemanticFrame !== undefined &&
      (obj.activate_generative_tale_semantic_frame =
        message.ActivateGenerativeTaleSemanticFrame
          ? TActivateGenerativeTaleSemanticFrame.toJSON(
              message.ActivateGenerativeTaleSemanticFrame
            )
          : undefined);
    message.StartIotDiscoverySemanticFrame !== undefined &&
      (obj.start_iot_discovery_semantic_frame =
        message.StartIotDiscoverySemanticFrame
          ? TStartIotDiscoverySemanticFrame.toJSON(
              message.StartIotDiscoverySemanticFrame
            )
          : undefined);
    message.FinishIotDiscoverySemanticFrame !== undefined &&
      (obj.finish_iot_discovery_semantic_frame =
        message.FinishIotDiscoverySemanticFrame
          ? TFinishIotDiscoverySemanticFrame.toJSON(
              message.FinishIotDiscoverySemanticFrame
            )
          : undefined);
    message.ForgetIotEndpointsSemanticFrame !== undefined &&
      (obj.forget_iot_endpoints_semantic_frame =
        message.ForgetIotEndpointsSemanticFrame
          ? TForgetIotEndpointsSemanticFrame.toJSON(
              message.ForgetIotEndpointsSemanticFrame
            )
          : undefined);
    message.HardcodedResponseSemanticFrame !== undefined &&
      (obj.hardcoded_response_semantic_frame =
        message.HardcodedResponseSemanticFrame
          ? THardcodedResponseSemanticFrame.toJSON(
              message.HardcodedResponseSemanticFrame
            )
          : undefined);
    message.IotYandexIOActionSemanticFrame !== undefined &&
      (obj.iot_yandex_io_action_semantic_frame =
        message.IotYandexIOActionSemanticFrame
          ? TIotYandexIOActionSemanticFrame.toJSON(
              message.IotYandexIOActionSemanticFrame
            )
          : undefined);
    message.TimeCapsuleStartSemanticFrame !== undefined &&
      (obj.time_capsule_start_semantic_frame =
        message.TimeCapsuleStartSemanticFrame
          ? TTimeCapsuleStartSemanticFrame.toJSON(
              message.TimeCapsuleStartSemanticFrame
            )
          : undefined);
    message.TimeCapsuleResumeSemanticFrame !== undefined &&
      (obj.time_capsule_resume_semantic_frame =
        message.TimeCapsuleResumeSemanticFrame
          ? TTimeCapsuleResumeSemanticFrame.toJSON(
              message.TimeCapsuleResumeSemanticFrame
            )
          : undefined);
    message.AddAccountSemanticFrame !== undefined &&
      (obj.multiaccount_add_account_semantic_frame =
        message.AddAccountSemanticFrame
          ? TAddAccountSemanticFrame.toJSON(message.AddAccountSemanticFrame)
          : undefined);
    message.RemoveAccountSemanticFrame !== undefined &&
      (obj.multiaccount_remove_account_semantic_frame =
        message.RemoveAccountSemanticFrame
          ? TRemoveAccountSemanticFrame.toJSON(
              message.RemoveAccountSemanticFrame
            )
          : undefined);
    message.EndpointStateUpdatesSemanticFrame !== undefined &&
      (obj.endpoint_state_updates_semantic_frame =
        message.EndpointStateUpdatesSemanticFrame
          ? TEndpointStateUpdatesSemanticFrame.toJSON(
              message.EndpointStateUpdatesSemanticFrame
            )
          : undefined);
    message.TimeCapsuleSkipQuestionSemanticFrame !== undefined &&
      (obj.time_capsule_skip_question_semantic_frame =
        message.TimeCapsuleSkipQuestionSemanticFrame
          ? TTimeCapsuleSkipQuestionSemanticFrame.toJSON(
              message.TimeCapsuleSkipQuestionSemanticFrame
            )
          : undefined);
    message.SkillSessionRequestSemanticFrame !== undefined &&
      (obj.skill_session_request_semantic_frame =
        message.SkillSessionRequestSemanticFrame
          ? TSkillSessionRequestSemanticFrame.toJSON(
              message.SkillSessionRequestSemanticFrame
            )
          : undefined);
    message.StartIotTuyaBroadcastSemanticFrame !== undefined &&
      (obj.start_iot_tuya_broadcast_semantic_frame =
        message.StartIotTuyaBroadcastSemanticFrame
          ? TStartIotTuyaBroadcastSemanticFrame.toJSON(
              message.StartIotTuyaBroadcastSemanticFrame
            )
          : undefined);
    message.OpenSmartDeviceExternalAppFrame !== undefined &&
      (obj.open_smart_device_external_app_frame =
        message.OpenSmartDeviceExternalAppFrame
          ? TOpenSmartDeviceExternalAppFrame.toJSON(
              message.OpenSmartDeviceExternalAppFrame
            )
          : undefined);
    message.RestoreIotNetworksSemanticFrame !== undefined &&
      (obj.restore_iot_networks_semantic_frame =
        message.RestoreIotNetworksSemanticFrame
          ? TRestoreIotNetworksSemanticFrame.toJSON(
              message.RestoreIotNetworksSemanticFrame
            )
          : undefined);
    message.SaveIotNetworksSemanticFrame !== undefined &&
      (obj.save_iot_networks_semantic_frame =
        message.SaveIotNetworksSemanticFrame
          ? TSaveIotNetworksSemanticFrame.toJSON(
              message.SaveIotNetworksSemanticFrame
            )
          : undefined);
    message.GuestEnrollmentStartSemanticFrame !== undefined &&
      (obj.guest_enrollment_start_semantic_frame =
        message.GuestEnrollmentStartSemanticFrame
          ? TGuestEnrollmentStartSemanticFrame.toJSON(
              message.GuestEnrollmentStartSemanticFrame
            )
          : undefined);
    message.TvPromoTemplateRequestSemanticFrame !== undefined &&
      (obj.tv_promo_request_semantic_frame =
        message.TvPromoTemplateRequestSemanticFrame
          ? TTvPromoTemplateRequestSemanticFrame.toJSON(
              message.TvPromoTemplateRequestSemanticFrame
            )
          : undefined);
    message.TvPromoTemplateShownReportSemanticFrame !== undefined &&
      (obj.tv_promo_template_shown_report_semantic_frame =
        message.TvPromoTemplateShownReportSemanticFrame
          ? TTvPromoTemplateShownReportSemanticFrame.toJSON(
              message.TvPromoTemplateShownReportSemanticFrame
            )
          : undefined);
    message.UploadContactsRequestSemanticFrame !== undefined &&
      (obj.upload_contacts_request_semantic_frame =
        message.UploadContactsRequestSemanticFrame
          ? TUploadContactsRequestSemanticFrame.toJSON(
              message.UploadContactsRequestSemanticFrame
            )
          : undefined);
    message.DeleteIotNetworksSemanticFrame !== undefined &&
      (obj.delete_iot_networks_semantic_frame =
        message.DeleteIotNetworksSemanticFrame
          ? TDeleteIotNetworksSemanticFrame.toJSON(
              message.DeleteIotNetworksSemanticFrame
            )
          : undefined);
    message.CentaurCollectWidgetGallerySemanticFrame !== undefined &&
      (obj.centaur_collect_widget_gallery_semantic_frame =
        message.CentaurCollectWidgetGallerySemanticFrame
          ? TCentaurCollectWidgetGallerySemanticFrame.toJSON(
              message.CentaurCollectWidgetGallerySemanticFrame
            )
          : undefined);
    message.CentaurAddWidgetFromGallerySemanticFrame !== undefined &&
      (obj.centaur_add_widget_from_gallery_semantic_frame =
        message.CentaurAddWidgetFromGallerySemanticFrame
          ? TCentaurAddWidgetFromGallerySemanticFrame.toJSON(
              message.CentaurAddWidgetFromGallerySemanticFrame
            )
          : undefined);
    message.UpdateContactsRequestSemanticFrame !== undefined &&
      (obj.update_contacts_request_semantic_frame =
        message.UpdateContactsRequestSemanticFrame
          ? TUpdateContactsRequestSemanticFrame.toJSON(
              message.UpdateContactsRequestSemanticFrame
            )
          : undefined);
    message.RemindersListSemanticFrame !== undefined &&
      (obj.reminders_list_semantic_frame = message.RemindersListSemanticFrame
        ? TRemindersListSemanticFrame.toJSON(message.RemindersListSemanticFrame)
        : undefined);
    message.CapabilityEventSemanticFrame !== undefined &&
      (obj.capability_event_semantic_frame =
        message.CapabilityEventSemanticFrame
          ? TCapabilityEventSemanticFrame.toJSON(
              message.CapabilityEventSemanticFrame
            )
          : undefined);
    message.ExternalSkillForceDeactivateSemanticFrame !== undefined &&
      (obj.external_skill_force_deactivate_semantic_frame =
        message.ExternalSkillForceDeactivateSemanticFrame
          ? TExternalSkillForceDeactivateSemanticFrame.toJSON(
              message.ExternalSkillForceDeactivateSemanticFrame
            )
          : undefined);
    message.GetVideoGalleries !== undefined &&
      (obj.get_video_galleries_semantic_frame = message.GetVideoGalleries
        ? TGetVideoGalleriesSemanticFrame.toJSON(message.GetVideoGalleries)
        : undefined);
    message.EndpointCapabilityEventsSemanticFrame !== undefined &&
      (obj.endpoint_capability_events_semantic_frame =
        message.EndpointCapabilityEventsSemanticFrame
          ? TEndpointCapabilityEventsSemanticFrame.toJSON(
              message.EndpointCapabilityEventsSemanticFrame
            )
          : undefined);
    message.MusicAnnounceDisableSemanticFrame !== undefined &&
      (obj.music_announce_disable_semantic_frame =
        message.MusicAnnounceDisableSemanticFrame
          ? TMusicAnnounceDisableSemanticFrame.toJSON(
              message.MusicAnnounceDisableSemanticFrame
            )
          : undefined);
    message.MusicAnnounceEnableSemanticFrame !== undefined &&
      (obj.music_announce_enable_semantic_frame =
        message.MusicAnnounceEnableSemanticFrame
          ? TMusicAnnounceEnableSemanticFrame.toJSON(
              message.MusicAnnounceEnableSemanticFrame
            )
          : undefined);
    message.GetTvSearchResult !== undefined &&
      (obj.get_tv_search_result = message.GetTvSearchResult
        ? TGetTvSearchResultSemanticFrame.toJSON(message.GetTvSearchResult)
        : undefined);
    message.SwitchTvChannelSemanticFrame !== undefined &&
      (obj.switch_tv_channel_semantic_frame =
        message.SwitchTvChannelSemanticFrame
          ? TSwitchTvChannelSemanticFrame.toJSON(
              message.SwitchTvChannelSemanticFrame
            )
          : undefined);
    message.ConvertSemanticFrame !== undefined &&
      (obj.convert_semantic_frame = message.ConvertSemanticFrame
        ? TConvertSemanticFrame.toJSON(message.ConvertSemanticFrame)
        : undefined);
    message.GetVideoGallerySemanticFrame !== undefined &&
      (obj.get_video_gallery_semantic_frame =
        message.GetVideoGallerySemanticFrame
          ? TGetVideoGallerySemanticFrame.toJSON(
              message.GetVideoGallerySemanticFrame
            )
          : undefined);
    message.MusicOnboardingTracksReaskSemanticFrame !== undefined &&
      (obj.music_onboarding_tracks_reask_semantic_frame =
        message.MusicOnboardingTracksReaskSemanticFrame
          ? TMusicOnboardingTracksReaskSemanticFrame.toJSON(
              message.MusicOnboardingTracksReaskSemanticFrame
            )
          : undefined);
    message.TestSemanticFrame !== undefined &&
      (obj.test_semantic_frame = message.TestSemanticFrame
        ? TTestSemanticFrame.toJSON(message.TestSemanticFrame)
        : undefined);
    message.EndpointEventsBatchSemanticFrame !== undefined &&
      (obj.endpoint_events_batch_semantic_frame =
        message.EndpointEventsBatchSemanticFrame
          ? TEndpointEventsBatchSemanticFrame.toJSON(
              message.EndpointEventsBatchSemanticFrame
            )
          : undefined);
    message.MediaSessionPlaySemanticFrame !== undefined &&
      (obj.media_session_play_semantic_frame =
        message.MediaSessionPlaySemanticFrame
          ? TMediaSessionPlaySemanticFrame.toJSON(
              message.MediaSessionPlaySemanticFrame
            )
          : undefined);
    message.MediaSessionPauseSemanticFrame !== undefined &&
      (obj.media_session_pause_semantic_frame =
        message.MediaSessionPauseSemanticFrame
          ? TMediaSessionPauseSemanticFrame.toJSON(
              message.MediaSessionPauseSemanticFrame
            )
          : undefined);
    message.FmRadioPlaySemanticFrame !== undefined &&
      (obj.fm_radio_play_semantic_frame = message.FmRadioPlaySemanticFrame
        ? TFmRadioPlaySemanticFrame.toJSON(message.FmRadioPlaySemanticFrame)
        : undefined);
    message.VideoCallLoginFailedSemanticFrame !== undefined &&
      (obj.video_call_login_failed_semantic_frame =
        message.VideoCallLoginFailedSemanticFrame
          ? TVideoCallLoginFailedSemanticFrame.toJSON(
              message.VideoCallLoginFailedSemanticFrame
            )
          : undefined);
    message.VideoCallOutgoingAcceptedSemanticFrame !== undefined &&
      (obj.video_call_outgoing_accepted_semantic_frame =
        message.VideoCallOutgoingAcceptedSemanticFrame
          ? TVideoCallOutgoingAcceptedSemanticFrame.toJSON(
              message.VideoCallOutgoingAcceptedSemanticFrame
            )
          : undefined);
    message.VideoCallOutgoingFailedSemanticFrame !== undefined &&
      (obj.video_call_outgoing_failed_semantic_frame =
        message.VideoCallOutgoingFailedSemanticFrame
          ? TVideoCallOutgoingFailedSemanticFrame.toJSON(
              message.VideoCallOutgoingFailedSemanticFrame
            )
          : undefined);
    message.VideoCallIncomingAcceptFailedSemanticFrame !== undefined &&
      (obj.video_call_incoming_accept_failed_semantic_frame =
        message.VideoCallIncomingAcceptFailedSemanticFrame
          ? TVideoCallIncomingAcceptFailedSemanticFrame.toJSON(
              message.VideoCallIncomingAcceptFailedSemanticFrame
            )
          : undefined);
    message.IotScenarioStepActionsSemanticFrame !== undefined &&
      (obj.iot_scenario_step_actions_semantic_frame =
        message.IotScenarioStepActionsSemanticFrame
          ? TIotScenarioStepActionsSemanticFrame.toJSON(
              message.IotScenarioStepActionsSemanticFrame
            )
          : undefined);
    message.PhoneCallSemanticFrame !== undefined &&
      (obj.phone_call_semantic_frame = message.PhoneCallSemanticFrame
        ? TPhoneCallSemanticFrame.toJSON(message.PhoneCallSemanticFrame)
        : undefined);
    message.OpenAddressBookSemanticFrame !== undefined &&
      (obj.open_address_book_semantic_frame =
        message.OpenAddressBookSemanticFrame
          ? TOpenAddressBookSemanticFrame.toJSON(
              message.OpenAddressBookSemanticFrame
            )
          : undefined);
    message.GetEqualizerSettingsSemanticFrame !== undefined &&
      (obj.get_equalizer_settings_semantic_frame =
        message.GetEqualizerSettingsSemanticFrame
          ? TGetEqualizerSettingsSemanticFrame.toJSON(
              message.GetEqualizerSettingsSemanticFrame
            )
          : undefined);
    message.VideoCallToSemanticFrame !== undefined &&
      (obj.video_call_to_semantic_frame = message.VideoCallToSemanticFrame
        ? TVideoCallToSemanticFrame.toJSON(message.VideoCallToSemanticFrame)
        : undefined);
    message.OnboardingGetGreetingsSemanticFrame !== undefined &&
      (obj.onboarding_get_greetings_semantic_frame =
        message.OnboardingGetGreetingsSemanticFrame
          ? TOnboardingGetGreetingsSemanticFrame.toJSON(
              message.OnboardingGetGreetingsSemanticFrame
            )
          : undefined);
    message.OpenTandemSettingSemanticFrame !== undefined &&
      (obj.open_tandem_setting_semantic_frame =
        message.OpenTandemSettingSemanticFrame
          ? TOpenTandemSettingSemanticFrame.toJSON(
              message.OpenTandemSettingSemanticFrame
            )
          : undefined);
    message.OpenSmartSpeakerSettingSemanticFrame !== undefined &&
      (obj.open_smart_speaker_setting_semantic_frame =
        message.OpenSmartSpeakerSettingSemanticFrame
          ? TOpenSmartSpeakerSettingSemanticFrame.toJSON(
              message.OpenSmartSpeakerSettingSemanticFrame
            )
          : undefined);
    message.FinishIotSystemDiscoverySemanticFrame !== undefined &&
      (obj.finish_iot_system_discovery_semantic_frame =
        message.FinishIotSystemDiscoverySemanticFrame
          ? TFinishIotSystemDiscoverySemanticFrame.toJSON(
              message.FinishIotSystemDiscoverySemanticFrame
            )
          : undefined);
    message.VideoCallSetFavoritesSemanticFrame !== undefined &&
      (obj.video_call_set_favorites_semantic_frame =
        message.VideoCallSetFavoritesSemanticFrame
          ? TVideoCallSetFavoritesSemanticFrame.toJSON(
              message.VideoCallSetFavoritesSemanticFrame
            )
          : undefined);
    message.VideoCallIncomingSemanticFrame !== undefined &&
      (obj.video_call_incoming_semantic_frame =
        message.VideoCallIncomingSemanticFrame
          ? TVideoCallIncomingSemanticFrame.toJSON(
              message.VideoCallIncomingSemanticFrame
            )
          : undefined);
    message.MessengerCallAcceptSemanticFrame !== undefined &&
      (obj.messenger_call_accept_semantic_frame =
        message.MessengerCallAcceptSemanticFrame
          ? TMessengerCallAcceptSemanticFrame.toJSON(
              message.MessengerCallAcceptSemanticFrame
            )
          : undefined);
    message.MessengerCallDiscardSemanticFrame !== undefined &&
      (obj.messenger_call_discard_semantic_frame =
        message.MessengerCallDiscardSemanticFrame
          ? TMessengerCallDiscardSemanticFrame.toJSON(
              message.MessengerCallDiscardSemanticFrame
            )
          : undefined);
    message.TvLongTapTutorialSemanticFrame !== undefined &&
      (obj.tv_long_tap_tutorial_semantic_frame =
        message.TvLongTapTutorialSemanticFrame
          ? TTvLongTapTutorialSemanticFrame.toJSON(
              message.TvLongTapTutorialSemanticFrame
            )
          : undefined);
    message.OnboardingWhatCanYouDoSemanticFrame !== undefined &&
      (obj.onboarding_what_can_you_do_semantic_frame =
        message.OnboardingWhatCanYouDoSemanticFrame
          ? TOnboardingWhatCanYouDoSemanticFrame.toJSON(
              message.OnboardingWhatCanYouDoSemanticFrame
            )
          : undefined);
    message.MessengerCallHangupSemanticFrame !== undefined &&
      (obj.messenger_call_hangup_semantic_frame =
        message.MessengerCallHangupSemanticFrame
          ? TMessengerCallHangupSemanticFrame.toJSON(
              message.MessengerCallHangupSemanticFrame
            )
          : undefined);
    message.PutMoneyOnPhoneSemanticFrame !== undefined &&
      (obj.put_money_on_phone_semantic_frame =
        message.PutMoneyOnPhoneSemanticFrame
          ? TPutMoneyOnPhoneSemanticFrame.toJSON(
              message.PutMoneyOnPhoneSemanticFrame
            )
          : undefined);
    message.OrderNotificationSemanticFrame !== undefined &&
      (obj.order_notification_semantic_frame =
        message.OrderNotificationSemanticFrame
          ? TOrderNotificationSemanticFrame.toJSON(
              message.OrderNotificationSemanticFrame
            )
          : undefined);
    message.PlayerRemoveLikeSemanticFrame !== undefined &&
      (obj.player_remove_like_semantic_frame =
        message.PlayerRemoveLikeSemanticFrame
          ? TPlayerRemoveLikeSemanticFrame.toJSON(
              message.PlayerRemoveLikeSemanticFrame
            )
          : undefined);
    message.PlayerRemoveDislikeSemanticFrame !== undefined &&
      (obj.player_remove_dislike_semantic_frame =
        message.PlayerRemoveDislikeSemanticFrame
          ? TPlayerRemoveDislikeSemanticFrame.toJSON(
              message.PlayerRemoveDislikeSemanticFrame
            )
          : undefined);
    message.CentaurCollectUpperShutterSemanticFrame !== undefined &&
      (obj.centaur_collect_upper_shutter =
        message.CentaurCollectUpperShutterSemanticFrame
          ? TCentaurCollectUpperShutterSemanticFrame.toJSON(
              message.CentaurCollectUpperShutterSemanticFrame
            )
          : undefined);
    message.EnrollmentStatusSemanticFrame !== undefined &&
      (obj.multiaccount_enrollment_status_semantic_frame =
        message.EnrollmentStatusSemanticFrame
          ? TEnrollmentStatusSemanticFrame.toJSON(
              message.EnrollmentStatusSemanticFrame
            )
          : undefined);
    message.GalleryVideoSelectSemanticFrame !== undefined &&
      (obj.gallery_video_select_semantic_frame =
        message.GalleryVideoSelectSemanticFrame
          ? TGalleryVideoSelectSemanticFrame.toJSON(
              message.GalleryVideoSelectSemanticFrame
            )
          : undefined);
    message.PlayerSongsByThisArtistSemanticFrame !== undefined &&
      (obj.player_songs_by_this_artist_semantic_frame =
        message.PlayerSongsByThisArtistSemanticFrame
          ? TPlayerSongsByThisArtistSemanticFrame.toJSON(
              message.PlayerSongsByThisArtistSemanticFrame
            )
          : undefined);
    message.ExternalSkillEpisodeForShowRequestSemanticFrame !== undefined &&
      (obj.external_skill_episode_for_show_request_semantic_frame =
        message.ExternalSkillEpisodeForShowRequestSemanticFrame
          ? TExternalSkillEpisodeForShowRequestSemanticFrame.toJSON(
              message.ExternalSkillEpisodeForShowRequestSemanticFrame
            )
          : undefined);
    message.MusicPlayFixlistSemanticFrame !== undefined &&
      (obj.music_play_fixlist_semantic_frame =
        message.MusicPlayFixlistSemanticFrame
          ? TMusicPlayFixlistSemanticFrame.toJSON(
              message.MusicPlayFixlistSemanticFrame
            )
          : undefined);
    message.MusicPlayAnaphoraSemanticFrame !== undefined &&
      (obj.music_play_anaphora_semantic_frame =
        message.MusicPlayAnaphoraSemanticFrame
          ? TMusicPlayAnaphoraSemanticFrame.toJSON(
              message.MusicPlayAnaphoraSemanticFrame
            )
          : undefined);
    message.MusicPlayFairytaleSemanticFrame !== undefined &&
      (obj.music_play_fairytale_semantic_frame =
        message.MusicPlayFairytaleSemanticFrame
          ? TMusicPlayFairytaleSemanticFrame.toJSON(
              message.MusicPlayFairytaleSemanticFrame
            )
          : undefined);
    message.StartMultiroomSemanticFrame !== undefined &&
      (obj.start_multiroom_semantic_frame = message.StartMultiroomSemanticFrame
        ? TStartMultiroomSemanticFrame.toJSON(
            message.StartMultiroomSemanticFrame
          )
        : undefined);
    message.PlayerWhatIsThisSongAboutSemanticFrame !== undefined &&
      (obj.player_what_is_this_song_about_semantic_frame =
        message.PlayerWhatIsThisSongAboutSemanticFrame
          ? TPlayerWhatIsThisSongAboutSemanticFrame.toJSON(
              message.PlayerWhatIsThisSongAboutSemanticFrame
            )
          : undefined);
    message.GuestEnrollmentFinishSemanticFrame !== undefined &&
      (obj.guest_enrollment_finish_semantic_frame =
        message.GuestEnrollmentFinishSemanticFrame
          ? TGuestEnrollmentFinishSemanticFrame.toJSON(
              message.GuestEnrollmentFinishSemanticFrame
            )
          : undefined);
    message.CentaurSetTeaserConfigurationSemanticFrame !== undefined &&
      (obj.centaur_set_teaser_configuration =
        message.CentaurSetTeaserConfigurationSemanticFrame
          ? TCentaurSetTeaserConfigurationSemanticFrame.toJSON(
              message.CentaurSetTeaserConfigurationSemanticFrame
            )
          : undefined);
    message.CentaurCollectTeasersPreviewSemanticFrame !== undefined &&
      (obj.centaur_collect_teasers_preview =
        message.CentaurCollectTeasersPreviewSemanticFrame
          ? TCentaurCollectTeasersPreviewSemanticFrame.toJSON(
              message.CentaurCollectTeasersPreviewSemanticFrame
            )
          : undefined);
    return obj;
  },
};

function createBaseTSemanticFrame(): TSemanticFrame {
  return { Name: "", Slots: [], TypedSemanticFrame: undefined };
}

export const TSemanticFrame = {
  encode(message: TSemanticFrame, writer: Writer = Writer.create()): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    for (const v of message.Slots) {
      TSemanticFrame_TSlot.encode(v!, writer.uint32(18).fork()).ldelim();
    }
    if (message.TypedSemanticFrame !== undefined) {
      TTypedSemanticFrame.encode(
        message.TypedSemanticFrame,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Slots.push(
            TSemanticFrame_TSlot.decode(reader, reader.uint32())
          );
          break;
        case 5:
          message.TypedSemanticFrame = TTypedSemanticFrame.decode(
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

  fromJSON(object: any): TSemanticFrame {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Slots: Array.isArray(object?.slots)
        ? object.slots.map((e: any) => TSemanticFrame_TSlot.fromJSON(e))
        : [],
      TypedSemanticFrame: isSet(object.typed_semantic_frame)
        ? TTypedSemanticFrame.fromJSON(object.typed_semantic_frame)
        : undefined,
    };
  },

  toJSON(message: TSemanticFrame): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    if (message.Slots) {
      obj.slots = message.Slots.map((e) =>
        e ? TSemanticFrame_TSlot.toJSON(e) : undefined
      );
    } else {
      obj.slots = [];
    }
    message.TypedSemanticFrame !== undefined &&
      (obj.typed_semantic_frame = message.TypedSemanticFrame
        ? TTypedSemanticFrame.toJSON(message.TypedSemanticFrame)
        : undefined);
    return obj;
  },
};

function createBaseTSemanticFrame_TSlot(): TSemanticFrame_TSlot {
  return {
    Name: "",
    Type: "",
    Value: "",
    AcceptedTypes: [],
    IsRequested: false,
    TypedValue: undefined,
    IsFilled: false,
  };
}

export const TSemanticFrame_TSlot = {
  encode(
    message: TSemanticFrame_TSlot,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Name !== "") {
      writer.uint32(10).string(message.Name);
    }
    if (message.Type !== "") {
      writer.uint32(18).string(message.Type);
    }
    if (message.Value !== "") {
      writer.uint32(26).string(message.Value);
    }
    for (const v of message.AcceptedTypes) {
      writer.uint32(34).string(v!);
    }
    if (message.IsRequested === true) {
      writer.uint32(40).bool(message.IsRequested);
    }
    if (message.TypedValue !== undefined) {
      TSlotValue.encode(message.TypedValue, writer.uint32(50).fork()).ldelim();
    }
    if (message.IsFilled === true) {
      writer.uint32(56).bool(message.IsFilled);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TSemanticFrame_TSlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSemanticFrame_TSlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Name = reader.string();
          break;
        case 2:
          message.Type = reader.string();
          break;
        case 3:
          message.Value = reader.string();
          break;
        case 4:
          message.AcceptedTypes.push(reader.string());
          break;
        case 5:
          message.IsRequested = reader.bool();
          break;
        case 6:
          message.TypedValue = TSlotValue.decode(reader, reader.uint32());
          break;
        case 7:
          message.IsFilled = reader.bool();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSemanticFrame_TSlot {
    return {
      Name: isSet(object.name) ? String(object.name) : "",
      Type: isSet(object.type) ? String(object.type) : "",
      Value: isSet(object.value) ? String(object.value) : "",
      AcceptedTypes: Array.isArray(object?.accepted_types)
        ? object.accepted_types.map((e: any) => String(e))
        : [],
      IsRequested: isSet(object.is_requested)
        ? Boolean(object.is_requested)
        : false,
      TypedValue: isSet(object.typed_value)
        ? TSlotValue.fromJSON(object.typed_value)
        : undefined,
      IsFilled: isSet(object.is_filled) ? Boolean(object.is_filled) : false,
    };
  },

  toJSON(message: TSemanticFrame_TSlot): unknown {
    const obj: any = {};
    message.Name !== undefined && (obj.name = message.Name);
    message.Type !== undefined && (obj.type = message.Type);
    message.Value !== undefined && (obj.value = message.Value);
    if (message.AcceptedTypes) {
      obj.accepted_types = message.AcceptedTypes.map((e) => e);
    } else {
      obj.accepted_types = [];
    }
    message.IsRequested !== undefined &&
      (obj.is_requested = message.IsRequested);
    message.TypedValue !== undefined &&
      (obj.typed_value = message.TypedValue
        ? TSlotValue.toJSON(message.TypedValue)
        : undefined);
    message.IsFilled !== undefined && (obj.is_filled = message.IsFilled);
    return obj;
  },
};

function createBaseTSemanticFrameRequestData(): TSemanticFrameRequestData {
  return {
    TypedSemanticFrame: undefined,
    Analytics: undefined,
    Origin: undefined,
    Params: undefined,
    RequestParams: undefined,
  };
}

export const TSemanticFrameRequestData = {
  encode(
    message: TSemanticFrameRequestData,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TypedSemanticFrame !== undefined) {
      TTypedSemanticFrame.encode(
        message.TypedSemanticFrame,
        writer.uint32(10).fork()
      ).ldelim();
    }
    if (message.Analytics !== undefined) {
      TAnalyticsTrackingModule.encode(
        message.Analytics,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Origin !== undefined) {
      TOrigin.encode(message.Origin, writer.uint32(26).fork()).ldelim();
    }
    if (message.Params !== undefined) {
      TFrameRequestParams.encode(
        message.Params,
        writer.uint32(34).fork()
      ).ldelim();
    }
    if (message.RequestParams !== undefined) {
      TRequestParams.encode(
        message.RequestParams,
        writer.uint32(42).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSemanticFrameRequestData {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSemanticFrameRequestData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TypedSemanticFrame = TTypedSemanticFrame.decode(
            reader,
            reader.uint32()
          );
          break;
        case 2:
          message.Analytics = TAnalyticsTrackingModule.decode(
            reader,
            reader.uint32()
          );
          break;
        case 3:
          message.Origin = TOrigin.decode(reader, reader.uint32());
          break;
        case 4:
          message.Params = TFrameRequestParams.decode(reader, reader.uint32());
          break;
        case 5:
          message.RequestParams = TRequestParams.decode(
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

  fromJSON(object: any): TSemanticFrameRequestData {
    return {
      TypedSemanticFrame: isSet(object.typed_semantic_frame)
        ? TTypedSemanticFrame.fromJSON(object.typed_semantic_frame)
        : undefined,
      Analytics: isSet(object.analytics)
        ? TAnalyticsTrackingModule.fromJSON(object.analytics)
        : undefined,
      Origin: isSet(object.origin)
        ? TOrigin.fromJSON(object.origin)
        : undefined,
      Params: isSet(object.params)
        ? TFrameRequestParams.fromJSON(object.params)
        : undefined,
      RequestParams: isSet(object.request_params)
        ? TRequestParams.fromJSON(object.request_params)
        : undefined,
    };
  },

  toJSON(message: TSemanticFrameRequestData): unknown {
    const obj: any = {};
    message.TypedSemanticFrame !== undefined &&
      (obj.typed_semantic_frame = message.TypedSemanticFrame
        ? TTypedSemanticFrame.toJSON(message.TypedSemanticFrame)
        : undefined);
    message.Analytics !== undefined &&
      (obj.analytics = message.Analytics
        ? TAnalyticsTrackingModule.toJSON(message.Analytics)
        : undefined);
    message.Origin !== undefined &&
      (obj.origin = message.Origin
        ? TOrigin.toJSON(message.Origin)
        : undefined);
    message.Params !== undefined &&
      (obj.params = message.Params
        ? TFrameRequestParams.toJSON(message.Params)
        : undefined);
    message.RequestParams !== undefined &&
      (obj.request_params = message.RequestParams
        ? TRequestParams.toJSON(message.RequestParams)
        : undefined);
    return obj;
  },
};

function createBaseTVideoCardDetailSemanticFrame(): TVideoCardDetailSemanticFrame {
  return {
    ContentId: undefined,
    ContentType: undefined,
    ContentOntoId: undefined,
  };
}

export const TVideoCardDetailSemanticFrame = {
  encode(
    message: TVideoCardDetailSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentId !== undefined) {
      TStringSlot.encode(message.ContentId, writer.uint32(10).fork()).ldelim();
    }
    if (message.ContentType !== undefined) {
      TStringSlot.encode(
        message.ContentType,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.ContentOntoId !== undefined) {
      TStringSlot.encode(
        message.ContentOntoId,
        writer.uint32(26).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoCardDetailSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoCardDetailSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.ContentType = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.ContentOntoId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoCardDetailSemanticFrame {
    return {
      ContentId: isSet(object.content_id)
        ? TStringSlot.fromJSON(object.content_id)
        : undefined,
      ContentType: isSet(object.content_type)
        ? TStringSlot.fromJSON(object.content_type)
        : undefined,
      ContentOntoId: isSet(object.onto_id)
        ? TStringSlot.fromJSON(object.onto_id)
        : undefined,
    };
  },

  toJSON(message: TVideoCardDetailSemanticFrame): unknown {
    const obj: any = {};
    message.ContentId !== undefined &&
      (obj.content_id = message.ContentId
        ? TStringSlot.toJSON(message.ContentId)
        : undefined);
    message.ContentType !== undefined &&
      (obj.content_type = message.ContentType
        ? TStringSlot.toJSON(message.ContentType)
        : undefined);
    message.ContentOntoId !== undefined &&
      (obj.onto_id = message.ContentOntoId
        ? TStringSlot.toJSON(message.ContentOntoId)
        : undefined);
    return obj;
  },
};

function createBaseTVideoThinCardDetailSemanticFrame(): TVideoThinCardDetailSemanticFrame {
  return { ContentId: undefined };
}

export const TVideoThinCardDetailSemanticFrame = {
  encode(
    message: TVideoThinCardDetailSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ContentId !== undefined) {
      TStringSlot.encode(message.ContentId, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TVideoThinCardDetailSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTVideoThinCardDetailSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ContentId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TVideoThinCardDetailSemanticFrame {
    return {
      ContentId: isSet(object.content_id)
        ? TStringSlot.fromJSON(object.content_id)
        : undefined,
    };
  },

  toJSON(message: TVideoThinCardDetailSemanticFrame): unknown {
    const obj: any = {};
    message.ContentId !== undefined &&
      (obj.content_id = message.ContentId
        ? TStringSlot.toJSON(message.ContentId)
        : undefined);
    return obj;
  },
};

function createBaseTGuestEnrollmentStartSemanticFrame(): TGuestEnrollmentStartSemanticFrame {
  return { Puid: undefined };
}

export const TGuestEnrollmentStartSemanticFrame = {
  encode(
    message: TGuestEnrollmentStartSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Puid !== undefined) {
      TStringSlot.encode(message.Puid, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGuestEnrollmentStartSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGuestEnrollmentStartSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Puid = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGuestEnrollmentStartSemanticFrame {
    return {
      Puid: isSet(object.puid) ? TStringSlot.fromJSON(object.puid) : undefined,
    };
  },

  toJSON(message: TGuestEnrollmentStartSemanticFrame): unknown {
    const obj: any = {};
    message.Puid !== undefined &&
      (obj.puid = message.Puid ? TStringSlot.toJSON(message.Puid) : undefined);
    return obj;
  },
};

function createBaseTGuestEnrollmentFinishSemanticFrame(): TGuestEnrollmentFinishSemanticFrame {
  return {};
}

export const TGuestEnrollmentFinishSemanticFrame = {
  encode(
    _: TGuestEnrollmentFinishSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGuestEnrollmentFinishSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGuestEnrollmentFinishSemanticFrame();
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

  fromJSON(_: any): TGuestEnrollmentFinishSemanticFrame {
    return {};
  },

  toJSON(_: TGuestEnrollmentFinishSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTTvPromoTemplateRequestSemanticFrame(): TTvPromoTemplateRequestSemanticFrame {
  return { ChosenTemplate: undefined };
}

export const TTvPromoTemplateRequestSemanticFrame = {
  encode(
    message: TTvPromoTemplateRequestSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ChosenTemplate !== undefined) {
      TTvChosenTemplateSlot.encode(
        message.ChosenTemplate,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTvPromoTemplateRequestSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTvPromoTemplateRequestSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.ChosenTemplate = TTvChosenTemplateSlot.decode(
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

  fromJSON(object: any): TTvPromoTemplateRequestSemanticFrame {
    return {
      ChosenTemplate: isSet(object.chosen_template)
        ? TTvChosenTemplateSlot.fromJSON(object.chosen_template)
        : undefined,
    };
  },

  toJSON(message: TTvPromoTemplateRequestSemanticFrame): unknown {
    const obj: any = {};
    message.ChosenTemplate !== undefined &&
      (obj.chosen_template = message.ChosenTemplate
        ? TTvChosenTemplateSlot.toJSON(message.ChosenTemplate)
        : undefined);
    return obj;
  },
};

function createBaseTTvPromoTemplateShownReportSemanticFrame(): TTvPromoTemplateShownReportSemanticFrame {
  return { ChosenTemplate: undefined };
}

export const TTvPromoTemplateShownReportSemanticFrame = {
  encode(
    message: TTvPromoTemplateShownReportSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.ChosenTemplate !== undefined) {
      TTvChosenTemplateSlot.encode(
        message.ChosenTemplate,
        writer.uint32(18).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTvPromoTemplateShownReportSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTvPromoTemplateShownReportSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 2:
          message.ChosenTemplate = TTvChosenTemplateSlot.decode(
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

  fromJSON(object: any): TTvPromoTemplateShownReportSemanticFrame {
    return {
      ChosenTemplate: isSet(object.chosen_template)
        ? TTvChosenTemplateSlot.fromJSON(object.chosen_template)
        : undefined,
    };
  },

  toJSON(message: TTvPromoTemplateShownReportSemanticFrame): unknown {
    const obj: any = {};
    message.ChosenTemplate !== undefined &&
      (obj.chosen_template = message.ChosenTemplate
        ? TTvChosenTemplateSlot.toJSON(message.ChosenTemplate)
        : undefined);
    return obj;
  },
};

function createBaseTGetVideoGalleriesSemanticFrame(): TGetVideoGalleriesSemanticFrame {
  return {
    CategoryId: undefined,
    MaxItemsPerGallery: undefined,
    Offset: undefined,
    Limit: undefined,
    CacheHash: undefined,
    FromScreenId: undefined,
    ParentFromScreenId: undefined,
    KidModeEnabled: undefined,
    RestrictionAge: undefined,
  };
}

export const TGetVideoGalleriesSemanticFrame = {
  encode(
    message: TGetVideoGalleriesSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.CategoryId !== undefined) {
      TStringSlot.encode(message.CategoryId, writer.uint32(10).fork()).ldelim();
    }
    if (message.MaxItemsPerGallery !== undefined) {
      TNumSlot.encode(
        message.MaxItemsPerGallery,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.Offset !== undefined) {
      TNumSlot.encode(message.Offset, writer.uint32(26).fork()).ldelim();
    }
    if (message.Limit !== undefined) {
      TNumSlot.encode(message.Limit, writer.uint32(34).fork()).ldelim();
    }
    if (message.CacheHash !== undefined) {
      TStringSlot.encode(message.CacheHash, writer.uint32(42).fork()).ldelim();
    }
    if (message.FromScreenId !== undefined) {
      TStringSlot.encode(
        message.FromScreenId,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.ParentFromScreenId !== undefined) {
      TStringSlot.encode(
        message.ParentFromScreenId,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.KidModeEnabled !== undefined) {
      TBoolSlot.encode(
        message.KidModeEnabled,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.RestrictionAge !== undefined) {
      TStringSlot.encode(
        message.RestrictionAge,
        writer.uint32(74).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetVideoGalleriesSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetVideoGalleriesSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CategoryId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.MaxItemsPerGallery = TNumSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Offset = TNumSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.Limit = TNumSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.CacheHash = TStringSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.FromScreenId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 7:
          message.ParentFromScreenId = TStringSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 8:
          message.KidModeEnabled = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 9:
          message.RestrictionAge = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetVideoGalleriesSemanticFrame {
    return {
      CategoryId: isSet(object.category_id)
        ? TStringSlot.fromJSON(object.category_id)
        : undefined,
      MaxItemsPerGallery: isSet(object.max_items_per_gallery)
        ? TNumSlot.fromJSON(object.max_items_per_gallery)
        : undefined,
      Offset: isSet(object.offset)
        ? TNumSlot.fromJSON(object.offset)
        : undefined,
      Limit: isSet(object.limit) ? TNumSlot.fromJSON(object.limit) : undefined,
      CacheHash: isSet(object.cache_hash)
        ? TStringSlot.fromJSON(object.cache_hash)
        : undefined,
      FromScreenId: isSet(object.from_screen_id)
        ? TStringSlot.fromJSON(object.from_screen_id)
        : undefined,
      ParentFromScreenId: isSet(object.parent_from_screen_id)
        ? TStringSlot.fromJSON(object.parent_from_screen_id)
        : undefined,
      KidModeEnabled: isSet(object.kid_mode_enabled)
        ? TBoolSlot.fromJSON(object.kid_mode_enabled)
        : undefined,
      RestrictionAge: isSet(object.restriction_age)
        ? TStringSlot.fromJSON(object.restriction_age)
        : undefined,
    };
  },

  toJSON(message: TGetVideoGalleriesSemanticFrame): unknown {
    const obj: any = {};
    message.CategoryId !== undefined &&
      (obj.category_id = message.CategoryId
        ? TStringSlot.toJSON(message.CategoryId)
        : undefined);
    message.MaxItemsPerGallery !== undefined &&
      (obj.max_items_per_gallery = message.MaxItemsPerGallery
        ? TNumSlot.toJSON(message.MaxItemsPerGallery)
        : undefined);
    message.Offset !== undefined &&
      (obj.offset = message.Offset
        ? TNumSlot.toJSON(message.Offset)
        : undefined);
    message.Limit !== undefined &&
      (obj.limit = message.Limit ? TNumSlot.toJSON(message.Limit) : undefined);
    message.CacheHash !== undefined &&
      (obj.cache_hash = message.CacheHash
        ? TStringSlot.toJSON(message.CacheHash)
        : undefined);
    message.FromScreenId !== undefined &&
      (obj.from_screen_id = message.FromScreenId
        ? TStringSlot.toJSON(message.FromScreenId)
        : undefined);
    message.ParentFromScreenId !== undefined &&
      (obj.parent_from_screen_id = message.ParentFromScreenId
        ? TStringSlot.toJSON(message.ParentFromScreenId)
        : undefined);
    message.KidModeEnabled !== undefined &&
      (obj.kid_mode_enabled = message.KidModeEnabled
        ? TBoolSlot.toJSON(message.KidModeEnabled)
        : undefined);
    message.RestrictionAge !== undefined &&
      (obj.restriction_age = message.RestrictionAge
        ? TStringSlot.toJSON(message.RestrictionAge)
        : undefined);
    return obj;
  },
};

function createBaseTGetVideoGallerySemanticFrame(): TGetVideoGallerySemanticFrame {
  return {
    Id: undefined,
    Offset: undefined,
    Limit: undefined,
    CacheHash: undefined,
    FromScreenId: undefined,
    ParentFromScreenId: undefined,
    CarouselPosition: undefined,
    CarouselTitle: undefined,
    KidModeEnabled: undefined,
    RestrictionAge: undefined,
    SelectedTags: undefined,
  };
}

export const TGetVideoGallerySemanticFrame = {
  encode(
    message: TGetVideoGallerySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Id !== undefined) {
      TStringSlot.encode(message.Id, writer.uint32(10).fork()).ldelim();
    }
    if (message.Offset !== undefined) {
      TNumSlot.encode(message.Offset, writer.uint32(18).fork()).ldelim();
    }
    if (message.Limit !== undefined) {
      TNumSlot.encode(message.Limit, writer.uint32(26).fork()).ldelim();
    }
    if (message.CacheHash !== undefined) {
      TStringSlot.encode(message.CacheHash, writer.uint32(34).fork()).ldelim();
    }
    if (message.FromScreenId !== undefined) {
      TStringSlot.encode(
        message.FromScreenId,
        writer.uint32(42).fork()
      ).ldelim();
    }
    if (message.ParentFromScreenId !== undefined) {
      TStringSlot.encode(
        message.ParentFromScreenId,
        writer.uint32(50).fork()
      ).ldelim();
    }
    if (message.CarouselPosition !== undefined) {
      TNumSlot.encode(
        message.CarouselPosition,
        writer.uint32(58).fork()
      ).ldelim();
    }
    if (message.CarouselTitle !== undefined) {
      TStringSlot.encode(
        message.CarouselTitle,
        writer.uint32(66).fork()
      ).ldelim();
    }
    if (message.KidModeEnabled !== undefined) {
      TBoolSlot.encode(
        message.KidModeEnabled,
        writer.uint32(74).fork()
      ).ldelim();
    }
    if (message.RestrictionAge !== undefined) {
      TStringSlot.encode(
        message.RestrictionAge,
        writer.uint32(82).fork()
      ).ldelim();
    }
    if (message.SelectedTags !== undefined) {
      TCatalogTagSlot.encode(
        message.SelectedTags,
        writer.uint32(90).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetVideoGallerySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetVideoGallerySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Id = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.Offset = TNumSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.Limit = TNumSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.CacheHash = TStringSlot.decode(reader, reader.uint32());
          break;
        case 5:
          message.FromScreenId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 6:
          message.ParentFromScreenId = TStringSlot.decode(
            reader,
            reader.uint32()
          );
          break;
        case 7:
          message.CarouselPosition = TNumSlot.decode(reader, reader.uint32());
          break;
        case 8:
          message.CarouselTitle = TStringSlot.decode(reader, reader.uint32());
          break;
        case 9:
          message.KidModeEnabled = TBoolSlot.decode(reader, reader.uint32());
          break;
        case 10:
          message.RestrictionAge = TStringSlot.decode(reader, reader.uint32());
          break;
        case 11:
          message.SelectedTags = TCatalogTagSlot.decode(
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

  fromJSON(object: any): TGetVideoGallerySemanticFrame {
    return {
      Id: isSet(object.id) ? TStringSlot.fromJSON(object.id) : undefined,
      Offset: isSet(object.offset)
        ? TNumSlot.fromJSON(object.offset)
        : undefined,
      Limit: isSet(object.limit) ? TNumSlot.fromJSON(object.limit) : undefined,
      CacheHash: isSet(object.cache_hash)
        ? TStringSlot.fromJSON(object.cache_hash)
        : undefined,
      FromScreenId: isSet(object.from_screen_id)
        ? TStringSlot.fromJSON(object.from_screen_id)
        : undefined,
      ParentFromScreenId: isSet(object.parent_from_screen_id)
        ? TStringSlot.fromJSON(object.parent_from_screen_id)
        : undefined,
      CarouselPosition: isSet(object.carousel_position)
        ? TNumSlot.fromJSON(object.carousel_position)
        : undefined,
      CarouselTitle: isSet(object.carousel_title)
        ? TStringSlot.fromJSON(object.carousel_title)
        : undefined,
      KidModeEnabled: isSet(object.kid_mode_enabled)
        ? TBoolSlot.fromJSON(object.kid_mode_enabled)
        : undefined,
      RestrictionAge: isSet(object.restriction_age)
        ? TStringSlot.fromJSON(object.restriction_age)
        : undefined,
      SelectedTags: isSet(object.selected_tags)
        ? TCatalogTagSlot.fromJSON(object.selected_tags)
        : undefined,
    };
  },

  toJSON(message: TGetVideoGallerySemanticFrame): unknown {
    const obj: any = {};
    message.Id !== undefined &&
      (obj.id = message.Id ? TStringSlot.toJSON(message.Id) : undefined);
    message.Offset !== undefined &&
      (obj.offset = message.Offset
        ? TNumSlot.toJSON(message.Offset)
        : undefined);
    message.Limit !== undefined &&
      (obj.limit = message.Limit ? TNumSlot.toJSON(message.Limit) : undefined);
    message.CacheHash !== undefined &&
      (obj.cache_hash = message.CacheHash
        ? TStringSlot.toJSON(message.CacheHash)
        : undefined);
    message.FromScreenId !== undefined &&
      (obj.from_screen_id = message.FromScreenId
        ? TStringSlot.toJSON(message.FromScreenId)
        : undefined);
    message.ParentFromScreenId !== undefined &&
      (obj.parent_from_screen_id = message.ParentFromScreenId
        ? TStringSlot.toJSON(message.ParentFromScreenId)
        : undefined);
    message.CarouselPosition !== undefined &&
      (obj.carousel_position = message.CarouselPosition
        ? TNumSlot.toJSON(message.CarouselPosition)
        : undefined);
    message.CarouselTitle !== undefined &&
      (obj.carousel_title = message.CarouselTitle
        ? TStringSlot.toJSON(message.CarouselTitle)
        : undefined);
    message.KidModeEnabled !== undefined &&
      (obj.kid_mode_enabled = message.KidModeEnabled
        ? TBoolSlot.toJSON(message.KidModeEnabled)
        : undefined);
    message.RestrictionAge !== undefined &&
      (obj.restriction_age = message.RestrictionAge
        ? TStringSlot.toJSON(message.RestrictionAge)
        : undefined);
    message.SelectedTags !== undefined &&
      (obj.selected_tags = message.SelectedTags
        ? TCatalogTagSlot.toJSON(message.SelectedTags)
        : undefined);
    return obj;
  },
};

function createBaseTTestSemanticFrame(): TTestSemanticFrame {
  return { Dummy: undefined };
}

export const TTestSemanticFrame = {
  encode(
    message: TTestSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Dummy !== undefined) {
      TStringSlot.encode(message.Dummy, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TTestSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTestSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Dummy = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TTestSemanticFrame {
    return {
      Dummy: isSet(object.dummy)
        ? TStringSlot.fromJSON(object.dummy)
        : undefined,
    };
  },

  toJSON(message: TTestSemanticFrame): unknown {
    const obj: any = {};
    message.Dummy !== undefined &&
      (obj.dummy = message.Dummy
        ? TStringSlot.toJSON(message.Dummy)
        : undefined);
    return obj;
  },
};

function createBaseTGetTvSearchResultSemanticFrame(): TGetTvSearchResultSemanticFrame {
  return {
    SearchText: undefined,
    RestrictionMode: undefined,
    RestrictionAge: undefined,
    SearchEntref: undefined,
  };
}

export const TGetTvSearchResultSemanticFrame = {
  encode(
    message: TGetTvSearchResultSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.SearchText !== undefined) {
      TStringSlot.encode(message.SearchText, writer.uint32(10).fork()).ldelim();
    }
    if (message.RestrictionMode !== undefined) {
      TStringSlot.encode(
        message.RestrictionMode,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.RestrictionAge !== undefined) {
      TStringSlot.encode(
        message.RestrictionAge,
        writer.uint32(26).fork()
      ).ldelim();
    }
    if (message.SearchEntref !== undefined) {
      TStringSlot.encode(
        message.SearchEntref,
        writer.uint32(34).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGetTvSearchResultSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGetTvSearchResultSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.SearchText = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.RestrictionMode = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.RestrictionAge = TStringSlot.decode(reader, reader.uint32());
          break;
        case 4:
          message.SearchEntref = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGetTvSearchResultSemanticFrame {
    return {
      SearchText: isSet(object.search_text)
        ? TStringSlot.fromJSON(object.search_text)
        : undefined,
      RestrictionMode: isSet(object.restriction_mode)
        ? TStringSlot.fromJSON(object.restriction_mode)
        : undefined,
      RestrictionAge: isSet(object.restriction_age)
        ? TStringSlot.fromJSON(object.restriction_age)
        : undefined,
      SearchEntref: isSet(object.search_entref)
        ? TStringSlot.fromJSON(object.search_entref)
        : undefined,
    };
  },

  toJSON(message: TGetTvSearchResultSemanticFrame): unknown {
    const obj: any = {};
    message.SearchText !== undefined &&
      (obj.search_text = message.SearchText
        ? TStringSlot.toJSON(message.SearchText)
        : undefined);
    message.RestrictionMode !== undefined &&
      (obj.restriction_mode = message.RestrictionMode
        ? TStringSlot.toJSON(message.RestrictionMode)
        : undefined);
    message.RestrictionAge !== undefined &&
      (obj.restriction_age = message.RestrictionAge
        ? TStringSlot.toJSON(message.RestrictionAge)
        : undefined);
    message.SearchEntref !== undefined &&
      (obj.search_entref = message.SearchEntref
        ? TStringSlot.toJSON(message.SearchEntref)
        : undefined);
    return obj;
  },
};

function createBaseTSwitchTvChannelSemanticFrame(): TSwitchTvChannelSemanticFrame {
  return { Uri: undefined };
}

export const TSwitchTvChannelSemanticFrame = {
  encode(
    message: TSwitchTvChannelSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Uri !== undefined) {
      TStringSlot.encode(message.Uri, writer.uint32(10).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TSwitchTvChannelSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTSwitchTvChannelSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Uri = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TSwitchTvChannelSemanticFrame {
    return {
      Uri: isSet(object.uri) ? TStringSlot.fromJSON(object.uri) : undefined,
    };
  },

  toJSON(message: TSwitchTvChannelSemanticFrame): unknown {
    const obj: any = {};
    message.Uri !== undefined &&
      (obj.uri = message.Uri ? TStringSlot.toJSON(message.Uri) : undefined);
    return obj;
  },
};

function createBaseTCurrencySlot(): TCurrencySlot {
  return { CurrencyValue: undefined };
}

export const TCurrencySlot = {
  encode(message: TCurrencySlot, writer: Writer = Writer.create()): Writer {
    if (message.CurrencyValue !== undefined) {
      writer.uint32(10).string(message.CurrencyValue);
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TCurrencySlot {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTCurrencySlot();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.CurrencyValue = reader.string();
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TCurrencySlot {
    return {
      CurrencyValue: isSet(object.currency_value)
        ? String(object.currency_value)
        : undefined,
    };
  },

  toJSON(message: TCurrencySlot): unknown {
    const obj: any = {};
    message.CurrencyValue !== undefined &&
      (obj.currency_value = message.CurrencyValue);
    return obj;
  },
};

function createBaseTConvertSemanticFrame(): TConvertSemanticFrame {
  return { TypeFrom: undefined, TypeTo: undefined, AmountFrom: undefined };
}

export const TConvertSemanticFrame = {
  encode(
    message: TConvertSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.TypeFrom !== undefined) {
      TCurrencySlot.encode(message.TypeFrom, writer.uint32(10).fork()).ldelim();
    }
    if (message.TypeTo !== undefined) {
      TCurrencySlot.encode(message.TypeTo, writer.uint32(18).fork()).ldelim();
    }
    if (message.AmountFrom !== undefined) {
      TNumSlot.encode(message.AmountFrom, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(input: Reader | Uint8Array, length?: number): TConvertSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTConvertSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.TypeFrom = TCurrencySlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.TypeTo = TCurrencySlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.AmountFrom = TNumSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TConvertSemanticFrame {
    return {
      TypeFrom: isSet(object.type_from)
        ? TCurrencySlot.fromJSON(object.type_from)
        : undefined,
      TypeTo: isSet(object.type_to)
        ? TCurrencySlot.fromJSON(object.type_to)
        : undefined,
      AmountFrom: isSet(object.amount_from)
        ? TNumSlot.fromJSON(object.amount_from)
        : undefined,
    };
  },

  toJSON(message: TConvertSemanticFrame): unknown {
    const obj: any = {};
    message.TypeFrom !== undefined &&
      (obj.type_from = message.TypeFrom
        ? TCurrencySlot.toJSON(message.TypeFrom)
        : undefined);
    message.TypeTo !== undefined &&
      (obj.type_to = message.TypeTo
        ? TCurrencySlot.toJSON(message.TypeTo)
        : undefined);
    message.AmountFrom !== undefined &&
      (obj.amount_from = message.AmountFrom
        ? TNumSlot.toJSON(message.AmountFrom)
        : undefined);
    return obj;
  },
};

function createBaseTMediaSessionPlaySemanticFrame(): TMediaSessionPlaySemanticFrame {
  return { MediaSessionId: undefined };
}

export const TMediaSessionPlaySemanticFrame = {
  encode(
    message: TMediaSessionPlaySemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.MediaSessionId !== undefined) {
      TStringSlot.encode(
        message.MediaSessionId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMediaSessionPlaySemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMediaSessionPlaySemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.MediaSessionId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMediaSessionPlaySemanticFrame {
    return {
      MediaSessionId: isSet(object.media_session_id)
        ? TStringSlot.fromJSON(object.media_session_id)
        : undefined,
    };
  },

  toJSON(message: TMediaSessionPlaySemanticFrame): unknown {
    const obj: any = {};
    message.MediaSessionId !== undefined &&
      (obj.media_session_id = message.MediaSessionId
        ? TStringSlot.toJSON(message.MediaSessionId)
        : undefined);
    return obj;
  },
};

function createBaseTMediaSessionPauseSemanticFrame(): TMediaSessionPauseSemanticFrame {
  return { MediaSessionId: undefined };
}

export const TMediaSessionPauseSemanticFrame = {
  encode(
    message: TMediaSessionPauseSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.MediaSessionId !== undefined) {
      TStringSlot.encode(
        message.MediaSessionId,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TMediaSessionPauseSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTMediaSessionPauseSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.MediaSessionId = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TMediaSessionPauseSemanticFrame {
    return {
      MediaSessionId: isSet(object.media_session_id)
        ? TStringSlot.fromJSON(object.media_session_id)
        : undefined,
    };
  },

  toJSON(message: TMediaSessionPauseSemanticFrame): unknown {
    const obj: any = {};
    message.MediaSessionId !== undefined &&
      (obj.media_session_id = message.MediaSessionId
        ? TStringSlot.toJSON(message.MediaSessionId)
        : undefined);
    return obj;
  },
};

function createBaseTOnboardingGetGreetingsSemanticFrame(): TOnboardingGetGreetingsSemanticFrame {
  return {};
}

export const TOnboardingGetGreetingsSemanticFrame = {
  encode(
    _: TOnboardingGetGreetingsSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOnboardingGetGreetingsSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnboardingGetGreetingsSemanticFrame();
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

  fromJSON(_: any): TOnboardingGetGreetingsSemanticFrame {
    return {};
  },

  toJSON(_: TOnboardingGetGreetingsSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTOnboardingWhatCanYouDoSemanticFrame(): TOnboardingWhatCanYouDoSemanticFrame {
  return { PhraseIndex: undefined };
}

export const TOnboardingWhatCanYouDoSemanticFrame = {
  encode(
    message: TOnboardingWhatCanYouDoSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.PhraseIndex !== undefined) {
      TUInt32Slot.encode(
        message.PhraseIndex,
        writer.uint32(10).fork()
      ).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TOnboardingWhatCanYouDoSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTOnboardingWhatCanYouDoSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.PhraseIndex = TUInt32Slot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TOnboardingWhatCanYouDoSemanticFrame {
    return {
      PhraseIndex: isSet(object.phrase_index)
        ? TUInt32Slot.fromJSON(object.phrase_index)
        : undefined,
    };
  },

  toJSON(message: TOnboardingWhatCanYouDoSemanticFrame): unknown {
    const obj: any = {};
    message.PhraseIndex !== undefined &&
      (obj.phrase_index = message.PhraseIndex
        ? TUInt32Slot.toJSON(message.PhraseIndex)
        : undefined);
    return obj;
  },
};

function createBaseTTvLongTapTutorialSemanticFrame(): TTvLongTapTutorialSemanticFrame {
  return {};
}

export const TTvLongTapTutorialSemanticFrame = {
  encode(
    _: TTvLongTapTutorialSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TTvLongTapTutorialSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTTvLongTapTutorialSemanticFrame();
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

  fromJSON(_: any): TTvLongTapTutorialSemanticFrame {
    return {};
  },

  toJSON(_: TTvLongTapTutorialSemanticFrame): unknown {
    const obj: any = {};
    return obj;
  },
};

function createBaseTGalleryVideoSelectSemanticFrame(): TGalleryVideoSelectSemanticFrame {
  return { Action: undefined, ProviderItemId: undefined, EmbedUri: undefined };
}

export const TGalleryVideoSelectSemanticFrame = {
  encode(
    message: TGalleryVideoSelectSemanticFrame,
    writer: Writer = Writer.create()
  ): Writer {
    if (message.Action !== undefined) {
      TStringSlot.encode(message.Action, writer.uint32(10).fork()).ldelim();
    }
    if (message.ProviderItemId !== undefined) {
      TStringSlot.encode(
        message.ProviderItemId,
        writer.uint32(18).fork()
      ).ldelim();
    }
    if (message.EmbedUri !== undefined) {
      TStringSlot.encode(message.EmbedUri, writer.uint32(26).fork()).ldelim();
    }
    return writer;
  },

  decode(
    input: Reader | Uint8Array,
    length?: number
  ): TGalleryVideoSelectSemanticFrame {
    const reader = input instanceof Reader ? input : new Reader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseTGalleryVideoSelectSemanticFrame();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1:
          message.Action = TStringSlot.decode(reader, reader.uint32());
          break;
        case 2:
          message.ProviderItemId = TStringSlot.decode(reader, reader.uint32());
          break;
        case 3:
          message.EmbedUri = TStringSlot.decode(reader, reader.uint32());
          break;
        default:
          reader.skipType(tag & 7);
          break;
      }
    }
    return message;
  },

  fromJSON(object: any): TGalleryVideoSelectSemanticFrame {
    return {
      Action: isSet(object.action)
        ? TStringSlot.fromJSON(object.action)
        : undefined,
      ProviderItemId: isSet(object.provider_item_id)
        ? TStringSlot.fromJSON(object.provider_item_id)
        : undefined,
      EmbedUri: isSet(object.embed_uri)
        ? TStringSlot.fromJSON(object.embed_uri)
        : undefined,
    };
  },

  toJSON(message: TGalleryVideoSelectSemanticFrame): unknown {
    const obj: any = {};
    message.Action !== undefined &&
      (obj.action = message.Action
        ? TStringSlot.toJSON(message.Action)
        : undefined);
    message.ProviderItemId !== undefined &&
      (obj.provider_item_id = message.ProviderItemId
        ? TStringSlot.toJSON(message.ProviderItemId)
        : undefined);
    message.EmbedUri !== undefined &&
      (obj.embed_uri = message.EmbedUri
        ? TStringSlot.toJSON(message.EmbedUri)
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

function isObject(value: any): boolean {
  return typeof value === "object" && value !== null;
}

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}
