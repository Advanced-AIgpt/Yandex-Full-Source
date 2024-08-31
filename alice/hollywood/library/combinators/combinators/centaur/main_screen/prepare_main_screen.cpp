#include "prepare_main_screen.h"

#include <alice/hollywood/library/combinators/combinators/centaur/schedule_service.h>

#include <util/generic/maybe.h>

#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/centaur/my_screen/widgets.pb.h>
#include <alice/protos/data/scenario/centaur/upper_shutter.pb.h>
#include <alice/protos/data/scenario/centaur/webview.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/music/infinite_feed.pb.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/wrappers.pb.h>

namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

TPrepareMainScreen::TPrepareMainScreen(THwServiceContext& ctx, const TCombinatorRequest& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest, Ctx.Logger(), Ctx),
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer),
      WidgetResponses(PrepareWidgetResponses(CombinatorContextWrapper))
{
}

void TPrepareMainScreen::Do() {
    CheckRequiredScenarios(CombinatorContextWrapper);
    SetMainScreenDirective();
    SetUpperShutterDirective();

    if (Request.HasExpFlag(MAIN_SCREEN_SERVER_UPDATE_EXP_FLAG_NAME)) {
        AddUpdateMainScreenScheduledAction();
    }

    Ctx.AddProtobufItemToApphostContext(ResponseForRenderer, RESPONSE_ITEM);
    PushUsedScenariosToContext();
}

void TPrepareMainScreen::AddUpdateMainScreenScheduledAction() {
    const auto& blackBoxProto = Request.BlackBoxUserInfo();
    if (!blackBoxProto.Defined()) {
        LOG_ERROR(Ctx.Logger()) << "Creating update main screen schedule action failed: no blackbox response";
        return;
    }
    const auto& userPuid = blackBoxProto->GetUserInfo().GetUid();
    const auto& clientInfoProto = Request.BaseRequestProto().GetClientInfo();
    auto updateAction = CreateUpdateMainScreenScheduleAction(clientInfoProto, userPuid);
    if (updateAction.Defined()) {
        LOG_INFO(Ctx.Logger()) << "Adding scheduled update action for centaur_collect_main_screen";
        *ResponseForRenderer.MutableResponseBody()->AddServerDirectives() = std::move(*updateAction);
    } else {
        LOG_ERROR(Ctx.Logger()) << "Creating update main screen schedule action";
    }
}

void TPrepareMainScreen::SetMainScreenDirective() {
    auto& mainScreenDirective = *ResponseForRenderer.MutableResponseBody()
                                    ->MutableLayout()
                                    ->AddDirectives()
                                    ->MutableSetMainScreenDirective();
    mainScreenDirective.SetName(MAIN_SCREEN_DIRECTIVE_NAME);

    const auto musicData = GetMusicScenarioData();

    AddMyScreenTab(mainScreenDirective, musicData);
    AddMusicTab(mainScreenDirective, musicData);
    AddSmartHomeTab(mainScreenDirective);
    if (Request.HasExpFlag(MAIN_SCREEN_SERVICES_TAB_ENABLE_EXP_FLAG_NAME)) {
        AddServicesTab(mainScreenDirective);
    }
    AddDiscoveryTab(mainScreenDirective);
}

void TPrepareMainScreen::AddMyScreenTab(TSetMainScreenDirective& mainScreenDirective, const TMaybe<TMusicData>& musicData) {
    auto& tabMyScreen = *mainScreenDirective.AddTabs();
    tabMyScreen.SetId(MY_SCREEN_TAB_ID);
    tabMyScreen.SetTitle(MY_SCREEN_TAB_TITLE);

    auto& cardsBlock = *tabMyScreen.AddBlocks();
    cardsBlock.SetId(MY_SCREEN_TAB_ID);

    auto& divBlock = *cardsBlock.MutableDivBlock();
    divBlock.SetCardId(MY_SCREEN_DIV_CARD_ID);

    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardName(MAIN_SCREEN_DIV_CARD_NAME);
    divRendererData.SetCardId(MY_SCREEN_DIV_CARD_ID);

    auto& myScreenData = *divRendererData.MutableScenarioData()->MutableCentaurMainScreenMyScreenData();
    myScreenData.SetId(MY_SCREEN_DIV_CARD_ID);

    const auto columns = GetCentaurWidgetDeviceConfig(CombinatorContextWrapper).GetColumns();

    for (int column = 0; column < columns.size(); column++) {
        const auto& widgets = columns[column].GetWidgetConfigs();
        auto& columnData = *myScreenData.AddColumns();
        for (int row = 0; row < widgets.size(); row++) {
            const auto& widgetConfigData = widgets[row];
            *columnData.AddWidgetCards() = MyScreenCard(widgetConfigData, column, row, WidgetResponses, CombinatorContextWrapper, musicData);
        }
    }

    for (const auto& response : Request.GetScenarioRunResponses()) {
        if (response.second.GetResponseBody().GetScenarioData().HasCentaurScenarioWidgetData()) {
            const auto& frameActions = response.second.GetResponseBody().GetFrameActions();
            (*ResponseForRenderer.MutableResponseBody()->MutableFrameActions()).insert(frameActions.begin(), frameActions.end());
        }
    }
    for (const auto& response : Request.GetScenarioContinueResponses()) {
        if (response.second.GetResponseBody().GetScenarioData().HasCentaurScenarioWidgetData()) {
            const auto& frameActions = response.second.GetResponseBody().GetFrameActions();
            (*ResponseForRenderer.MutableResponseBody()->MutableFrameActions()).insert(frameActions.begin(), frameActions.end());
        }
    }

    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

TMaybe<TIoTUserInfo> TPrepareMainScreen::GetIoTScenarioData() const {
    const auto& responses = Request.GetScenarioRunResponses();

    TStringBuf scenario = IOT_SCENARIO_NAME;

    LOG_INFO(Ctx.Logger()) << "Use scenario " << scenario << " as iot data source";

    const auto& scenarioResponse = responses.find(scenario);
    if (scenarioResponse == responses.end()) {
        LOG_ERROR(Ctx.Logger()) << "No " << scenario << " scenario response for main screen combinator";
        return Nothing();
    }

    const auto& scenarioData = scenarioResponse->second.GetResponseBody().GetScenarioData();
    if (!scenarioData.HasIoTUserData()) {
        LOG_ERROR(Ctx.Logger()) << scenario << " scenario data does not contain iot user info";
        return Nothing();
    }

    return scenarioData.GetIoTUserData();
}

void TPrepareMainScreen::AddSmartHomeTab(TSetMainScreenDirective& mainScreenDirective) {

    auto& tabServices = *mainScreenDirective.AddTabs();
    tabServices.SetId(SMART_HOME_TAB_ID);
    tabServices.SetTitle(SMART_HOME_TAB_TITLE);

    auto& servicesBlock = *tabServices.AddBlocks();
    servicesBlock.SetId(SMART_HOME_TAB_ID);

    auto& divBlock = *servicesBlock.MutableDivBlock();
    divBlock.SetCardId(SMART_HOME_DIV_CARD_ID);

    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(SMART_HOME_DIV_CARD_ID);
    if (Request.HasExpFlag(DIVVED_SMART_HOME_TAB_EXP_FLAG_NAME)) {
        // tmp place for experimental iot support in centaur - until iot team supports in its own scenario
        const auto iotUserInfo = GetIoTScenarioData();
        if (iotUserInfo.Empty()) {
            return;
        }
        auto& smartHomeTabData =
            *divRendererData.MutableScenarioData()->MutableCentaurMainScreenSmartHomeTabData();
        smartHomeTabData.SetId(SMART_HOME_DIV_CARD_ID);
        smartHomeTabData.MutableIoTUserData()->CopyFrom(*iotUserInfo);
    } else {
        auto& webviewCardData =
            *divRendererData.MutableScenarioData()->MutableCentaurWebviewData();
        webviewCardData.SetId(SMART_HOME_DIV_CARD_ID);
        webviewCardData.SetWebviewUrl(SMART_HOME_WEBVIEW_URL);
    }
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

void TPrepareMainScreen::AddServicesTab(TSetMainScreenDirective& mainScreenDirective) {
    auto& tabServices = *mainScreenDirective.AddTabs();
    tabServices.SetId(SERVICES_TAB_ID);
    tabServices.SetTitle(SERVICES_TAB_TITLE);

    auto& servicesBlock = *tabServices.AddBlocks();
    servicesBlock.SetId(SERVICES_TAB_ID);

    auto& divBlock = *servicesBlock.MutableDivBlock();
    divBlock.SetCardId(SERVICES_DIV_CARD_ID);
    PushServicesTabDataToContext(SERVICE_CARDS);
}

void TPrepareMainScreen::AddMusicTab(TSetMainScreenDirective& mainScreenDirective, const TMaybe<TMusicData>& musicData) {
    if (!musicData.Defined()) {
        return;
    }
    auto& tabMusic = *mainScreenDirective.AddTabs();
    tabMusic.SetId(MUSIC_TAB_ID);
    tabMusic.SetTitle(MUSIC_TAB_TITLE);

    auto& musicBlock = *tabMusic.AddBlocks();
    musicBlock.SetId(MUSIC_TAB_ID);

    auto& divBlock = *musicBlock.MutableDivBlock();
    divBlock.SetCardId(MUSIC_DIV_CARD_ID);
    PushMusicTabDataToContext(*musicData);
    return;
}

void TPrepareMainScreen::AddDiscoveryTab(TSetMainScreenDirective& mainScreenDirective) {
    auto& tabDiscovery = *mainScreenDirective.AddTabs();
    tabDiscovery.SetId(DISCOVERY_TAB_ID);
    tabDiscovery.SetTitle(DISCOVERY_TAB_TITLE);

    auto& discoveryBlock = *tabDiscovery.AddBlocks();
    discoveryBlock.SetId(DISCOVERY_TAB_ID);

    auto& divBlock = *discoveryBlock.MutableDivBlock();
    divBlock.SetCardId(DISCOVERY_DIV_CARD_ID);

    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(DISCOVERY_DIV_CARD_ID);
    auto& webviewCardData =
        *divRendererData.MutableScenarioData()->MutableCentaurMainScreenDiscoveryTabData();
    webviewCardData.SetId(DISCOVERY_DIV_CARD_ID);
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

const TMaybe<TMusicData> TPrepareMainScreen::GetMusicScenarioData() {
    const auto& runResponses = Request.GetScenarioRunResponses();
    const auto& continueResponses = Request.GetScenarioContinueResponses();

    NData::TScenarioData musicScenarioData;
    const auto& musicScenarioRunResponse = runResponses.find(MUSIC_SCENARIO_NAME);
    if (musicScenarioRunResponse != runResponses.end() &&
        musicScenarioRunResponse->second.HasResponseBody())
    {
        musicScenarioData = musicScenarioRunResponse->second.GetResponseBody().GetScenarioData();
    } else {
        const auto& musicContinueScenarioResponse = continueResponses.find(MUSIC_SCENARIO_NAME);
        if (musicContinueScenarioResponse != continueResponses.end() &&
            musicContinueScenarioResponse->second.HasResponseBody())
        {
            musicScenarioData = musicContinueScenarioResponse->second.GetResponseBody().GetScenarioData();
        } else {
            LOG_WARN(Ctx.Logger()) << "No music scenario response for main screen combinator";
            return Nothing();
        }
    }

    if (!musicScenarioData.HasMusicInfiniteFeedData()) {
        LOG_WARN(Ctx.Logger()) << "No infinite feed data in music scenario response";
        return Nothing();
    }

    UsedScenarios.insert(MUSIC_SCENARIO_NAME);
    TMusicData musicData;
    for (const auto& musicBlockType : MUSIC_BLOCK_TYPES_ORDER) {
        const auto* musicFeedBlock =
            FindIfPtr(musicScenarioData.GetMusicInfiniteFeedData().GetMusicObjectsBlocks(),
                        [&musicBlockType] (const auto& block) {
                            // cover special case when type is like inf_feed_tag_playlists-5ebeb05d501c1e482a6f926b
                            const auto& type = block.GetType();
                            return type == musicBlockType ||
                                    (type.StartsWith(musicBlockType) && type != "inf_feed_tag_playlists-for_kids");
                        });

        if (!musicFeedBlock) {
            LOG_INFO(Ctx.Logger()) << "Cannot get \"" << musicBlockType << "\" block in music feed";
            continue;
        }

        TMusicScenarioDataBlock musicDataBlock;
        musicDataBlock.Type = musicFeedBlock->GetType();
        musicDataBlock.Title = musicFeedBlock->GetTitle();
        TVector<TMusicScenarioData> musicBlockObjects;
        for (const auto& musicScenarioData : musicFeedBlock->GetMusicObjects()) {
            TMaybe<TMusicScenarioData> data;
            TFrameAction onClickAction;
            auto& parsedUtterance = *onClickAction.MutableParsedUtterance();

            auto* frame = parsedUtterance.MutableTypedSemanticFrame();
            auto musicPlayDirective = frame->MutableMusicPlaySemanticFrame();
            musicPlayDirective->MutableDisableNlg()->SetBoolValue(true);
            auto* analytics = parsedUtterance.MutableAnalytics();

            analytics->SetProductScenario(CENTAUR_MAIN_SCREEN_COMBINATOR_PSN);
            analytics->SetPurpose("music_play_from_centaur_main_screen");
            analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

            if (musicScenarioData.HasAutoPlaylist()) {
                musicPlayDirective->MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Playlist);
                musicPlayDirective->MutableObjectId()->SetStringValue(musicScenarioData.GetAutoPlaylist().GetUid() + ":" +
                                                                        musicScenarioData.GetAutoPlaylist().GetKind());
                data = TMusicScenarioData{
                    /* id= */ musicScenarioData.GetAutoPlaylist().GetKind(),
                    /* imageUrl= */ musicScenarioData.GetAutoPlaylist().GetImageUrl(),
                    /* type= */ "auto_playlist",
                    /* title= */ musicScenarioData.GetAutoPlaylist().GetTitle(),
                    /* modified= */ musicScenarioData.GetAutoPlaylist().GetModified()};
            } else if (musicScenarioData.HasPlaylist()) {
                musicPlayDirective->MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Playlist);
                musicPlayDirective->MutableObjectId()->SetStringValue(musicScenarioData.GetPlaylist().GetUid() + ":" +
                                                                        musicScenarioData.GetPlaylist().GetKind());
                data = TMusicScenarioData{
                    /* id= */ musicScenarioData.GetPlaylist().GetKind(),
                    /* imageUrl= */ musicScenarioData.GetPlaylist().GetImageUrl(),
                    /* type= */ "playlist",
                    /* title= */ musicScenarioData.GetPlaylist().GetTitle(),
                    /* modified= */ musicScenarioData.GetPlaylist().GetModified(),
                    /* likes_count= */ musicScenarioData.GetPlaylist().GetLikesCount()};
            } else if (musicScenarioData.HasArtist()) {
                musicPlayDirective->MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Artist);
                musicPlayDirective->MutableObjectId()->SetStringValue(musicScenarioData.GetArtist().GetId());
                data = TMusicScenarioData{
                    /* id= */ musicScenarioData.GetArtist().GetId(),
                    /* imageUrl= */ musicScenarioData.GetArtist().GetImageUrl(),
                    /* type= */ "artist",
                    /* title= */ musicScenarioData.GetArtist().GetName()};
                for (const auto& genre : musicScenarioData.GetArtist().GetGenres()) {
                    data->Genres.push_back(genre);
                }
            } else if (musicScenarioData.HasAlbum()) {
                musicPlayDirective->MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Album);
                musicPlayDirective->MutableObjectId()->SetStringValue(musicScenarioData.GetAlbum().GetId());
                data = TMusicScenarioData{
                    /* id= */ musicScenarioData.GetAlbum().GetId(),
                    /* imageUrl= */ musicScenarioData.GetAlbum().GetImageUrl(),
                    /* type= */ "album",
                    /* title= */ musicScenarioData.GetAlbum().GetTitle()};
                data->ReleaseDate = musicScenarioData.GetAlbum().GetReleaseDate();
                data->LikesCount = musicScenarioData.GetAlbum().GetLikesCount();
                for (const auto& artist : musicScenarioData.GetAlbum().GetArtists()) {
                    data->Artists.push_back({artist.GetId(), artist.GetName()});
                }
            }

            if (data.Defined()) {
                const auto frameActionId = "OnClickMainScreenMusicTab_" + data->Id;
                (*ResponseForRenderer.MutableResponseBody()->MutableFrameActions())[frameActionId] = onClickAction;
                data->Action = "@@mm_deeplink#" + frameActionId;
                google::protobuf::Any typedAction;
                typedAction.PackFrom(std::move(*frame));
                data->TypedAction = typedAction;
                data->BlockType = musicFeedBlock->GetType();
                if (data->ImgUrl.Contains("%%")) {
                    data->ImgUrl.replace(data->ImgUrl.find("%%"), 2, "460x460"); // should be in renderer
                }
                data->ImgUrl = "https://" + data->ImgUrl;
                musicDataBlock.MusicScenarioData.push_back(*data);
            }
        }
        musicData.musicDataBlocks.push_back(musicDataBlock);
    }
    return musicData;
}

void TPrepareMainScreen::SetUpperShutterDirective() {
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(UPPER_SHUTTER_DIV_CARD_ID);

    const auto iotUserInfo = GetIoTScenarioData();
    if (iotUserInfo.Empty()) {
        return;
    }

    auto& upperShutterData =
        *divRendererData.MutableScenarioData()->MutableCentaurUpperShutterData();
    upperShutterData.SetId(UPPER_SHUTTER_DIV_CARD_ID);
    auto& smartHomeData = *upperShutterData.MutableSmartHomeData();
    smartHomeData.MutableIoTUserData()->CopyFrom(*iotUserInfo);

    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);

    auto& upperShutterDirective = *ResponseForRenderer.MutableResponseBody()
                                    ->MutableLayout()
                                    ->AddDirectives()
                                    ->MutableSetUpperShutterDirective();
    upperShutterDirective.SetName(UPPER_SHUTTER_DIRECTIVE_NAME);
    upperShutterDirective.SetCardId(UPPER_SHUTTER_DIV_CARD_ID);
}

void TPrepareMainScreen::PushMusicTabDataToContext(const TMusicData& musicData) {
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(MUSIC_DIV_CARD_ID);
    auto& musicTabCardData =
        *divRendererData.MutableScenarioData()->MutableCentaurMainScreenMusicTabData();
    musicTabCardData.SetId(MUSIC_DIV_CARD_ID);


    for (const auto& musicFeedBlock : musicData.musicDataBlocks) {
        musicTabCardData.SetId(MUSIC_DIV_CARD_ID);
        auto& musicBlockData = *musicTabCardData.AddHorizontalMusicBlockData();
        musicBlockData.SetTitle(musicFeedBlock.Title);
        musicBlockData.SetType(musicFeedBlock.Type);
        for (const auto& data : musicFeedBlock.MusicScenarioData) {
            auto& cardData = *musicBlockData.AddCentaurMainScreenGalleryMusicCardData();
            FillMusicCardData(data, cardData);
        }
    }
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

void TPrepareMainScreen::FillMusicCardData(const auto& scenarioData, NData::TCentaurMainScreenGalleryMusicCardData& musicCardData) {
    musicCardData.SetId(scenarioData.Id);
    musicCardData.SetImageUrl(scenarioData.ImgUrl);
    musicCardData.SetAction(scenarioData.Action);
    if (scenarioData.TypedAction.Defined()) {
        // should be TTypedSemanticframe
        *musicCardData.MutableTypedAction() = *scenarioData.TypedAction;
    }
    musicCardData.SetType(scenarioData.Type);
    musicCardData.SetTitle(scenarioData.Title);
    if (scenarioData.Modified.Defined()) {
        musicCardData.MutableModified()->set_value(*scenarioData.Modified);
    }
    if (scenarioData.LikesCount.Defined()) {
        musicCardData.MutableLikesCount()->set_value(*scenarioData.LikesCount);
    }
    for (const auto& genre : scenarioData.Genres) {
        *musicCardData.AddGenres() = genre;
    }
    if (scenarioData.ReleaseDate.Defined()) {
        musicCardData.MutableReleaseDate()->set_value(*scenarioData.ReleaseDate);
    }
    for (const auto& artistInfo : scenarioData.Artists) {
        auto& artist = *musicCardData.AddArtists();
        artist.SetId(artistInfo.Id);
        artist.SetName(artistInfo.Name);
    }
}

void TPrepareMainScreen::PushMusicCardDataToContext(const auto& scenarioData) {
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(scenarioData.Id);
    auto& musicCardData =
        *divRendererData.MutableScenarioData()->MutableCentaurMainScreenGalleryMusicCardData();
    FillMusicCardData(scenarioData, musicCardData);
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

void TPrepareMainScreen::PushVideoCardDataToContext(const auto& scenarioData) {
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(scenarioData.Id);
    auto& videoCardData =
        *divRendererData.MutableScenarioData()->MutableCentaurMainScreenGalleryVideoCardData();
    videoCardData.SetId(scenarioData.Id);
    videoCardData.SetImageUrl(scenarioData.ImgUrl);
    videoCardData.SetAction(scenarioData.Action);
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

void TPrepareMainScreen::PushServicesCardDataToContext(const auto& scenarioData) {
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(scenarioData.Id);
    auto& serviceCardData =
        *divRendererData.MutableScenarioData()->MutableCentaurMainScreenWebviewCardData();
    serviceCardData.SetId(scenarioData.Id);
    serviceCardData.SetImageUrl(scenarioData.ImgUrl);
    serviceCardData.SetWebviewUrl(scenarioData.WebviewUrl);
    serviceCardData.SetTitle(scenarioData.Text);
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

void TPrepareMainScreen::PushServicesTabDataToContext(const TVector<TServiceData>& servicesData) {
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(SERVICES_DIV_CARD_ID);
    auto& servicesTabCardData =
        *divRendererData.MutableScenarioData()->MutableCentaurMainScreenServicesTabData();
    servicesTabCardData.SetId(SERVICES_DIV_CARD_ID);

    for (const auto& service : servicesData) {
        auto& serviceCardData = *servicesTabCardData.AddCentaurMainScreenWebviewCardData();
        serviceCardData.SetId(service.Id);
        serviceCardData.SetImageUrl(service.ImgUrl);
        serviceCardData.SetWebviewUrl(service.WebviewUrl);
        serviceCardData.SetTitle(service.Text);
    }

    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}

void TPrepareMainScreen::PushUsedScenariosToContext() {
    TCombinatorUsedScenarios usedScenariosProto;
    for (const auto& usedScenario : UsedScenarios) {
        usedScenariosProto.AddScenarioNames(usedScenario);
    }
    Ctx.AddProtobufItemToApphostContext(usedScenariosProto, COMBINATOR_USED_SCENARIOS_ITEM);
}

} // namespace NAlice::NHollywood::NCombinators
