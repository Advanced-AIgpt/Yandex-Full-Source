#pragma once

#include <search/begemot/rules/internal/locale/proto/locale.pb.h>
#include <util/generic/fwd.h>

namespace NBg {

    void NormalizePhrase(const TStringBuf phrase, const NProto::TLocaleResult& ctx,
                         TString* normalizedPhrase, TVector<TString>* normalizedTokens);

} // namespace NBg
