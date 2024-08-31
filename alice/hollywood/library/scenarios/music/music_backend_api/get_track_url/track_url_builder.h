#pragma once

#include "xml_resp_parser.h"

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NMusic {

TString BuildTrackUrl(const TStringBuf trackId, const TStringBuf from, const TStringBuf uid,
                      const TXmlRespParseResult& dlInfoXml);

} // namespace NAlice::NHollywood::NMusic
