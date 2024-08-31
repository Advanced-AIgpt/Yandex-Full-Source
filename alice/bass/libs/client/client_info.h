#pragma once

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <alice/library/client/client_info.h>
#include <alice/library/client/client_features.h>

#include <alice/bass/libs/request/request.sc.h>

namespace NBASS {
/**
 * A wrapper for information about client application
 */

using TMeta = NBASSRequest::TMetaConst<TSchemeTraits>;

struct TClientInfo : public NAlice::TClientInfo {
    TClientInfo(TStringBuf clientId);
    TClientInfo(const TMeta& requestMetaScheme);

    TString UserAgent;
};

struct TClientFeatures : public NAlice::TClientFeatures {
    TClientFeatures(const TMeta& requestMetaScheme, const NSc::TValue& flags);
};
}
