#pragma once

#include "context.h"

namespace NAlice::NHollywood::NSettings {

class TMusicAnnounce {
public:
    TMusicAnnounce(const TSettingsRunContext& ctx)
        : Ctx(ctx)
    {}

    bool HandleFrame(const TMusicAnnounceEnableSemanticFrame&) {
        return SetAnnounceEnabled(true);
    }
    bool HandleFrame(const TMusicAnnounceDisableSemanticFrame&) {
        return SetAnnounceEnabled(false);
    }

private:
    const TSettingsRunContext& Ctx;

private:
    bool SetAnnounceEnabled(bool enabled);
};

} // namespace NAlice::NHollywood
