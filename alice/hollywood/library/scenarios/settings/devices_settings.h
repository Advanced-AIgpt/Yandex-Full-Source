#pragma once

#include "context.h"

namespace NAlice::NHollywood::NSettings {

enum class ESettingObject {
    Tandem = 0 /* "tandem" */,
    SmartSpeaker = 1 /* "smart_speaker" */,
};

class TDevicesSettings {
public:
    TDevicesSettings(const TSettingsRunContext& ctx)
        : Ctx(ctx)
    {
    }

    bool HandleFrame(const TOpenTandemSettingSemanticFrame&);
    bool HandleFrame(const TOpenSmartSpeakerSettingSemanticFrame&);

private:
    void RenderTandemSettingScreen(const ESettingObject settingObject) const;
    void RenderTandemSettingStory(const TString& url) const;
    void RenderTandemSettingTextAndVoice(const ESettingObject settingObject) const;
    void RenderTandemPushDirective() const;
    void RenderAddSmartSpeakerScreen(const TString& url) const ;
    void RenderCantOpenDevicesSettingsScreen() const;
    void RenderNoAuth() const;

private:
    const TSettingsRunContext& Ctx;
};


} // namespace NAlice::NHollywood
