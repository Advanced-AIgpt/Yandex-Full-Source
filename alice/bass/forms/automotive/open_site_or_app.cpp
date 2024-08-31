#include "open_site_or_app.h"
#include "url_build.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/navigation/fixlist.h>
#include <alice/bass/forms/urls_builder.h>

#include <util/generic/set.h>
#include <util/string/cast.h>

namespace  NBASS {
namespace  NAutomotive {

// нам нужно переопределить только 1 (одно) приложение
namespace {
const TSet<TStringBuf> FM_RADIO_QUERY = {"фм радио", "радио", "радио фм"};
}

TResultValue HandleOpenSiteOrApp(TContext& ctx, TStringBuf target) {
    TStringBuf app;
    if (FM_RADIO_QUERY.contains(target)) {
        app = "radio";
    } else {
        TMaybe<NAlice::TNavigationFixList::TNativeApp> data = TNavigationFixList::Instance()->FindNativeApp(ToString(target));
        if (data && !data->Empty(ctx.MetaClientInfo())) {
            app = target;
        }
        else {
            const NSc::TValue& data = TNavigationFixList::Instance()->Find(ToString(target), ctx);
            const NSc::TValue& yaAuto = data["nav"]["app"]["yaauto"];
            // не нашли подходяшего приложения
            if (yaAuto.IsNull()) {
                ctx.AddAttention("nav_not_supported_on_device", {} /* value */);
                return TResultValue();
            }
            app = yaAuto.GetString();
        }
    }
    NSc::TValue result;
    result["text"].SetString();
    result["tts"].SetString();
    auto answer = ctx.CreateSlot(TStringBuf("navigation_results"), TStringBuf("navigation_results"));
    answer->Value.Swap(result);

    TDirectiveBuilder<TAutoLaunchDirective> builder(TStringBuf("launch"));
    builder.InsertParam(ToString("name"), ToString(app));

    return builder.AddDirective(ctx);
}

} // namespace NAutomotive
} // namespace NBASS
