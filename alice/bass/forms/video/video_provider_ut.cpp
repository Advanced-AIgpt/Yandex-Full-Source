#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/forms/video/video_provider.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/bass/libs/video_common/video_ut_helpers.h>

#include <alice/bass/util/error.h>
#include <alice/bass/ut/helpers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/system/yassert.h>

using namespace NBASS;
using namespace NBASS::NVideo;
using namespace NTestingHelpers;
using namespace NVideoCommon;

namespace {
class TFakeProvider : public IVideoClipsProvider {
public:
    explicit TFakeProvider(bool authorized)
        : Authorized(authorized) {
    }

    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& /* request */) const override {
        return {};
    }

    std::unique_ptr<IWebSearchByProviderHandle>
    MakeWebSearchRequest(const TVideoClipsRequest& /* request */) const override {
        return {};
    }

    std::unique_ptr<IVideoItemHandle>
    MakeContentInfoRequest(TVideoItemConstScheme /* item */,
                           NHttpFetcher::IMultiRequest::TRef /* multiRequest */,
                           bool /* forceUnfiltered */) const override {
        return {};
    }

    std::unique_ptr<IVideoItemHandle>
    MakeSimpleContentInfoRequest(TVideoItemConstScheme /* item */,
                                 NHttpFetcher::IMultiRequest::TRef /* multiRequest */,
                                 bool /* forceUnfiltered */) const override {
        return {};
    }

    std::unique_ptr<IVideoClipsHandle>
    MakeRecommendationsRequest(const TVideoClipsRequest& /* request */) const override {
        return {};
    }

    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& /* request */) const override {
        return {};
    }

    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& /* request */) const override {
        return {};
    }

    std::unique_ptr<IVideoClipsHandle>
    MakeVideosByGenreRequest(const TVideoClipsRequest& /* request */) const override {
        return {};
    }

    TResult ResolveTvShowSeason(TVideoItemConstScheme /* tvShowItem */, const TSerialIndex& /* season */,
                                TVideoGalleryScheme* /* episodes */,
                                TSerialDescriptor* /* serialDescr */,
                                TSeasonDescriptor* /* seasonDescr */) const override {
        return TResult{NBASS::TError{NBASS::TError::EType::NOTSUPPORTED}};
    }

    TResult ResolveTvShowEpisode(TVideoItemConstScheme /* tvShowItem */, TVideoItemScheme* /* episodeItem */,
                                 TVideoItemScheme* /* nextEpisodeItem */,
                                 const TResolvedEpisode& /* resolvedEpisode */) const override {
        return TResult{NBASS::TError{NBASS::TError::EType::NOTSUPPORTED}};
    }

    TResultValue GetSerialDescriptor(TVideoItemConstScheme /* tvShowItem */,
                                     TSerialDescriptor* /* serial */) const override {
        return NBASS::TError{NBASS::TError::EType::NOTSUPPORTED};
    }

    TResult FillSeasonDescriptor(TVideoItemConstScheme /* tvShowItem */, const TSerialDescriptor& /* serialDescr */,
                                 const TSerialIndex& /* season */, TSeasonDescriptor* /* result */) const override {
        return TResult{NBASS::TError{NBASS::TError::EType::NOTSUPPORTED}};
    }

    TResult FillSerialAndSeasonDescriptors(TVideoItemConstScheme /* tvShowItem */, const TSerialIndex& /* season */,
                                           TSerialDescriptor* /* serialDescr */,
                                           TSeasonDescriptor* /* seasonDescr */) const override {
        return TResult{NBASS::TError{NBASS::TError::EType::NOTSUPPORTED}};
    }

    TResultValue FillProviderUniqueVideoItem(TVideoItem& /* item */) const override {
        return {};
    }

    TStringBuf GetProviderName() const override {
        return {};
    }

    TStringBuf GetAuthToken() const override {
        return {};
    }

    bool IsUnauthorized() const override {
        return !Authorized;
    }

protected:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme /* item */,
                                       TPlayVideoCommandDataScheme /* commandData */) const override {
        return TPlayError{NVideoCommon::EPlayError::VIDEOERROR, "unsupported"};
    }

private:
    const bool Authorized;
};

TSerialDescriptor MakeTestSerialDescriptor() {
    constexpr TStringBuf serialId = "FakeSerial";

    auto season1 = NTestingHelpers::MakeSeason({} /* providerName */, serialId, "s1" /* id */, 0, 2);
    auto season2 = NTestingHelpers::MakeSeason({} /* providerName */, serialId, "s1" /* id */, 1, 3);
    auto season3 = NTestingHelpers::MakeSeason({} /* providerName */, serialId, "s1" /* id */, 2, 2);

    season2.Soon = true;
    season2.UpdateAt = TInstant::ParseIso8601("2019-05-15T11:00:00Z");
    season2.EpisodeItems[1]->UpdateAtUs() = season2.UpdateAt->MicroSeconds();
    season2.EpisodeItems[1]->Soon() = true;
    season2.EpisodeItems[2]->UpdateAtUs() = TInstant::ParseIso8601("2019-05-15T13:00:00Z").MicroSeconds();
    season2.EpisodeItems[2]->Soon() = true;

    season3.Soon = true;
    season3.UpdateAt = TInstant::ParseIso8601("2019-05-16T11:00:00Z");
    season3.EpisodeItems[0]->Soon() = true;
    season3.EpisodeItems[0]->UpdateAtUs() = season3.UpdateAt->MicroSeconds();
    season3.EpisodeItems[1]->Soon() = true;
    season3.EpisodeItems[1]->UpdateAtUs() = season3.UpdateAt->MicroSeconds();

    TSerialDescriptor result;
    result.Id = serialId;
    result.Seasons = {season1, season2, season3};
    result.TotalEpisodesCount = 7;
    result.MinAge = Nothing();
    return result;
}

class TFakeDescriptorProvider : public TFakeProvider {
public:
    TFakeDescriptorProvider(bool authorized)
        : TFakeProvider(authorized)
        , FakeSerialDescriptor{MakeTestSerialDescriptor()}
    {
    }

    TResultValue GetSerialDescriptor(TVideoItemConstScheme, TSerialDescriptor *serialDescriptor) const override {
        *serialDescriptor = FakeSerialDescriptor;
        return {};
    }

    TResult FillSeasonDescriptor(TVideoItemConstScheme, const TSerialDescriptor& serialDescriptor,
                                 const TSerialIndex& seasonIndex, TSeasonDescriptor* seasonDescriptor) const override {
        auto seasonIdx = ExtractSerialIndexValue(seasonIndex, 0, serialDescriptor.Seasons.size() - 1);
        Y_ASSERT(seasonIdx);
        if (*seasonIdx >= serialDescriptor.Seasons.size())
            return TResult{EBadArgument::Season};

        *seasonDescriptor = serialDescriptor.Seasons[*seasonIdx];
        return {};
    }

    TResult FillSerialAndSeasonDescriptors(TVideoItemConstScheme tvShowItem, const TSerialIndex& seasonIndex,
                                           TSerialDescriptor* serialDescr,
                                           TSeasonDescriptor* seasonDescr) const override {
        *serialDescr = FakeSerialDescriptor;
        return FillSeasonDescriptor(tvShowItem, *serialDescr, seasonIndex, seasonDescr);
    }

private:
    TSerialDescriptor FakeSerialDescriptor;
};

struct TEpisodeChecker {
    bool operator()(const IVideoClipsProvider::TResolvedEpisode& episode) const {
        return episode.EpisodeIndex == ExpectedEpisodeIndex && episode.SeasonIndex == ExpectedSeasonIndex;
    }

    bool operator()(const IVideoClipsProvider::TResult&) const {
        return false;
    }

    ui32 ExpectedSeasonIndex = 0;
    ui32 ExpectedEpisodeIndex = 0;
};

struct TErrorChecker {
    bool operator()(const IVideoClipsProvider::TResolvedEpisode&) const {
        return false;
    }

    bool operator()(const IVideoClipsProvider::TResult& error) const {
        return !ErrorKindToCheck || error == *ErrorKindToCheck;
    }

    TMaybe<IVideoClipsProvider::TResult> ErrorKindToCheck = Nothing();
};

class TTestClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TTestClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context)
    {
    }

    // IVideoClipsProvider overrides:
    bool IsUnauthorized() const override {
        return true;
    }

    TStringBuf GetProviderName() const override {
        return {};
    }

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme /* item */,
                                       TPlayVideoCommandDataScheme /* commandData */) const override {
        return {};
    }

protected:
    // TVideoClipsHttpProviderBase:
    TResultValue DoGetSerialDescriptor(TVideoItemConstScheme /* tvShowItem */,
                                       TSerialDescriptor* serialDescr) const override {
        *serialDescr = MakeTestSerialDescriptor();
        return {};
    }

    TResultValue RequestAndParseSeasonDescription(TVideoItemConstScheme /* item */,
                                                  const TSerialDescriptor& serialDescr,
                                                  TSeasonDescriptor& seasonDescr) const override {
        Y_ENSURE(seasonDescr.ProviderNumber <= 3, "Season index is too big!");
        seasonDescr = serialDescr.Seasons[seasonDescr.ProviderNumber - 1];
        return {};
    }
};

TContext::TPtr CreateContextWithTimestamp(TStringBuf timestampIso8601) {
    const TInstant timestamp = TInstant::ParseIso8601(timestampIso8601);
    auto contextCreator = [timestamp](const NSc::TValue& context) {
        NSc::TValue patchedContext = context;
        patchedContext["meta"]["request_start_time"] = timestamp.MicroSeconds();
        patchedContext["meta"]["experiments"].Push(NSc::TValue(NAlice::NVideoCommon::FLAG_VIDEO_DONT_USE_CONTENT_DB));
        return MakeContext(patchedContext);
    };
    return CreateVideoContextWithAgeRestriction(EContentRestrictionLevel::Without, contextCreator);
}

Y_UNIT_TEST_SUITE(VideoProviderEpisodeResolving) {
    Y_UNIT_TEST(ResolveSeasonAndEpisode) {
        {
            NSc::TValue VIDEO_PLAY_DUMMY_REQUEST = NSc::TValue::FromJson(R"(
            {
                "form": {
                    "name": "personal_assistant.scenarios.video_play",
                    "slots": []
                },
                "meta": {
                    "epoch": 1526559187,
                    "tz": "Europe/Moscow",
                    "device_state": {
                        "last_watched": {
                            "tv_shows": [
                                {
                                    "item": {
                                        "season": 2,
                                        "episode": 3,
                                        "provider_name": "",
                                        "provider_item_id": "s1"
                                    },
                                    "tv_show_item": {
                                        "provider_name": "",
                                        "provider_item_id": "s1"
                                    }
                                }
                            ]
                        }
                    }
                }
            }
            )");
            TFakeDescriptorProvider provider(true /* authorized */);
            auto ctxPtr = NTestingHelpers::MakeContext(VIDEO_PLAY_DUMMY_REQUEST);
            UNIT_ASSERT(ctxPtr);
            TContext& ctx = *ctxPtr;

            NSc::TValue item;
            TVideoItemConstScheme tvShowItem(&item);

            NSc::TValue lastWatchedItem = NSc::TValue::FromJson(R"(
            {
                "provider_name": "",
                "provider_item_id": "s1"
            }
            )");
            TVideoItemConstScheme lastWatchedTvShowItem(&lastWatchedItem);

            // Concrete values of season/episode.
            {
                const auto episodeOrError =
                    provider.ResolveSeasonAndEpisode(tvShowItem, ctx, 0U, 0U, Nothing(), Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError =
                    provider.ResolveSeasonAndEpisode(tvShowItem, ctx, 1U, 2U, Nothing(), Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 2};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError =
                    provider.ResolveSeasonAndEpisode(tvShowItem, ctx, 1U, 2U, 2U, 0U);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }

            // Last/Init.
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(tvShowItem, ctx, Nothing(), Nothing(),
                                                                             Nothing(), ESpecialSerialNumber::Last);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 1}; // The very last episode.
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U /* Second season is being shown right now */,
                    Nothing() /* No episode is chosen right now (season gallery) */,
                    Nothing() /* The user hasn't pointed the season explicitly */,
                    ESpecialSerialNumber::Last /* The user has requested the last episode */);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1,
                                        .ExpectedEpisodeIndex = 2}; // The last episode of the 2nd season.
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError =
                    provider.ResolveSeasonAndEpisode(tvShowItem, ctx, 1U, 0U, 2U, Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError =
                    provider.ResolveSeasonAndEpisode(tvShowItem, ctx, 0U, 1U, ESpecialSerialNumber::Last, Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError =
                    provider.ResolveSeasonAndEpisode(tvShowItem, ctx, 1U, 2U, ESpecialSerialNumber::Init, Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 2U, ESpecialSerialNumber::Init, ESpecialSerialNumber::Last);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, Nothing(), 1U, ESpecialSerialNumber::Last);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 2};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }

            // Prev/next.
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 0U, 0U, ESpecialSerialNumber::Next, 1U);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 2U, ESpecialSerialNumber::Next, ESpecialSerialNumber::Last);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 2U, 0U, ESpecialSerialNumber::Next, ESpecialSerialNumber::Last);
                UNIT_ASSERT(std::visit(TErrorChecker(), episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 0U, 0U, Nothing(), ESpecialSerialNumber::Next);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 1U, Nothing(), ESpecialSerialNumber::Next);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 2};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 2U, Nothing(), ESpecialSerialNumber::Next);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 0U, 0U, ESpecialSerialNumber::Prev, Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 2U, ESpecialSerialNumber::Prev, 1U);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 2U, ESpecialSerialNumber::Prev, ESpecialSerialNumber::Last);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 0U, Nothing(), ESpecialSerialNumber::Prev);
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 0, .ExpectedEpisodeIndex = 1};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }

            // Corner cases
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, Nothing(), 0U, Nothing(), ESpecialSerialNumber::Prev);
                TErrorChecker visitor{.ErrorKindToCheck = NBASS::TError{NBASS::TError::EType::INVALIDPARAM}};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 1U, 0U, ESpecialSerialNumber::Next, ESpecialSerialNumber::Prev);
                TErrorChecker visitor{.ErrorKindToCheck = NBASS::TError{NBASS::TError::EType::INVALIDPARAM}};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 0U, 0U, Nothing(), ESpecialSerialNumber::Prev);
                TErrorChecker visitor{.ErrorKindToCheck = EBadArgument::NoPrevEpisode};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    tvShowItem, ctx, 2U, 1U, Nothing(), ESpecialSerialNumber::Next); // Next for the very last episode.
                TErrorChecker visitor{.ErrorKindToCheck = EBadArgument::NoNextEpisode};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }

            // Last watched
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    lastWatchedTvShowItem, ctx, Nothing(), Nothing(), Nothing(), Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 2};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    lastWatchedTvShowItem, ctx, 1U, Nothing(), Nothing(), Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 2};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    lastWatchedTvShowItem, ctx, 2U, Nothing(), Nothing(), Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    lastWatchedTvShowItem, ctx, 2U, Nothing(), 1U, Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 1, .ExpectedEpisodeIndex = 2};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
            {
                const auto episodeOrError = provider.ResolveSeasonAndEpisode(
                    lastWatchedTvShowItem, ctx, 2U, Nothing(), 2U, Nothing());
                TEpisodeChecker visitor{.ExpectedSeasonIndex = 2, .ExpectedEpisodeIndex = 0};
                UNIT_ASSERT(std::visit(visitor, episodeOrError));
            }
        }
    }

    Y_UNIT_TEST(TimeAvailability) {
        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-15T10:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            provider.GetSerialDescriptor(item.Scheme(), &serialDescr);
            UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 1u);
        }
        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-15T11:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            provider.GetSerialDescriptor(item.Scheme(), &serialDescr);
            UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 2u);
        }
        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-15T13:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            provider.GetSerialDescriptor(item.Scheme(), &serialDescr);
            UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 2u);
        }
        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-16T11:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            provider.GetSerialDescriptor(item.Scheme(), &serialDescr);
            UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 3u);
        }

        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-15T11:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            TSeasonDescriptor seasonDescr;
            {
                const auto error = provider.FillSerialAndSeasonDescriptors(item.Scheme(), 1u /* seasonIndex */,
                                                                           &serialDescr, &seasonDescr);
                UNIT_ASSERT(!error);
                UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 2u);
                UNIT_ASSERT_VALUES_EQUAL(seasonDescr.EpisodeItems.size(), 2u);
            }

            {
                const auto error = provider.FillSerialAndSeasonDescriptors(item.Scheme(), 2u /* seasonIndex */,
                                                                           &serialDescr, &seasonDescr);
                UNIT_ASSERT(error.Defined());
            }
        }
        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-15T14:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            TSeasonDescriptor seasonDescr;
            {
                const auto error = provider.FillSerialAndSeasonDescriptors(item.Scheme(), 1u /* seasonIndex */,
                                                                           &serialDescr, &seasonDescr);
                UNIT_ASSERT(!error);
                UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 2u);
                UNIT_ASSERT_VALUES_EQUAL(seasonDescr.EpisodeItems.size(), 3u);
            }

            {
                const auto error = provider.FillSerialAndSeasonDescriptors(item.Scheme(), 2u /* seasonIndex */,
                                                                           &serialDescr, &seasonDescr);
                UNIT_ASSERT(error.Defined());
            }
        }
        {
            auto ctxPtr = CreateContextWithTimestamp("2019-05-16T11:00:00Z");
            UNIT_ASSERT(ctxPtr);
            TTestClipsProvider provider{*ctxPtr};
            TVideoItem item;
            TSerialDescriptor serialDescr;
            TSeasonDescriptor seasonDescr;
            {
                const auto error = provider.FillSerialAndSeasonDescriptors(item.Scheme(), 1u /* seasonIndex */,
                                                                           &serialDescr, &seasonDescr);
                UNIT_ASSERT(!error);
                UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 3u);
                UNIT_ASSERT_VALUES_EQUAL(seasonDescr.EpisodeItems.size(), 3u);
            }

            {
                const auto error = provider.FillSerialAndSeasonDescriptors(item.Scheme(), 2u /* seasonIndex */,
                                                                           &serialDescr, &seasonDescr);
                UNIT_ASSERT(!error);
                UNIT_ASSERT_VALUES_EQUAL(serialDescr.Seasons.size(), 3u);
                UNIT_ASSERT_VALUES_EQUAL(seasonDescr.EpisodeItems.size(), 2u);
            }
        }
    }
}

} // namespace
