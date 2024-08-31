#include "navigator_intent.h"

#include <alice/hollywood/library/scenarios/search/scenarios/ellipsis_intents.h>

#include <alice/library/json/json.h>

#include <util/string/subst.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NSearch {

namespace {

TString PrintNaviCgi(const TCgiParameters& cgi) {
    TString cgiStr = cgi.Print();
    SubstGlobal(cgiStr, "+", "%20"); // Some strange decoding of '+' symbol in yandexnavi app
    return cgiStr;
}

} // namespace

bool TNavigatorScenario::Do(const TSearchResult& /* response */) {
    if (Ctx.GetRequest().Interfaces().GetHasNavigator()) {
        TCgiParameters params;
        params.InsertUnescaped(TStringBuf("text"), Ctx.GetQuery());
        if (const auto searchPos = InitGeoPositionFromRequest(Ctx.GetRequest().BaseRequestProto())) {
            params.InsertUnescaped(TStringBuf("ll"), searchPos->GetLonLatString());
        }

        const TString intentString = TString::Join("yandexnavi://map_search?", PrintNaviCgi(params));
        NJson::TJsonValue data;
        data["url"] = intentString;

        Ctx.SetResultSlot(TStringBuf("map_search_url"), data);
        Ctx.AddSuggest(TStringBuf("search__show_on_map"), false);
        AddShowOnMapButton(Ctx, intentString);
        return true;
    }
    return false;
}

} // namespace NAlice::NHollywood::NSearch
