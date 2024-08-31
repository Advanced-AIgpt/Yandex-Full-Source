#pragma once

#include <util/generic/string.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NMusic {

struct TXmlRespParseResult {
    TString Host;
    TString Path;
    TString Ts;
    TString Region;
    TString Signature;
};

TMaybe<TXmlRespParseResult> ParseDlInfoXmlResp(const TStringBuf xmlStr);

} //namespace NAlice::NHollywood::NMusic
