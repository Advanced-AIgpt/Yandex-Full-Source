#pragma once

#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NVideo {

    TMaybe<TString> GetMdsUrlInfo(const TString& s);
    TMaybe<TString> FindEmbedUrl(const TString& link);
    bool HasNetloc(const TString& url);
    TString FixSchema(const TString& url);

} // namespace NAlice::NHollywood::NVideo
