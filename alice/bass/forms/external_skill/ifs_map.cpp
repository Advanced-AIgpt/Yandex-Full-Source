#include "ifs_map.h"

#include <alice/bass/forms/context/context.h>

namespace NBASS {
namespace NExternalSkill {

bool DeviceSupportsAccountLinking(const TContext& ctx) {
    if (ctx.MetaClientInfo().IsYaBrowserDesktop())
        return ctx.HasExpFlag(EXPERIMENTAL_FLAG_ENABLE_ACCOUNT_LINKING_ON_DESKTOP_BROWSER);

    return ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsSearchApp() || ctx.MetaClientInfo().IsYaBrowserMobile();
}

bool DeviceSupportsBilling(const TContext& ctx) {
    return ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsSearchApp();
}

// JFYI: https://wiki.yandex-team.ru/dialogs/development/station/#poleinterfaces
NSc::TValue CreateHookInterfaces(const TContext& ctx) {
    const TClientInfo& client = ctx.MetaClientInfo();
    NSc::TValue interfaces;

    if (client.IsSmartSpeaker() ||
        client.IsElariWatch() ||
        client.IsNavigator() ||
        client.IsYaAuto()) {
        interfaces.SetDict();
    } else {
        interfaces["screen"].SetDict();
    }

    if (DeviceSupportsAccountLinking(ctx)) {
        interfaces["account_linking"].SetDict();
    }
    if (DeviceSupportsBilling(ctx)) {
        interfaces["payments"].SetDict();
    }

    return interfaces;
}

} // NExternalSkill
} // NBASS
