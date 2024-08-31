#pragma once

#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/fwd.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

TFrameNluHint MakeOrdinalNluHint(
    const TStringBuf name,
    const TStringBuf ordinalName,
    const size_t displayPosition,
    const ELang lang = L_RUS);

TFrameNluHint MakeChooseContactNluHint(
    const TStringBuf name,
    const TVector<TString>& tokens,
    const TStringBuf ordinalName,
    const size_t displayPosition,
    const ELang lang = L_RUS,
    const bool makeOrdinal = true);

TFrameNluHint MakeOpenAddressBookNluHint();

TFrameNluHint MakeFrameNluHint(const TStringBuf frameName);

} // namespace NAlice::NHollywood
