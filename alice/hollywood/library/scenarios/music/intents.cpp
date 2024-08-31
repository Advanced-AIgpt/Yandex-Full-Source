#include "intents.h"

#include "common.h"

#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/music/defs.h>
#include <alice/megamind/library/util/slot.h>

#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>
#include <util/string/join.h>

using namespace NAlice::NMusic;
using namespace ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood::NMusic {

namespace {

const TString PROMO_TYPE = "promo_type";
const TString ALICE_SHOW = "alice_show";
const TString MEDITATION = "meditation";

const TString LITE_HARDCODED_PLAYLIST_PHRASE = "lite_hardcoded_playlist";

const TString AUTOPLAY_ACTION_REQUEST = "autoplay";

const TString ALICE_SHOW_PLAYLIST = "morningShow";

const TString GET_MORNING_SHOW_PUSHES = "get_morning_show_pushes";

constexpr TStringBuf MEDITATION_TYPE_SLOT = "meditation_type";
constexpr TStringBuf MEDITATION_TYPE_DEFAULT_VALUE = "basic";

constexpr TStringBuf SLOT_SHOW_TYPE = "show_type";

const TString ALICE_SHOW_ANALYTICS_INTENT = "alice.vinsless.music.morning_show";
const TString ALICE_SHOW_INTENT_GOOD_EVENING = "alice.alice_show.good_evening";
const TString ALICE_SHOW_INTENT_GOOD_MORNING = "personal_assistant.scenarios.morning_show_good_morning";

const TString MEDITATION_ANALYTICS_INTENT = "alice.vinsless.music.meditation";
const TString MEDITATION_INTENT = "alice.meditation";

constexpr TStringBuf EXP_MM_ENABLE_MEDITATION = "mm_enable_meditation";

using EPromoType = NClient::EPromoType;

struct TLitePlaylist {
    const TString Kind;
    const TString OwnerId;
};

const THashMap<EPromoType, TLitePlaylist> LITE_PLAYLISTS = {
    {EPromoType::PT_GREEN_PERSONALITY, TLitePlaylist{/* Kind= */ "2568", /* OwnerId= */ "103372440"}},
    {EPromoType::PT_BEIGE_PERSONALITY, TLitePlaylist{/* Kind= */ "2571", /* OwnerId= */ "103372440"}},
    {EPromoType::PT_PINK_PERSONALITY, TLitePlaylist{/* Kind= */ "2572", /* OwnerId= */ "103372440"}},
    {EPromoType::PT_YELLOW_PERSONALITY, TLitePlaylist{/* Kind= */ "2570", /* OwnerId= */ "103372440"}},
    {EPromoType::PT_RED_PERSONALITY, TLitePlaylist{/* Kind= */ "2569", /* OwnerId= */ "103372440"}},
    {EPromoType::PT_PURPLE_PERSONALITY, TLitePlaylist{/* Kind= */ "2567", /* OwnerId= */ "103372440"}}};

NJson::TJsonValue RenderMorningShowTopic(const NData::NMusic::TTopic& showPart) {
    if (!showPart.GetPodcast()) {
        return NJson::TJsonValue::UNDEFINED;
    }
    return NJson::TJsonMap({
        {"selector", "podcast"},
        {"group", showPart.GetPodcast()},
    });
}

TVector<NJson::TJsonValue> RenderMorningShowTopics(const TScenarioRunRequestWrapper& req, const TMorningShowTopicsConfig& configProto, const EShowType showType, IRng& rng) {
    int topicCount = 1; // any negative is default (all topics)

    const TString topicCountFlag = TString::Join("hw_", ToString(showType), "_show_topic_count=");
    if (const auto value = req.GetValueFromExpPrefix(topicCountFlag); value.Defined()) {
        TryFromString(*value, topicCount);
    }
    TVector<NJson::TJsonValue> topicSelectors;
    if (topicCount == 0 || configProto.GetDisabled()) {
        return topicSelectors;
    }

    TVector<int> indices(configProto.GetTopics().size());
    Iota(indices.begin(), indices.end(), 0);
    ShuffleRange(indices, rng);

    for (const auto& index : indices) {
        const auto& showPart = configProto.GetTopics()[index];
        if (auto showPartJson = RenderMorningShowTopic(showPart); showPartJson != NJson::TJsonValue::UNDEFINED) {
            topicSelectors.push_back(std::move(showPartJson));
            if ((int)topicSelectors.size() == topicCount) {
                // will never happen with negative topicCount
                break;
            }
        }
    }
    return topicSelectors;
}

std::pair<TStringBuf, TStringBuf> GetMorningShowNewsSourceAndRubric(const TMorningShowNewsConfig& newsConfigProto, IRng& rng) {
    const auto nNews = newsConfigProto.GetNewsProviders().size();
    if (nNews == 0) {
        return { DEFAULT_MORNING_SHOW_NEWS_SOURCE, DEFAULT_MORNING_SHOW_NEWS_RUBRIC };
    }
    const auto& provider = newsConfigProto.GetNewsProviders(rng.RandomInteger(nNews));
    const auto& source = provider.GetNewsSource();
    if (!source.empty() && source != DEFAULT_MORNING_SHOW_NEWS_SOURCE) {
        return { source, {} };
    }
    return { source, provider.GetRubric() };
}

bool IsEveningTime(const NDatetime::TSimpleTM& localTime) {
    return localTime.Hour < 5 || localTime.Hour >= 18;
}

class TMorningShowPartsBuilder {
public:
    TMorningShowPartsBuilder(const TScenarioRunRequestWrapper& req,
                             const TMorningShowProfile& profile,
                             const EShowType showType,
                             const TMaybe<THardcodedMorningShowSemanticFrame>& sourceFrame,
                             IRng& rng)
        : Request(req)
        , Profile(profile)
        , NextTrackIndex(sourceFrame ? sourceFrame->GetNextTrackIndex().GetNumValue() : 0)
        , ShowType(showType)
        , SourceFrame(sourceFrame)
        , Rng(rng)
    {
    }

    void AddMusicPreroll() {
        ShowParts.AppendValue(JsonFromString(R"(
            {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["music-preroll", "common"]}
        )"));
        if (ShowType == EShowType::Evening) {
            ShowParts.Back()["tags"][1] = "night";
        }
    }

    void AddOnBoarding() {
        ShowParts.AppendValue(JsonFromString(R"(
            {"selector": "onBoarding",
                "onBoarding": {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["onboarding"]},
                "greeting": {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["greeting", "newshow"]}
            }
        )"));
        if (SourceFrame.Defined() && SourceFrame->GetNewsProvider().HasSerializedData()) {
            ShowParts.Back()["greeting"]["tags"][1] = "about-news-sources";
        } else if (SourceFrame.Defined() && SourceFrame->GetTopic().HasSerializedData()) {
            ShowParts.Back()["greeting"]["tags"][1] = "about-podcast-topics";
        } else if (ShowType == EShowType::Evening) {
            ShowParts.Back()["onBoarding"]["tags"][0] = "onboarding-night";
            ShowParts.Back()["greeting"]["tags"][1] = "night";
        } else if (!IsDefaultMorningShowProfile(Profile)) {
            ShowParts.Back()["greeting"]["tags"][1] = "common-or-summer";
        }
    }

    void AddWeather() {
        auto selectorWeatherPreroll = JsonFromString(R"(
            {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["weather-preroll", "common-or-summer"]}
        )");
        auto selectorWeather = JsonFromString(R"(
            {"selector": "shot", "mandatoryGeo": true, "mandatoryUid": null, "shotType": "weather", "shotSubType": "common", "tags": ["today"]}
        )");
        if (ShowType == EShowType::Evening) {
            selectorWeatherPreroll["tags"][1] = "tomorrow";
            selectorWeather["tags"][0] = "tomorrow";
        }
        auto weather = JsonFromString(R"({"selector": "multi", "parts": []})");
        weather["parts"] = NJson::TJsonArray({selectorWeatherPreroll, selectorWeather});
        ShowParts.AppendValue(std::move(weather));
    }

    void AddNextTrack() {
        AddNextTrack(ShowParts);
    };

    void AddNews() {
        if (Profile.GetNewsConfig().GetDisabled() || Request.HasExpFlag(EXP_DISABLE_NEWS)) {
            return;
        }
        const auto [newsSource, newsRubric] = GetMorningShowNewsSourceAndRubric(Profile.GetNewsConfig(), Rng);
        if (newsSource.empty() || newsSource == DEFAULT_MORNING_SHOW_NEWS_SOURCE) {
            // common news
            auto selectorNewsPreroll = JsonFromString(R"(
                {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["news-preroll","common","v2"]}
            )");
            auto selectorNewsPart1 = JsonFromString(R"(
                {"selector": "shot", "mandatoryGeo": true, "mandatoryUid": null, "shotType": "news", "shotSubType": "partial", "tags": ["newsPart1-with-pause"]}
            )");
            if (newsRubric != DEFAULT_MORNING_SHOW_NEWS_RUBRIC) {
                selectorNewsPreroll["tags"] = NJson::TJsonArray({"news-preroll", "custom", newsRubric});
                selectorNewsPart1["tags"] = NJson::TJsonArray({TString::Join(newsRubric, "NewsPart1")});
            }
            NJson::TJsonValue newsPart1 = JsonFromString(R"({"selector": "multi", "parts": []})");
            newsPart1["parts"] = NJson::TJsonArray({selectorNewsPreroll, selectorNewsPart1});
            ShowParts.AppendValue(std::move(newsPart1));

            AddMusicPreroll();

            AddNextTrack();

            NJson::TJsonValue newsPart2 = JsonFromString(R"(
                {"selector": "multi", "parts": [
                    {"selector": "shot", "mandatoryGeo": true, "mandatoryUid": null, "shotType": "news", "shotSubType": "partial", "tags": ["shuffledRandomNewsPart1"]},
                    {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["music-preroll", "common"]},
                ]}
            )");
            if (ShowType == EShowType::Evening) {
                newsPart2["parts"][1]["tags"][1] = "night";
            }
            if (newsRubric != DEFAULT_MORNING_SHOW_NEWS_RUBRIC) {
                newsPart2["parts"][0]["tags"] = NJson::TJsonArray({TString::Join(newsRubric, "NewsPart2")});
            }
            ShowParts.AppendValue(std::move(newsPart2));

            AddNextTrack();

            NJson::TJsonValue newsPart3 = JsonFromString(R"(
                {"selector": "multi", "parts": [
                    {"selector": "shot", "mandatoryGeo": true, "mandatoryUid": null, "shotType": "news", "shotSubType": "partial", "tags": ["shuffledRandomNewsPart2"]},
                    {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["music-preroll", "common"]},
                ]}
            )");
            if (ShowType == EShowType::Evening) {
                newsPart3["parts"][1]["tags"][1] = "night";
            }
            if (newsRubric != DEFAULT_MORNING_SHOW_NEWS_RUBRIC) {
                newsPart3["parts"][0]["tags"] = NJson::TJsonArray({TString::Join(newsRubric, "NewsPart3")});
            }
            ShowParts.AppendValue(std::move(newsPart3));
        } else {
            // radio news
            auto selectorRadioNewsPreroll = JsonFromString(R"(
                {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show"}
            )");
            selectorRadioNewsPreroll["tags"] = NJson::TJsonArray({"radionews-preroll", "custom", newsSource});
            auto selectorRadioNews = JsonFromString(R"(
                {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "radionews", "shotSubType": "show"}
            )");
            selectorRadioNews["tags"] = NJson::TJsonArray({"radionews", newsSource});
            NJson::TJsonValue radioNews = JsonFromString(R"({"selector": "multi", "parts": []})");
            radioNews["parts"] = NJson::TJsonArray({selectorRadioNewsPreroll, selectorRadioNews});
            ShowParts.AppendValue(std::move(radioNews));

            AddMusicPreroll();
        }
        AddNextTrack();
    }

    void AddJoke() {
        NJson::TJsonValue joke = JsonFromString(R"(
            {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["joke", "summer", "head"]}
        )");
        ShowParts.AppendValue(joke);
        AddNextTrack();
    }

    void AddPromo() {
        if (ShowType != EShowType::Evening) {
            ShowParts.AppendValue(JsonFromString(R"({"selector": "promo"})"));
        }
    }

    void AddSkills(bool addTailTrack) {
        if (!SkillsEnabled()) {
            return;
        }
        auto skillSelectors = RenderShowSkills(Request, Profile.GetSkillsConfig(), ShowType, Rng, addTailTrack);
        for (size_t i = 0; i < skillSelectors.size(); ++i) {
            ShowParts.AppendValue(std::move(skillSelectors[i]));
        }
    }

    void TryAddSkillsBeforeTopics() {
        const bool skillsBeforeTopics = !Request.ExpFlags().contains(EXP_HW_MORNING_SHOW_SKILLS_AFTER_TOPICS);
        if (SkillsEnabled() && skillsBeforeTopics) {
            AddSkills(/* addTailTrack = */ true);
        }
    }

    void TryAddSkillsAfterTopics() {
        const bool skillsAfterTopics = Request.ExpFlags().contains(EXP_HW_MORNING_SHOW_SKILLS_AFTER_TOPICS);
        if (SkillsEnabled() && skillsAfterTopics) {
            AddSkills(/* addTailTrack = */ false);
        }
    }

    void AddTopics() {
        auto topicSelectors = RenderMorningShowTopics(Request, Profile.GetTopicsConfig(), ShowType, Rng);
        const bool insertMusic = Request.ExpFlags().contains(EXP_HW_MORNING_SHOW_MUSIC_BETWEEN_TOPICS);
        for (size_t i = 0; i < topicSelectors.size(); ++i) {
            if (insertMusic && i > 0) {
                AddNextTrack();
            }
            ShowParts.AppendValue(std::move(topicSelectors[i]));
        }
    }

    void AddTailPlaylist() {
        NJson::TJsonValue playlistTail = JsonFromString(R"(
            {"selector": "tail"}
        )");
        playlistTail["startFrom"] = NextTrackIndex;
        ShowParts.AppendValue(std::move(playlistTail));
    }

    NJson::TJsonArray Release() {
        return std::move(ShowParts);
    }

private:
    bool SkillsEnabled() const {
        return !Profile.GetSkillsConfig().GetDisabled();
    }

    void AddNextTrack(NJson::TJsonValue& selector) {
        selector.AppendValue(NJson::TJsonMap({
            {"selector", "playlist"},
            {"announce", "do_not_announce"},
            {"trackIndex", NextTrackIndex},
        }));
        ++NextTrackIndex;
    };

    NJson::TJsonValue RenderShowSkill(const TMorningShowSkillsConfig::TSkillProvider& showPart, bool addTrack) {
        if (!showPart.GetSkillSlug()) {
            return NJson::TJsonValue::UNDEFINED;
        }
        auto skillJson = JsonFromString(R"({
            "selector": "multi",
            "parts": [
                {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "comment", "shotSubType": "show", "tags": ["skill-preroll"]},
                {"selector": "shot", "mandatoryGeo": null, "mandatoryUid": null, "shotType": "skill", "shotSubType": "show", "tags": ["skill"]}
            ]
        })");
        skillJson["parts"][0]["tags"].AppendValue(showPart.GetSkillSlug());
        skillJson["parts"][1]["tags"].AppendValue(showPart.GetSkillSlug());
        if (!Request.ExpFlags().contains(EXP_HW_MORNING_SHOW_DISABLE_SKILL_HISTORY)) {
            skillJson["parts"][1]["mandatoryHistory"] = true;
        }
        if (addTrack) {
            AddNextTrack(skillJson["parts"]);
        }
        return skillJson;
    }

    TVector<NJson::TJsonValue> RenderShowSkills(const TScenarioRunRequestWrapper& req, const TMorningShowSkillsConfig& configProto, const EShowType showType, IRng& rng, bool addTailTrack) {
        int skillCount = -1; // any negative is default (all skills)

        const TString skillCountFlag = TString::Join("hw_", ToString(showType), "_show_skill_count=");
        if (const auto value = req.GetValueFromExpPrefix(skillCountFlag); value.Defined()) {
            TryFromString(*value, skillCount);
        }

        TVector<NJson::TJsonValue> skillSelectors;
        if (skillCount == 0 || configProto.GetDisabled()) {
            return skillSelectors;
        }

        TVector<int> indices(configProto.GetSkillProviders().size());
        Iota(indices.begin(), indices.end(), 0);
        ShuffleRange(indices, rng);

        const auto currentSkillCount = [&]() {
            return static_cast<int>(skillSelectors.size());
        };

        for (const auto& index : indices) {
            const auto& showPart = configProto.GetSkillProviders()[index];
            const bool addTrack = currentSkillCount() < skillCount - 1 || addTailTrack;

            if (auto showPartJson = RenderShowSkill(showPart, addTrack); showPartJson != NJson::TJsonValue::UNDEFINED) {
                skillSelectors.push_back(std::move(showPartJson));
                if (currentSkillCount() == skillCount) {
                    // will never happen with negative topicCount
                    break;
                }
            }
        }
        return skillSelectors;
    }

private:
    const TScenarioRunRequestWrapper& Request;
    const TMorningShowProfile& Profile;
    NJson::TJsonArray ShowParts;
    size_t NextTrackIndex;
    const EShowType ShowType;
    const TMaybe<THardcodedMorningShowSemanticFrame>& SourceFrame;

    IRng& Rng;
};

EShowType DetermineShowType(const TScenarioRunRequestWrapper& req) {
    const auto localTime = NDatetime::ToCivilTime(TInstant::Seconds(req.ClientInfo().Epoch), NDatetime::GetTimeZone(req.ClientInfo().Timezone));
    const auto rawSourceFrame = req.Input().FindSemanticFrame(ALICE_SHOW_INTENT);
    if (rawSourceFrame) {
        const auto slot = GetSlot(*rawSourceFrame, SLOT_SHOW_TYPE);
        if (slot && slot->GetValue() == "evening" || IsEveningTime(localTime)) {
            return EShowType::Evening;
        }
    } else if (req.Input().FindSemanticFrame(ALICE_SHOW_INTENT_GOOD_EVENING)) {
        return EShowType::Evening;
    }
    return EShowType::Morning;
}

// Default config: https://paste.yandex-team.ru/1289482
NJson::TJsonValue RenderPersonalisedMorningShowConfig(
    const TScenarioRunRequestWrapper& req,
    const TMorningShowProfile& profile,
    const EShowType showType,
    const TMaybe<THardcodedMorningShowSemanticFrame>& sourceFrame,
    IRng& rng,
    TRTLogger& logger
) {
    TMorningShowPartsBuilder builder{req, profile, showType, sourceFrame, rng};

    // offset is sent by the new protocol morning show to skip already shown parts.
    const size_t offset = sourceFrame.Defined() ? sourceFrame->GetOffset().GetNumValue() : 0;

    constexpr std::array parts{
        &TMorningShowPartsBuilder::AddOnBoarding,
        &TMorningShowPartsBuilder::AddWeather,
        &TMorningShowPartsBuilder::AddNews,
        &TMorningShowPartsBuilder::AddJoke,
        &TMorningShowPartsBuilder::TryAddSkillsBeforeTopics,
        &TMorningShowPartsBuilder::AddTopics,
        &TMorningShowPartsBuilder::AddPromo,
        &TMorningShowPartsBuilder::TryAddSkillsAfterTopics,
        &TMorningShowPartsBuilder::AddTailPlaylist,
    };
    for (const size_t i : xrange(offset, parts.size())) {
        (builder.*parts[i])();
    }

    auto result = NJson::TJsonMap({
        {"showParts", builder.Release()},
        {"id", -1},
    });
    if (showType == EShowType::Evening && !req.HasExpFlag(EXP_HW_EVENING_SHOW_FORCE_PLAYLIST_OF_THE_DAY)) {
        result["mainPlaylist"] = "DAILY_CALM";
    }
    result["fallbackPlaylistId"] = NJson::TJsonMap({
        // "Вечные хиты"
        {"uid",  105590476},
        {"kind", 1250}
    });
    LOG_INFO(logger) << "Personalised config: " << JsonToString(result);
    return result;
}

TFrame CreateSpecialPlaylistFrame(const TString& playlist) {
    TFrame frame{MUSIC_PLAY_FRAME};
    frame.AddSlot(TSlot{ToString(SLOT_SPECIAL_PLAYLIST),
                        ToString(SLOT_SPECIAL_PLAYLIST_TYPE),
                        TSlot::TValue{playlist}});
    frame.AddSlot(TSlot{ToString(SLOT_ACTION_REQUEST),
                        ToString(SLOT_ACTION_REQUEST_TYPE),
                        TSlot::TValue{AUTOPLAY_ACTION_REQUEST}});
    return frame;
}

class TLiteHardcodedPlaylistIntent : public TMusicHardcodedIntent {

public:
    TLiteHardcodedPlaylistIntent(TRTLogger& logger, EPromoType promoType)
        : TMusicHardcodedIntent(logger, LITE_HARDCODED_MUSIC_PLAY_INTENT)
        , PromoType(promoType)
    {
    }

    bool IsIrrelevant() const {
        return PromoType == EPromoType::PT_NO_TYPE;
    }

    std::variant<TFrame, NScenarios::TScenarioRunResponse> PrepareMusicFrame(TScenarioHandleContext&,
                                                                         const TScenarioRunRequestWrapper& req,
                                                                         TNlgWrapper& nlg) const override {
        const auto* playlistPtr = LITE_PLAYLISTS.FindPtr(PromoType);
        if (!playlistPtr) {
            return std::move(Irrelevant(nlg, req));
        }
        NJson::TJsonValue fixlist;
        fixlist[NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO]["value"]["kind"] = playlistPtr->Kind;
        fixlist[NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO]["value"]["owner"]["id"] = playlistPtr->OwnerId;

        fixlist[NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO]["name"] = SLOT_SPECIAL_ANSWER_INFO;
        fixlist[NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO]["type"] = SLOT_SPECIAL_ANSWER_INFO_TYPE;
        fixlist[NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO]["value"]["answer_type"] = SLOT_PLAYLIST;
        fixlist[NAlice::NMusic::SLOT_ORDER]["name"] = NAlice::NMusic::SLOT_ORDER;
        fixlist[NAlice::NMusic::SLOT_ORDER]["type"] = NAlice::NMusic::SLOT_ORDER_TYPE;
        fixlist[NAlice::NMusic::SLOT_ORDER]["value"] = "shuffle";
        return std::move(CreateSpecialAnswerFrame(fixlist));
    }

    void RenderResponse(const NJson::TJsonValue& bassResponse, TBassResponseRenderer& bassRenderer) const override {
        bassRenderer.SetContextValue(PROMO_TYPE, NClient::EPromoType_Name(PromoType));
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, LITE_HARDCODED_PLAYLIST_PHRASE, bassResponse);
    }

    const TString& ProductScenarioName() const override {
        return NAlice::NProductScenarios::MUSIC;
    }

private:

    NScenarios::TScenarioRunResponse Irrelevant(TNlgWrapper& nlg, const TScenarioRunRequestWrapper& req) const {
        // should have been THwFrameworkRunResponseBuilder, but that would make no sense
        TRunResponseBuilder response(&nlg, ConstructBodyRenderer(req));
        response.SetIrrelevant();
        return std::move(*std::move(response).BuildResponse());
    }

    EPromoType PromoType;
};

class TMeditationIntent : public TMusicHardcodedIntent {
public:
    TMeditationIntent(TRTLogger& logger, const TStringBuf meditationType)
        : TMusicHardcodedIntent(logger, MEDITATION_INTENT)
        , MeditationType(meditationType)
    {
    }

    std::variant<TFrame, NScenarios::TScenarioRunResponse> PrepareMusicFrame(TScenarioHandleContext&,
                                                                         const TScenarioRunRequestWrapper&,
                                                                         TNlgWrapper&) const override {
        return CreateSpecialPlaylistFrame(Join('_', MEDITATION, MeditationType));
    }

    void RenderResponse(const NJson::TJsonValue& bassResponse, TBassResponseRenderer& bassRenderer) const override {
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, MEDITATION, bassResponse, MEDITATION_ANALYTICS_INTENT);
    }

    const TString& ProductScenarioName() const override {
        return NAlice::NProductScenarios::MUSIC_MEDITATION;
    }

private:
    const TString MeditationType;
};

class TAliceShowIntent : public TMusicHardcodedIntent {
public:
    explicit TAliceShowIntent(TRTLogger& logger)
        : TMusicHardcodedIntent(logger, ALICE_SHOW_INTENT)
    {
    }

    std::variant<TFrame, NScenarios::TScenarioRunResponse> PrepareMusicFrame(TScenarioHandleContext& ctx,
                                                                         const TScenarioRunRequestWrapper& req,
                                                                         TNlgWrapper& nlg) const override {
        if (!req.ClientInfo().IsSmartSpeaker()) {
            THwFrameworkRunResponseBuilder response(ctx, &nlg, ConstructBodyRenderer(req));
            auto& bodyBuilder = response.CreateResponseBodyBuilder();
            bodyBuilder.CreateAnalyticsInfoBuilder()
                .SetIntentName(ALICE_SHOW_ANALYTICS_INTENT)
                .SetProductScenarioName(ProductScenarioName());
            TNlgData nlgData{Logger, req};
            if (!req.Input().FindSemanticFrame(ALICE_SHOW_INTENT)) {
                response.SetIrrelevant();
            }
            nlgData.Context["error"]["data"]["code"] = "morning_show_not_supported";
            nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "render_error__musicerror",
                    /* buttons = */ {}, nlgData);
            return *std::move(response).BuildResponse();
        }

        TMaybe<TSemanticFrame::TSlot> showTypeSlot;
        TMaybe<THardcodedMorningShowSemanticFrame> sourceFrame;
        if (const auto rawSourceFrame = req.Input().FindSemanticFrame(ALICE_SHOW_INTENT); rawSourceFrame) {
            sourceFrame = rawSourceFrame->GetTypedSemanticFrame().GetHardcodedMorningShowSemanticFrame();
            showTypeSlot = GetSlot(*rawSourceFrame, SLOT_SHOW_TYPE);
        }

        const EShowType showType = DetermineShowType(req);
        TMorningShowProfile morningShowProfile = ParseMorningShowProfile(req.BaseRequestProto().GetMemento());
        TryUpdateMorningShowProfileFromFrame(morningShowProfile, sourceFrame, true);
        const auto morningShowConfig = RenderPersonalisedMorningShowConfig(
            req,
            morningShowProfile,
            showType,
            sourceFrame,
            ctx.Rng,
            ctx.Ctx.Logger()
        );

        const auto uid = GetUid(req);
        const auto datasyncRequest = PrepareDataSyncRequest(
            DATASYNC_MORNING_SHOW_PATH,
            ctx.RequestMeta,
            TString(uid),
            ctx.Ctx.Logger(),
            GET_MORNING_SHOW_PUSHES
        );
        AddDataSyncRequestItems(ctx, datasyncRequest);

        auto frame = CreateSpecialPlaylistFrame(ALICE_SHOW_PLAYLIST);
        frame.AddSlot(TSlot{ToString(SLOT_MORNING_SHOW_CONFIG),
                            ToString(SLOT_MORNING_SHOW_CONFIG_TYPE),
                            TSlot::TValue{JsonToString(morningShowConfig)}});

        TString showTypeName;
        if (showTypeSlot.Defined()) {
            showTypeName = showTypeSlot->GetValue();
        } else if (sourceFrame.Defined() && sourceFrame->GetShowType().HasStringValue()) {
            showTypeName = ToString(sourceFrame->GetShowType().GetStringValue());
        } else {
            showTypeName = ToString(showType);
        }
        frame.AddSlot(TSlot{ToString(SLOT_MORNING_SHOW_TYPE),
                            ToString(SLOT_MORNING_SHOW_TYPE_TYPE),
                            TSlot::TValue{showTypeName}});

        return frame;
    }

    void RenderResponse(const NJson::TJsonValue& bassResponse, TBassResponseRenderer& bassRenderer) const override {
        bassRenderer.Render(TEMPLATE_MUSIC_PLAY, ALICE_SHOW, bassResponse, ALICE_SHOW_ANALYTICS_INTENT);
    }

    const TString& ProductScenarioName() const override {
        return NAlice::NProductScenarios::ALICE_SHOW;
    }
};

} // namespace

std::unique_ptr<TMusicHardcodedIntent> CreateMusicHardcodedIntent(TRTLogger& logger, const TScenarioRunRequestWrapper& request) {
    const auto& input = request.Input();

    if (const auto frame = input.FindSemanticFrame(MEDITATION_INTENT); frame && !request.HasExpFlag(EXP_MM_ENABLE_MEDITATION)) {
        LOG_INFO(logger) << "Chosen the meditation intent";
        if (const auto slot = GetSlot(*frame, MEDITATION_TYPE_SLOT)) {
            return std::make_unique<TMeditationIntent>(logger, slot.GetRef().GetValue());
        }
        return std::make_unique<TMeditationIntent>(logger, MEDITATION_TYPE_DEFAULT_VALUE);
    }

    if (const auto frame = input.FindSemanticFrame(ALICE_SHOW_INTENT)) {
        if (const auto slot = GetSlot(*frame, SLOT_SHOW_TYPE)) {
            const auto value = slot.GetRef().GetValue();
            if (IsIn({"morning", "children"}, value)) {
                LOG_INFO(logger) << "Chosen the alice_show intent (" << value << ")";
                return std::make_unique<TAliceShowIntent>(logger);
            }
            if (value == "evening") {
                LOG_INFO(logger) << "Chosen the alice_show intent (" << value << ") under flag";
                return std::make_unique<TAliceShowIntent>(logger);
            }
        } else {
            LOG_INFO(logger) << "Chosen the alice_show intent (common)";
            return std::make_unique<TAliceShowIntent>(logger);
        }
    }

    if (const auto frame = input.FindSemanticFrame(LITE_HARDCODED_MUSIC_PLAY_INTENT); frame) {
        LOG_INFO(logger) << "Got lite hardcoded music play intent";
        auto liteIntent = std::make_unique<TLiteHardcodedPlaylistIntent>(logger, request.ClientInfo().PromoType);
        if (!liteIntent->IsIrrelevant()) {
            return std::move(liteIntent);
        } else {
            LOG_INFO(logger) << "Lite playlist intent is irrelevant by promo type";
        }
    }

    if (input.FindSemanticFrame(ALICE_SHOW_INTENT_GOOD_MORNING) && HasMusicSubscription(request)) {
        LOG_INFO(logger) << "Chosen the alice_show intent (morning)";
        return std::make_unique<TAliceShowIntent>(logger);
    }
    if (input.FindSemanticFrame(ALICE_SHOW_INTENT_GOOD_EVENING) && HasMusicSubscription(request) &&
        request.HasExpFlag(EXP_HW_ENABLE_EVENING_SHOW_GOOD_EVENING)
    ) {
        LOG_INFO(logger) << "Chosen the alice_show intent (evening)";
        return std::make_unique<TAliceShowIntent>(logger);
    }

    return {};
}

} // namespace NAlice::NHollywood::NMusic
