#include "music_announce.h"

#include <alice/hollywood/library/scenarios/settings/common/names.h>

#include <alice/protos/data/scenario/music/config.pb.h>

namespace NMemento = ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood::NSettings {

namespace {

constexpr TStringBuf MUSIC_ANNOUNCE_NLG = "music_announce";
constexpr TStringBuf RENDER_MUSIC_ANNOUNCE_DISABLE = "announce_disable";
constexpr TStringBuf RENDER_MUSIC_ANNOUNCE_ENABLE = "announce_enable";
constexpr TStringBuf RENDER_MUSIC_UNSUPPORTED = "unsupported";

bool AddUserConfigs(TMementoChangeUserObjectsDirective& mementoDirective, NMemento::EConfigKey key, const NProtoBuf::Message& value) {
    NMemento::TConfigKeyAnyPair mementoConfig;
    mementoConfig.SetKey(key);
    if (!mementoConfig.MutableValue()->PackFrom(value)) {
        return false;
    }
    *mementoDirective.MutableUserObjects()->AddUserConfigs() = std::move(mementoConfig);
    return true;
}

bool UpdateMementoMusicConfig(const TSettingsRunContext& ctx, const NData::NMusic::TUserConfig& config) {
    TMementoChangeUserObjectsDirective mementoDirective;
    if (!AddUserConfigs(mementoDirective, NMemento::EConfigKey::CK_MUSIC, config)) {
        LOG_ERROR(ctx.Logger) << "Failed to add MusicConfig";
        return false;
    }
    *ctx.BodyBuilder.GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective() = std::move(mementoDirective);

    auto& analyticsBuilder = ctx.BodyBuilder.GetAnalyticsInfoBuilder();
    if (config.GetAnnounceTracks()) {
        analyticsBuilder.AddAction("alice.music.announce.enable",
                                   "music announce enabled",
                                   "анонс треков включен");
    } else {
        analyticsBuilder.AddAction("alice.music.announce.disable",
                                   "music announce disabled",
                                   "анонс треков выключен");
    }
    return true;
}

void RenderPhrase(const TSettingsRunContext& ctx, const TStringBuf phrase, bool changed = false) {
    TNlgData nlgData{ctx.NlgData};
    nlgData.Context["changed"] = changed;
    ctx.BodyBuilder.AddRenderedTextAndVoice(MUSIC_ANNOUNCE_NLG, phrase, nlgData);
}

} // namespace

bool TMusicAnnounce::SetAnnounceEnabled(const bool enable) {
    if (!Ctx.Request.HasExpFlag(EXP_HW_MUSIC_ANNOUNCE)) {
        return false;
    }
    if (!GetUid(Ctx.Request)) {
        Ctx.BodyBuilder.AddRenderedTextAndVoice(SCENARIO_NAME, RENDER_UNAUTHORIZED, Ctx.NlgData);
        return true;
    }

    if (!Ctx.Request.Interfaces().GetHasAudioClient()) {
        RenderPhrase(Ctx, RENDER_MUSIC_UNSUPPORTED);
        return true;
    }

    auto config = Ctx.Request.BaseRequestProto().GetMemento().GetUserConfigs().GetMusicConfig();
    const bool changed = (enable != config.GetAnnounceTracks());
    if (changed) {
        config.SetAnnounceTracks(enable);
        if (!UpdateMementoMusicConfig(Ctx, config)) {
            return false;
        }
    }

    RenderPhrase(Ctx, enable ? RENDER_MUSIC_ANNOUNCE_ENABLE : RENDER_MUSIC_ANNOUNCE_DISABLE, changed);
    return true;
}

} // namespace NAlice::NHollywood
