#include "navigator_intent.h"
#include <util/string/subst.h>

#include <alice/bass/forms/directives.h>

namespace NBASS {

namespace {
TString PrintNaviCgi(const TCgiParameters& cgi) {
    TString cgiStr = cgi.Print();
    SubstGlobal(cgiStr, "+", "%20"); // Some strange decoding of '+' symbol in yandexnavi app
    return cgiStr;
}

void ProcessAdditionalRememberLocation(TContext& context, const TCgiParameters& cgi, TStringBuf scheme) {
    const TContext::TSlot* needRememberLocation = context.GetSlot("need_remember_location");
    if (!needRememberLocation ||
        (scheme != "build_route_on_map") ||
        !(cgi.Has(TStringBuf("lat_to")) &&
          cgi.Has(TStringBuf("lon_to")) &&
          cgi.Has(TStringBuf("confirmation")))) {
        return;
    }

    NSc::TValue namedLocationInfo = needRememberLocation->Value;
    if (namedLocationInfo.TrySelect("/locationName").IsNull() ||
        namedLocationInfo.TrySelect("/slotWhat").IsNull() ||
        namedLocationInfo.TrySelect("/slotWhere").IsNull()) {
        return;
    }

    if ((TStringBuf("what_to") != namedLocationInfo.TrySelect("/slotWhat")) ||
        (TStringBuf("where_to") != namedLocationInfo.TrySelect("/slotWhere"))) {
        return;
    }

    TCgiParameters params;
    params.InsertUnescaped(TStringBuf("lat"), cgi.Get(TStringBuf("lat_to")));
    params.InsertUnescaped(TStringBuf("lon"), cgi.Get(TStringBuf("lon_to")));
    params.InsertUnescaped(TStringBuf("type"), namedLocationInfo.TrySelect("locationName"));
    TStringBuilder navigatorIntent;
    navigatorIntent << TStringBuf("yandexnavi://set_place");
    navigatorIntent << "?" << PrintNaviCgi(params);

    NSc::TValue intentData;
    intentData["uri"].SetString(navigatorIntent);
    context.AddCommand<TNavigatorSetPlaceDirective>(TStringBuf("open_uri"), intentData);
}

} // end anon namespace

TResultValue INavigatorIntent::Do() {
    if (TResultValue err = SetupSchemeAndParams()) {
        return err;
    }

    if (Scheme.empty()) {
        return TResultValue();
    }

    TString navigatorIntent = BuildIntentString();
    NSc::TValue intentData;
    intentData["uri"].SetString(navigatorIntent);
    ProcessAdditionalRememberLocation(Context, Params, Scheme);
    const auto analyticsDirectiveIndex = GetDirectiveIndex();

    if (ListeningIsPossible) {
        intentData["listening_is_possible"].SetBool(true);
    }
    Context.AddCommand(TStringBuf("open_uri"), analyticsDirectiveIndex, intentData);

    return TResultValue();
}

TString INavigatorIntent::BuildIntentString() const {
    TStringBuilder navigatorIntent;
    navigatorIntent << "yandexnavi://" << Scheme;
    if (!Params.empty()) {
        navigatorIntent << "?" << PrintNaviCgi(Params);
    }

    return navigatorIntent;
}

void INavigatorIntent::AddShortAnswerBlock() {
    const TStringBuf EXP_FLAG_SHORT_ANSWER = "ya_auto_short_answer";
    if (Context.MetaClientInfo().IsYaAuto() && Context.HasExpFlag(EXP_FLAG_SHORT_ANSWER)) {
        NSc::TValue data;
        data["card_tag"] = "short_answer";
        Context.AddTextCardBlock("ya_auto_short_answer", data);
    }
}

}
