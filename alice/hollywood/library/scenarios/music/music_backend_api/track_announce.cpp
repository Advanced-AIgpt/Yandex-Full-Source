#include "track_announce.h"

#include "music_common.h"
#include "result_renders.h"
#include "shots.h"

#include <alice/protos/data/scenario/music/config.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf DJ_SHOT = "dj_shot";

void AnnounceTrack(TRTLogger& logger, TApplyResponseBuilder& builder, const TNlgData& nlgData) {
    LOG_INFO(logger) << "Rendering track announcement, nlg context=" << nlgData.Context;
    auto& bodyBuilder = builder.GetOrCreateResponseBodyBuilder();
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "track_announce", {}, nlgData);
}

bool AnnounceEnabledByConfig(const TScenarioBaseRequestWrapper& request) {
    const auto& config = request.BaseRequestProto().GetMemento().GetUserConfigs().GetMusicConfig();
    return config.GetAnnounceTracks() && request.HasExpFlag(EXP_HW_MUSIC_ANNOUNCE);
}

class TPhraseRef {
private:
    const TPhraseGroup::TPhrase& Phrase;
    const TStringBuf Tag;

private:
    static TStringBuf FindHashTag(const TVector<TStringBuf>& tags) {
        const auto tagPtr = FindIfPtr(tags, [](TStringBuf tag) { return tag.StartsWith('#'); });
        return tagPtr ? *tagPtr : TStringBuf{};
    }

public:
    TPhraseRef(const TPhraseGroup::TPhrase& phrase, const TVector<TStringBuf>& tags)
        : Phrase(phrase)
        , Tag(FindHashTag(tags))
    {}

    TStringBuf GetText() const {
        return Phrase.GetText();
    }
    TStringBuf GetVoice() const {
        return Phrase.GetVoice() ?: Phrase.GetText();
    }
    TStringBuf GetTag() const {
        return Tag;
    }
};

NJson::TJsonValue ChooseShot(const TMusicShotsFastData& shots, IRng& rng, const TMusicQueueWrapper& mq, bool firstPlay,
                             const bool announceShot) {
    NJson::TJsonValue result{NJson::JSON_MAP};

    if (!announceShot) {
        return result;
    }

    TProtoEvaluator protoEvaluator;
    protoEvaluator.SetProtoRef("Item", mq.CurrentItem());
    protoEvaluator.SetProtoRef("ContentId", mq.ContentId());
    protoEvaluator.SetProtoRef("ContentInfo", mq.ContentInfo());
    protoEvaluator.SetParameterValue("FirstPlay", ToString(firstPlay));

    TTagEvaluator tagEvaluator{shots.GetTagConditionCollection(), protoEvaluator};
    const auto tagChecker = [&](const TStringBuf tag) { return tagEvaluator.CheckTag(tag); };

    if (!tagChecker("dj_shot_allowed")) {
        return result;
    }

    TVector<TPhraseRef> phrases;
    const auto consumer = [&](auto&& phrase, auto&& tags, double probability) {
        if (probability < 1 && rng.RandomDouble() >= probability) {
            return false;
        }
        phrases.emplace_back(phrase, tags);
        return true;
    };
    shots.GetPhraseCollection().FindPhrases(DJ_SHOT, tagChecker, consumer);

    if (!phrases.empty()) {
        const auto& phrase = phrases[rng.RandomInteger(phrases.size())];
        result["text"] = phrase.GetText();
        result["voice"] = phrase.GetVoice();
        result["tag"] = phrase.GetTag();
    }

    return result;
}

} // namespace

void FillMusicAnswer(TNlgData& nlgData, const TScenarioBaseRequestWrapper& request, const TQueueItem& currentItem,
                          const TContentId& contentId) {
    auto& answerValue = nlgData.Context.InsertValue("answer", MakeMusicAnswer(currentItem, contentId));
    if (IsThinClientShotPlaying(request)) {
        answerValue["subtype"] = "shot";
    }
}

bool TryFillMusicAnswer(TNlgData& nlgData, const TScenarioBaseRequestWrapper& request, const TMusicQueueWrapper& mq) {
    if (!mq.HasCurrentItem()) {
        return false;
    }
    FillMusicAnswer(nlgData, request, mq.CurrentItem(), mq.ContentId());
    return true;
}

bool TryAnnounceTrack(TRTLogger& logger, const NHollywood::TScenarioApplyRequestWrapper& request,
                      TApplyResponseBuilder& builder, const TMusicQueueWrapper& mq, const TMusicContext& mCtx,
                      const TMusicShotsFastData& shots, IRng& rng) {
    if (!AnnounceEnabledByConfig(request) || mq.HasShotsEnabled()) {
        return false;
    }
    if (const auto bodyBuilder = builder.GetResponseBodyBuilder(); bodyBuilder && bodyBuilder->HasListenDirective()) {
        // skip the announce if asking something
        return false;
    }
    TNlgData nlgData{logger, request};
    if (!TryFillMusicAnswer(nlgData, request, mq)) {
        return false;
    }
    nlgData.Context["first_play"] = mCtx.GetFirstPlay();
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    nlgData.Context["announce_album"] = request.HasExpFlag(EXP_HW_MUSIC_ANNOUNCE_ALBUM);
    nlgData.Context[DJ_SHOT] = ChooseShot(shots, rng, mq, mCtx.GetFirstPlay(),
                                          request.HasExpFlag(EX_HW_MUSIC_ANNOUNCE_SHOT));

    AnnounceTrack(logger, builder, nlgData);
    return true;
}

} // namespace NAlice::NHollywood::NMusic
