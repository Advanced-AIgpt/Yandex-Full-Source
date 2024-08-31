#pragma once

#include "xml_resp_parser.h"

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/datetime/base.h>

namespace NAlice::NHollywood::NMusic{

namespace NImpl {

TString CalculateSignatureToken(const TXmlRespParseResult& result, TStringBuf secret);
TString CalculateHlsSignatureToken(const TStringBuf trackId, TInstant ts, const TStringBuf secret);

} // namespace NImpl

TString CalculateSignatureToken(const TXmlRespParseResult& result);
TString CalculateHlsSignatureToken(const TStringBuf trackId, TInstant ts);

} // namespace NAlice::NHollywood::NMusic
