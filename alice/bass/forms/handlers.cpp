#include "handlers.h"

#include <alice/bass/clients.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/forms/registrator.h>
#include <alice/bass/forms/vins.h>

// forms
#include <alice/bass/forms/automotive/automotive_demo_2019.h>
#include <alice/bass/forms/poi.h>
#include <alice/bass/forms/remember_address.h>
#include <alice/bass/forms/route.h>
#include <alice/bass/forms/automotive/greeting.h>
#include <alice/bass/forms/avia.h>
#include <alice/bass/forms/bluetooth.h>
#include <alice/bass/forms/browser_read_page.h>
#include <alice/bass/forms/battery_power_state.h>
#include <alice/bass/forms/client_command.h>
#include <alice/bass/forms/computer_vision/handler.h>
#include <alice/bass/forms/computer_vision/barcode_handler.h>
#include <alice/bass/forms/computer_vision/translate_handler.h>
#include <alice/bass/forms/computer_vision/info_cv_handler.h>
#include <alice/bass/forms/computer_vision/market_handler.h>
#include <alice/bass/forms/computer_vision/clothes_handler.h>
#include <alice/bass/forms/computer_vision/ocr_handler.h>
#include <alice/bass/forms/computer_vision/ocr_voice_handler.h>
#include <alice/bass/forms/computer_vision/office_lens_handler.h>
#include <alice/bass/forms/computer_vision/similarlike_cv_handler.h>
#include <alice/bass/forms/computer_vision/similar_artwork_handler.h>
#include <alice/bass/forms/convert.h>
#include <alice/bass/forms/crmbot/handlers/search_handler.h>
#include <alice/bass/forms/crmbot/handlers/order_cancel_handler.h>
#include <alice/bass/forms/crmbot/handlers/order_cancel_we_did_handler.h>
#include <alice/bass/forms/crmbot/handlers/order_status_handler.h>
#include <alice/bass/forms/external_skill.h>
#include <alice/bass/forms/external_skill_recommendation/skill_recommendation.h>
#include <alice/bass/forms/facts.h>
#include <alice/bass/forms/fairy_tales.h>
#include <alice/bass/forms/feedback.h>
#include <alice/bass/forms/games_onboarding.h>
#include <alice/bass/forms/get_location.h>
#include <alice/bass/forms/general_conversation/general_conversation.h>
#include <alice/bass/forms/general_conversation/general_conversation_vinsless.h>
#include <alice/bass/forms/getdate.h>
#include <alice/bass/forms/gettime.h>
#include <alice/bass/forms/happy_new_year.h>
#include <alice/bass/forms/market.h>
#include <alice/bass/forms/music/ambient_sound.h>
#include <alice/bass/forms/music/music.h>
#include <alice/bass/forms/music/sing_song.h>
#include <alice/bass/forms/navigation/navigation.h>
#include <alice/bass/forms/navigator/add_point.h>
#include <alice/bass/forms/navigator/faster_route.h>
#include <alice/bass/forms/navigator/how_long.h>
#include <alice/bass/forms/navigator/navi_onboarding.h>
#include <alice/bass/forms/navigator/simple_intents.h>
#include "alice/bass/forms/navigator/refuel.h"
#include <alice/bass/forms/news/news.h>
#include <alice/bass/forms/no_op.h>
#include <alice/bass/forms/onboarding.h>
#include <alice/bass/forms/phone_call/phone_call.h>
#include <alice/bass/forms/player_command.h>
#include <alice/bass/forms/radio.h>
#include <alice/bass/forms/random_number.h>
#include <alice/bass/forms/reminders/registrator.h>
#include <alice/bass/forms/search/film_gallery.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/search/serp_gallery.h>
#include <alice/bass/forms/send_bugreport.h>
#include <alice/bass/forms/session_start.h>
#include <alice/bass/forms/shazam.h>
#include <alice/bass/forms/show_collection.h>
#include <alice/bass/forms/search_filter.h>
#include <alice/bass/forms/sound.h>
#include <alice/bass/forms/taxi.h>
#include <alice/bass/forms/taxi/handler.h>
#include <alice/bass/forms/thereminvox.h>
#include <alice/bass/forms/ether.h>
#include <alice/bass/forms/ether/video.h>
#include <alice/bass/forms/traffic.h>
#include <alice/bass/forms/translate/translate.h>
#include <alice/bass/forms/tv/switch_input.h>
#include <alice/bass/forms/tv/tv_broadcast.h>
#include <alice/bass/forms/tv/tv_next_episode.h>
#include <alice/bass/forms/unit_test_form.h>
#include <alice/bass/forms/video/video.h>
#include <alice/bass/forms/voiceprint/voiceprint_enroll.h>
#include <alice/bass/forms/voiceprint/voiceprint_remove.h>
#include <alice/bass/forms/weather/weather.h>
#include <alice/bass/forms/weather/weather_nowcast.h>
#include <alice/bass/forms/whats_new.h>
#include <alice/bass/forms/messaging.h>

namespace NBASS {
namespace {

void FillInAliceFormHandlers(THandlersMap& handlers, IGlobalContext& globalCtx) {
    TAmbientSoundHandler::Register(&handlers);
    TAutomotiveDemo2019Handler::Register(&handlers);
    TAviaFormHandler::Register(&handlers, globalCtx);
    TBatteryPowerStateFormHandler::Register(&handlers);
    TFactsHandler::Register(&handlers);
    TRandomNumberFormHandler::Register(&handlers);
    TWeatherFormHandler::Register(&handlers);
    TWeatherNowcastFormHandler::Register(&handlers);
    TTvBroadcastFormHandler::Register(&handlers);

    TPoiFormHandler::Register(&handlers);
    TRouteFormHandler::Register(&handlers);
    TSearchFormHandler::Register(&handlers, globalCtx);
    Register(&handlers, NSerpGallery::FORM_HANDLER_PAIRS);
    Register(&handlers, NFilmGallery::FORM_HANDLER_PAIRS);
    THappyNewYearHandler::Register(&handlers);
    TTimeFormHandler::Register(&handlers);
    TDateFormHandler::Register(&handlers);
    TNoOpFormHandler::Register(&handlers);
    TConvertFormHandler::Register(&handlers);
    TTrafficFormHandler::Register(&handlers, globalCtx);
    NNews::TNewsFormHandler::Register(&handlers);
    TExternalSkillHandler::RegisterForm(&handlers, globalCtx);
    TSessionStartFormHandler::Register(&handlers);
    TFeedbackFormHandler::Register(&handlers);
    TGeneralConversationFormHandler::Register(&handlers);
    TGeneralConversationVinslessFormHandler::Register(&handlers);
    TGetMyLocationHandler::Register(&handlers);
    TSaveAddressHandler::Register(&handlers);
    NMusic::TSearchMusicHandler::Register(&handlers);
    NMusic::TMusicAnaphoraHandler::Register(&handlers);
    TFairyTalesHandler::Register(&handlers);
    TSearchFilterFormHandler::Register(&handlers);
    TSoundFormHandler::Register(&handlers);
    TRadioFormHandler::Register(&handlers);
    TPlayerSimpleCommandHandler::Register(&handlers);
    TPlayerAuthorizedCommandHandler::Register(&handlers);
    TPlayerContinueCommandHandler::Register(&handlers);
    TPlayerNextPrevCommandHandler::Register(&handlers);
    TPlayerRewindCommandHandler::Register(&handlers);
    TClientCommandHandler::Register(&handlers);
    TOnboardingHandler::Register(&handlers);
    TOnboardingCancelHandler::Register(&handlers);
    TOnboardingNextHandler::Register(&handlers);
    TGamesOnboardingHandler::Register(&handlers);
    TVideoSearchFormHandler::Register(&handlers);
    TSelectVideoFromGalleryHandler::Register(&handlers);
    TThereminvoxHandler::Register(&handlers);
    TEtherHandler::Register(&handlers);
    NEther::TVideoHandler::Register(&handlers);
    TVideoPaymentConfirmedHandler::Register(&handlers);
    TOpenCurrentVideoHandler::Register(&handlers);
    TOpenCurrentTrailerHandler::Register(&handlers);
    TVideoGoToScreenFormHandler::Register(&handlers);
    TTaxiOrderHandler::Register(&handlers);
    NTaxi::THandler::Register(&handlers);
    TMarketChoiceFormHandler::Register(&handlers, globalCtx);
    TMarketHowMuchFormHandler::Register(&handlers, globalCtx);
    TMarketOrdersStatusFormHandler::Register(&handlers);
    TMarketRecurringPurchaseFormHandler::Register(&handlers, globalCtx);
    TMarketBeruMyBonusesListFormHandler::Register(&handlers);
    TShoppingListFormHandler::Register(&handlers);
    TNavigationFormHandler::Register(&handlers);
    TPhoneCallHandler::Register(&handlers);
    TSendBugreportFormHandler::Register(&handlers);
    TSingSongFormHandler::Register(&handlers);
    Register(&handlers, NReminders::FORM_HANDLER_PAIRS);
    TShazamHandler::Register(&handlers);
    TShowCollectionHandler::Register(&handlers);
    TComputerVisionMarketHandler::Register(&handlers);
    TComputerVisionClothesHandler::Register(&handlers);
    TComputerVisionSimilarHandler::Register(&handlers);
    TComputerVisionOcrHandler::Register(&handlers);
    TComputerVisionMainHandler::Register(&handlers);
    TComputerVisionBarcodeHandler::Register(&handlers);
    TComputerVisionTranslateHandler::Register(&handlers);
    TComputerVisionOnboardingHandler::Register(&handlers);
    TComputerVisionOcrVoiceHandler::Register(&handlers);
    TComputerVisionOcrVoiceSuggestHandler::Register(&handlers);
    TComputerVisionEllipsisOcrVoiceHandler::Register(&handlers);
    TComputerVisionOfficeLensHandler::Register(&handlers);
    TComputerVisionOfficeLensDiskHandler::Register(&handlers);
    TComputerVisionClothesBoxHandler::Register(&handlers);
    TComputerVisionEllipsisClothesHandler::Register(&handlers);
    TComputerVisionEllipsisDetailsHandler::Register(&handlers);
    TComputerVisionEllipsisMarketHandler::Register(&handlers);
    TComputerVisionEllipsisOcrHandler::Register(&handlers);
    TComputerVisionNegativeFeedbackHandler::Register(&handlers);
    TComputerVisionFrontalSimilarPeople::Register(&handlers);
    TComputerVisionEllipsisInfoHandler::Register(&handlers);
    TComputerVisionEllipsisSimilarLikeHandler::Register(&handlers);
    TComputerVisionSimilarPeople::Register(&handlers);
    TComputerVisionEllipsisSimilarPeopleHandler::Register(&handlers);
    TComputerVisionSimilarArtworkHandler::Register(&handlers);
    TComputerVisionEllipsisSimilarArtworkHandler::Register(&handlers);
    TNavigatorHowLongHandler::Register(&handlers);
    TNavigatorOnboardingHandler::Register(&handlers);
    TNavigatorSimpleIntentsHandler::Register(&handlers);
    TNavigatorAddPointHandler::Register(&handlers);
    TNavigatorFasterRouteHandler::Register(&handlers);
    TUnitTestFormHandler::Register(&handlers);
    TWhatsNewHandler::Register(&handlers);
    TNavigatorRefuelHandler::Register(&handlers);
    TBluetoothHandler::Register(&handlers);
    TAutomotiveGreetingFormHandler::Register(&handlers);
    NExternalSkill::TSkillRecommendationInitializer::Register(&handlers);
    RegisterVoiceprintEnrollHandlers(handlers);
    RegisterVoiceprintRemoveHandlers(handlers);
    TTranslateFormHandler::Register(&handlers);
    TMessagingFormHandler::Register(&handlers);
    TBrowserReadPageFormHandler::Register(&handlers);
    TTvSwitchInputFormHandler::Register(&handlers);
    TVideoRecommendationHandler::Register(&handlers);
    TShowVideoSettingsHandler::Register(&handlers);
    TSkipVideoFragmentHandler::Register(&handlers);
    TChangeTrackHandler::Register(&handlers);
    TVideoHowLongHandler::Register(&handlers);
    TVideoFinishedTrackHandler::Register(&handlers);
}

void FillInCrmbotFormHandlers(THandlersMap& handlers) {
    NCrmbot::TSearchHandler::Register(&handlers);
    NCrmbot::TOrderStatusHandler::Register(&handlers);
    NCrmbot::TOrderStatusContinuationHandler::Register(&handlers);
    NCrmbot::TOrderStatusPickupWhereHandler::Register(&handlers);
    NCrmbot::TOrderCancelHandler::Register(&handlers);
    NCrmbot::TOrderCancelReasonHandler::Register(&handlers);
    NCrmbot::TOrderCancelFinishHandler::Register(&handlers);
    NCrmbot::TOrderCancelWeDidHandler::Register(&handlers);
    NCrmbot::TOrderCancelWeDidContinuationHandler::Register(&handlers);
}

void FillInFormHandlers(THandlersMap& handlers, IGlobalContext& globalCtx) {
    switch (globalCtx.Config().BASSClient()) {
        case EBASSClient::Alice:
        case EBASSClient::DevNoMusic:
            FillInAliceFormHandlers(handlers, globalCtx);
            break;
        case EBASSClient::Crmbot:
            FillInCrmbotFormHandlers(handlers);
            break;
    }
}

void FillInAliceActionHandlers(THandlersMap& handlers) {
    TNextVideoTrackHandler::Register(&handlers);
    TPlayVideoActionHandler::Register(&handlers);
    TPlayVideoFromDescriptorActionHandler::Register(&handlers);
    NMusic::TMusicPlayObjectActionHandler::Register(&handlers);
    TRadioPlayObjectActionHandler::Register(&handlers);
    TNextTvEpisodeActionHandler::Register(&handlers);
    TExternalSkillHandler::RegisterAction(&handlers);
}

void FillInActionHandlers(THandlersMap& handlers, IGlobalContext& globalCtx) {
    switch (globalCtx.Config().BASSClient()) {
        case EBASSClient::Alice:
        case EBASSClient::DevNoMusic:
            FillInAliceActionHandlers(handlers);
            break;
        case EBASSClient::Crmbot:
            break;
    }
}

} // namespace

void RegisterHandlers(THandlersMap& handlers, IGlobalContext& globalCtx) {
    FillInFormHandlers(handlers, globalCtx);
    FillInActionHandlers(handlers, globalCtx);
}

} // namespace NBASS
