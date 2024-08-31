import { renderMainScreenMusicCard } from './mainScreen/music';
import { renderMainScreenVideo } from './mainScreen/video';
import { renderMainScreenWebviewTab } from './mainScreen/webview';
import { renderMainScreenService } from './mainScreen/service';
import newsScenario from './news/scenario';
import newsTeaser from './news/teasers';
import SearchFact from './SearchFact/SearchFact';
import SearchObject from './SearchObject/SearchObject';
import {
    weatherRenderDayHours,
    weatherRenderDay,
    weatherRenderPartDay,
    weatherRenderDaysRange,
} from './weather/scenario';
import { weatherRenderTeaser } from './weather/teaser';
import { renderWebview } from './webview/webview';
import { TeaserChrome } from './TeaserChrome/TeaserChrome';
import { renderDiscoveryTab } from './mainScreen/discovery';
import { renderMainScreenServiceTab } from './mainScreen/servicesTab';
import { renderSmartHomeUpperShutter, renderUpperShutter } from './upperShutter/upperShutter';
import MainScreen2_0 from './mainScreen2_0';
import skills from './skills';
import { Music } from './Music/Music';
import screenSaverRender from './photoframe/screenSaver';
import WidgetChoosePanel from './widgetChoosePanel';
import { MainScreenMusicTab } from './MainScreenMusicTab/MainScreenMusicTab';
import { IDivPatchTemplates, Templates } from '../../index';
import Conversation from './Conversation/Conversation';
import AfishaTeaser from './Afisha/teasers';
import ChooseContact from './telegram/ChooseContact';
import IncomingCall from './telegram/IncomingCall';
import OutgoingCall from './telegram/OutgoingCall';
import CurrentCall from './telegram/CurrentCall';
import VideoSearch from './TvSearchResult/VideoSearch';
import SearchRichCard from './SearchRichCard/SearchRichCard';
import SkillTeaser from './skills/teasers/teasers';
import RouteResponse from './RouteResponse/RouteResponse';
import MainScreenDivPatch from './mainScreen2_0/MainScreenDivPatch';
import TeaserSettings from './teasers/settings_screen/ItemsListTeaser';
import ProactivityTeaser from './Proactivity/teasers';

export const templates: Templates = {
    SearchFactData: SearchFact,
    SearchObjectData: SearchObject,
    SearchRichCardData: SearchRichCard,
    NewsGalleryData: newsScenario,
    NewsTeaserData: newsTeaser,
    WeatherDayHoursData: weatherRenderDayHours,
    WeatherDayData: weatherRenderDay,
    WeatherDayPartData: weatherRenderPartDay,
    WeatherDaysRangeData: weatherRenderDaysRange,
    WeatherTeaserData: weatherRenderTeaser,
    CentaurMainScreenGalleryMusicCardData: renderMainScreenMusicCard,
    CentaurMainScreenGalleryVideoCardData: renderMainScreenVideo,
    CentaurMainScreenWebviewCardData: renderMainScreenService,
    CentaurMainScreenWebviewTabData: renderMainScreenWebviewTab,
    CentaurTeaserChromeDefaultLayerData: TeaserChrome,
    CentaurMainScreenDiscoveryTabData: renderDiscoveryTab,
    CentaurMainScreenMusicTabData: MainScreenMusicTab,
    CentaurMainScreenServicesTabData: renderMainScreenServiceTab,
    CentaurMainScreenSmartHomeTabData: renderSmartHomeUpperShutter,
    CentaurUpperShutterData: renderUpperShutter,
    CentaurWebviewData: renderWebview,
    MusicPlayerData: Music,
    DialogovoSkillCardData: skills,
    CentaurMainScreenMyScreenData: MainScreen2_0,
    ScreenSaverData: screenSaverRender,
    CentaurWidgetGalleryData: WidgetChoosePanel,
    ConversationData: Conversation,
    AfishaTeaserData: AfishaTeaser,
    VideoCallContactChoosingData: ChooseContact,
    IncomingTelegramCallData: IncomingCall,
    VideoSearchResultData: VideoSearch,
    OutgoingTelegramCallData: OutgoingCall,
    CurrentTelegramCallData: CurrentCall,
    DialogovoTeaserCardData: SkillTeaser,
    ShowRouteData: RouteResponse,
    TeaserSettingsWithContentData: TeaserSettings,
    ProactivityTeaserData: ProactivityTeaser,
};

export const templatesDivPatch: IDivPatchTemplates = {
    CentaurScenarioWidgetData: MainScreenDivPatch,
};

export { AdapterGetMusicDataFromTrack } from './Music/dataAdapter';
export { ITMusicPlayerData } from './Music/dataAdapter';
