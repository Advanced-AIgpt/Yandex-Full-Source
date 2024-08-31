#include "video.h"

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/tv/channels_info.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/util/error.h>
#include <alice/bass/forms/tv/tv_helper.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/forms/video/video.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/library/network/headers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/video_common/video_helper.h>

#include <library/cpp/scheme/scheme.h>

namespace {

    constexpr TStringBuf VIDEO_SELECT = "personal_assistant.scenarios.ether.quasar.video_select";
    constexpr TStringBuf SERIAL_TYPE = "tv_show";
    constexpr TStringBuf MOVIE_TYPE = "movie";
    constexpr TStringBuf VIDEO_TYPE = "video";

    bool MakeRequestToPlayerHandler(const NBASS::TContext& context, const TStringBuf itemId,  NSc::TValue& data)
    {
        NHttpFetcher::TRequestPtr request = context.GetSources().VideoHostingPlayer(TStringBuilder() << itemId << ".json").Request();
        NBASS::NVideo::AddCodecHeadersIntoRequest(request, context);
        NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();
        if (response->IsHttpOk()) {
            data = NSc::TValue::FromJson(response->Data);
            return true;
        }

        return false;
    }

    NBASS::NVideo::TLightVideoItem CreateProviderInfo(
        const TStringBuf itemId,
        const TStringBuf providerName,
        const TStringBuf itemType)
    {
        NBASS::NVideo::TLightVideoItem providerInfo{};

        providerInfo->ProviderName() = providerName;
        providerInfo->ProviderItemId() = itemId;
        providerInfo->Type() = itemType;
        providerInfo->Available() = true;

        return providerInfo;
    }

    void UpdateVideoItemFromPlayerHandlerResponse(
        NBASS::NVideo::TVideoItem& videoItem,
        const NSc::TValue& response
    ) {
        const auto& content = response["content"];
        if (!content.IsDict()) {
            return;
        }

        const auto& thumbnail = content["thumbnail"];
        if (thumbnail.IsString()) {
            videoItem->CoverUrl16X9() = NAlice::NVideoCommon::BuildThumbnailUri(thumbnail.GetString(), "1920x1080");
        }

        const auto& ontoPoster = content["onto_poster"];
        if (ontoPoster.IsString()) {
            videoItem->CoverUrl2X3() = NAlice::NVideoCommon::BuildThumbnailUri(ontoPoster.GetString(), "320x480");
        }

        const auto& description = content["description"];
        if (description.IsString()) {
            videoItem->Description() = description.GetString();
        }

        const auto& ontoType = content["onto_otype"];
        if (ontoType == "Film/Film") {
            videoItem->Type() = MOVIE_TYPE;
        } else if (ontoType == "Film/Series@on") {
            videoItem->Type() = SERIAL_TYPE;
        }

        const auto& ontoId = content["onto_id"];
        if (ontoId.IsString()) {
            videoItem->Entref() = TStringBuilder() << "entnext=" << ontoId.GetString();
        }

        const auto& kinopoiskRating = content["rating_kp"];
        if (kinopoiskRating.IsNumber()) {
            videoItem->Rating() = content["rating_kp"].GetNumber();
        }

        const auto& releaseYear = content["release_year"];
        if (releaseYear.IsIntNumber()) {
            videoItem->ReleaseYear() = releaseYear.GetIntNumber();
        }

        const auto& restrictionAge = content["restriction_age"];
        if (restrictionAge.IsIntNumber()) {
            videoItem->MinAge() = restrictionAge.GetIntNumber();
            videoItem->AgeLimit() = ToString(restrictionAge.GetIntNumber());
        }

        const auto& providerItemId = content["content_id"];
        if (providerItemId.IsString()) {
            if (videoItem->Type() == MOVIE_TYPE || videoItem->Type() == SERIAL_TYPE) {
                const NBASS::NVideo::TLightVideoItem providerInfo{CreateProviderInfo(providerItemId, NAlice::NVideoCommon::PROVIDER_KINOPOISK, videoItem->Type())};
                NVideoCommon::AddProviderInfoIfNeeded(videoItem.Scheme(), providerInfo.Scheme());
                NBASS::NVideo::FillFromProviderInfo(NBASS::NVideo::TLightVideoItem(*providerInfo->GetRawValue()), videoItem);
            }
        }
    }

    void UpdateTitle(NBASS::NVideo::TVideoItem& videoItem, const TString& title)
    {
        videoItem->Name() = title;
    }

    void UpdateThumbnail(NBASS::NVideo::TVideoItem& videoItem, const TString& thumbnail)
    {
        videoItem->ThumbnailUrl16X9() = thumbnail;
        videoItem->ThumbnailUrl16X9Small() = thumbnail;
    }

    NBASS::NVideo::TVideoItem CreateVideoItem(
        const TStringBuf url,
        const TStringBuf itemId,
        bool shouldAddProviderInfo,
        const TMaybe<TString>& title = Nothing(),
        const TMaybe<TString>& thumbnail = Nothing(),
        const TMaybe<TString>& description = Nothing(),
        const TMaybe<ui32>& duration = Nothing(),
        const TMaybe<ui32>& seasonNumber = Nothing(),
        const TMaybe<ui32>& episodeNumber = Nothing()
    ) {
        NBASS::NVideo::TVideoItem item{};

        item->PlayUri() = url;
        item->ProviderName() = NAlice::NVideoCommon::PROVIDER_STRM;
        item->ProviderItemId() = itemId;
        item->Type() = VIDEO_TYPE;
        item->Available() = true;

        if (title.Defined()) {
            UpdateTitle(item, title.GetRef());
        }

        if (thumbnail.Defined()) {
            UpdateThumbnail(item, thumbnail.GetRef());
        }

        if (description.Defined()) {
            item->Description() = description.GetRef();
        }

        if (duration.Defined()) {
            item->Duration() = duration.GetRef();
        }

        if (shouldAddProviderInfo) {
            const NBASS::NVideo::TLightVideoItem providerInfo{CreateProviderInfo(itemId, NAlice::NVideoCommon::PROVIDER_STRM, VIDEO_TYPE)};
            NBASS::NVideo::FillFromProviderInfo(NBASS::NVideo::TLightVideoItem(*providerInfo->GetRawValue()), item);
        }

        if (seasonNumber.Defined()) {
            item->Season() = seasonNumber.GetRef();
        }

        if (episodeNumber.Defined()) {
            item->Episode() = episodeNumber.GetRef();
        }

        return item;
    }

    void PrepareCommandForShowDescription(
        NBASS::TContext& context,
        NBASS::NVideo::TVideoItem& videoItem,
        const TStringBuf attention
    ) {
        if (attention) {
            context.AddAttention(attention);
        }
        if (!context.HasExpFlag(NAlice::NVideoCommon::FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY) && !videoItem->Entref()->empty()) {
            return NBASS::NVideo::AddShowWebviewVideoEntityResponse(videoItem.Scheme(), context);
        } else {
            NBASS::NVideo::TShowVideoDescriptionCommandData command;
            command->Item() = videoItem.Scheme();
            context.AddCommand<NBASS::TVideoShowDescriptionDirective>(NAlice::NVideoCommon::COMMAND_SHOW_DESCRIPTION, std::move(command.Value()));
        }
    }
}

namespace NBASS::NEther {

TResultValue TVideoHandler::Do(TRequestHandler& r)
{
    TContext& context = r.Ctx();
    context.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    if (!context.HasExpFlag(EXP_ETHER)) {
        return TResultValue();
    }

    if (context.FormName() == VIDEO_SELECT) {
        const auto& deviceState = context.Meta().DeviceState();
        const TStringBuf userSubscription = deviceState.Video().HasViewState() ?
                NAlice::NVideoCommon::GetUserSubscriptionType(*deviceState.Video().ViewState().GetRawValue()) :
                NAlice::NVideoCommon::GetUserSubscriptionType(*deviceState.Video().PageState().GetRawValue());


        NVideo::TEnumSlot<NVideo::ESelectionAction> action{context.GetSlot(NAlice::NVideoCommon::SLOT_ACTION, TStringBuf("video_selection_action")), NVideo::ESelectionAction::Play};

        TString url = TString{context.GetSlot("content_url")->Value.GetString()};
        TString itemId = TString{context.GetSlot("uuid")->Value.GetString()};

        TMaybe<TString> title;
        if (const auto* slot = context.GetSlot("title"); !IsSlotEmpty(slot)) {
            title = TString{slot->Value.GetString()};
        }

        TMaybe<TString> thumbnail;
        if (const auto* slot = context.GetSlot("thumbnail"); !IsSlotEmpty(slot)) {
            thumbnail = NAlice::NVideoCommon::BuildThumbnailUri(TString{slot->Value.GetString()});
        }

        TMaybe<TString> description;
        if (const auto* slot = context.GetSlot("description"); !IsSlotEmpty(slot)) {
            description = TString{slot->Value.GetString()};
        }

        TMaybe<ui32> duration;
        if (const auto* slot = context.GetSlot("duration"); !IsSlotEmpty(slot)) {
            duration = static_cast<ui32>(slot->Value.GetIntNumber());
        }

        TMaybe<TString> serialId;
        if (const auto* slot = context.GetSlot("serial_id"); !IsSlotEmpty(slot)) {
            serialId = TString{slot->Value.GetString()};
        }

        THashSet<TStringBuf> subscriptions;
        if (const auto* slot = context.GetSlot("subscriptions"); !IsSlotEmpty(slot)) {
            const auto subscriptionsCommaSeparated = slot->Value.GetString();
            TContainerConsumer<THashSet<TStringBuf>> consumer(&subscriptions);
            const TCharDelimiter<const char> delimiter = ',';
            SplitString(subscriptionsCommaSeparated.begin(), subscriptionsCommaSeparated.end(), delimiter, consumer);
        }

        auto currentItem = CreateVideoItem(
            url,
            itemId,
            false /* shouldAddProviderInfo */,
            title,
            thumbnail,
            description,
            duration);

        const auto& requestedItemId = serialId.Defined() ? serialId.GetRef() : itemId;
        if (NSc::TValue response; MakeRequestToPlayerHandler(context, requestedItemId, response)) {
            UpdateVideoItemFromPlayerHandlerResponse(currentItem, response);
        }

        switch (action.GetEnumValue()) {
            case NVideo::ESelectionAction::Play: {
                const bool isAvailableBySubscription = !subscriptions || subscriptions.contains(userSubscription);
                if (!isAvailableBySubscription) {
                    PrepareCommandForShowDescription(context, currentItem, NVideo::ATTENTION_PAID_CONTENT);
                    return TResultValue();
                }

                if (currentItem->Type() == MOVIE_TYPE || currentItem->Type() == SERIAL_TYPE) {
                    return NBASS::PlayVideo(context, currentItem.Scheme());
                }

                if (context.HasExpFlag(EXPERIMENTAL_FLAG_MORDOVIA_SUPPORT_CHANNELS)) {
                    TTvChannelsHelper tvHelper(context);
                    return tvHelper.PlayCurrentTvEpisode(itemId);
                }

                NVideo::TPlayVideoCommandData commandData;
                commandData->Item() = currentItem.Scheme();
                if (currentItem->HasNextItems()) {
                    commandData->NextItem() = currentItem->NextItems(0);
                }
                const TStringBuf streamsAsString = context.GetSlot("streams")->Value.GetString();
                const TString payload = TStringBuilder{} << "{\"streams\": " << streamsAsString << "}";
                commandData->Payload() = payload;
                commandData->Uri() = url;
                context.AddAttention(NVideo::ATTENTION_AUTOPLAY);
                NVideo::AddPlayCommand(context, commandData, true);

                break;
            }
            case NVideo::ESelectionAction::Description: {
                if (currentItem->Type() == MOVIE_TYPE || currentItem->Type() == SERIAL_TYPE) {
                    PrepareCommandForShowDescription(context, currentItem, NVideo::ATTENTION_AUTOSELECT);
                }
                break;
            }
            case NVideo::ESelectionAction::ListSeasons: {
                break;
            }
            case NVideo::ESelectionAction::ListEpisodes: {
                break;
            }
        }
    }

    return TResultValue();
}

void TVideoHandler::Register(THandlersMap* handlers)
{
    handlers->RegisterFormHandler(VIDEO_SELECT, []() { return MakeHolder<TVideoHandler>(); });
}

}
