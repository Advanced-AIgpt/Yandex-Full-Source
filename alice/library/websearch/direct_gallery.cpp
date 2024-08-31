#include "direct_gallery.h"

#include <alice/library/experiments/flags.h>
#include <alice/library/experiments/experiments.h>

using namespace NAlice::NExperiments;

namespace NAlice::NDirectGallery {

namespace {

constexpr TStringBuf FLAG_EXPID_FOR_SEARCH = "direct_expid_for_search";
constexpr TStringBuf FLAG_DIRECT_PAGE = "direct_page";
constexpr TStringBuf FLAG_DIRECT_EXPID = "exp_flags";
constexpr TStringBuf DEFAULT_DIRECT_PAGEID = "467415";
constexpr TStringBuf ALICE_CONFIRM_DIRECT_PAGE_ID = "620060";

} // namespace

bool CanShowDirectGallery(const TClientInfo& clientInfo, const THashMap<TString, TMaybe<TString>>& experiments) {
    return HasExpFlag(experiments, WEBSEARCH_ENABLE_DIRECT_GALLERY) ||
        (!HasExpFlag(experiments, WEBSEARCH_DISABLE_DIRECT_GALLERY) &&
            (clientInfo.IsSearchApp() || clientInfo.IsYaBrowser()));
}

bool CanShowDirectGallery(const TClientFeatures& features) {
    return CanShowDirectGallery(
        features,
        features.Experiments().GetRawExpFlags()
    );
}

TCgiParameters MakeDirectExperimentCgi(const TExpFlags& flags) {
    TCgiParameters cgi;

    TStringBuf pageId =
        flags.Has(WEBSEARCH_DISABLE_CONFIRM_DIRECT_HIT) ? DEFAULT_DIRECT_PAGEID : ALICE_CONFIRM_DIRECT_PAGE_ID;
    flags.OnEachFlag([&pageId, &cgi](TStringBuf flag) {
        if (flag.StartsWith(FLAG_EXPID_FOR_SEARCH)) {
            const TStringBuf expId = flag.After('=');
            cgi.InsertUnescaped(FLAG_DIRECT_EXPID, TString::Join("direct_raw_parameters=experiment-id=", expId));
        } else if (flag.StartsWith(FLAG_DIRECT_PAGE)) {
            pageId = flag.After('=');
        }
    });
    cgi.InsertUnescaped(FLAG_DIRECT_PAGE, pageId);
    return cgi;
}

} // namespace NAlice::NDirectGallery
