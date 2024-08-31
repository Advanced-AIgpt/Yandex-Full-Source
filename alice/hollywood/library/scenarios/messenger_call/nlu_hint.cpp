#include "nlu_hint.h"

#include <alice/megamind/protos/common/frame.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf OPEN_ADDRES_BOOK_HINT_FRAME = "alice.phone_call.open_address_book";

} // namespace

TFrameNluHint MakeOrdinalNluHint(
    const TStringBuf name,
    const TStringBuf ordinalName,
    const size_t displayPosition,
    const ELang lang)
{
    TFrameNluHint nluHint;
    nluHint.SetFrameName(TString{name});

    auto* hintInstance = nluHint.AddInstances();
    hintInstance->SetPhrase(ToString(displayPosition + 1));
    hintInstance->SetLanguage(lang);

    hintInstance = nluHint.AddInstances();
    hintInstance->SetPhrase(TString{ordinalName});
    hintInstance->SetLanguage(lang);

    return nluHint;
}

TFrameNluHint MakeChooseContactNluHint(
    const TStringBuf name,
    const TVector<TString>& tokens,
    const TStringBuf ordinalName,
    const size_t displayPosition,
    const ELang lang,
    const bool makeOrdinal)
{
    TFrameNluHint nluHint;
    if (makeOrdinal) {
        nluHint = MakeOrdinalNluHint(name, ordinalName, displayPosition, lang);
    } else {
        nluHint.SetFrameName(TString{name});
    }

    for (const auto& token : tokens) {
        auto& hintInstance = *nluHint.AddInstances();
        hintInstance.SetPhrase(token);
        hintInstance.SetLanguage(lang);
    }

    return nluHint;
}

TFrameNluHint MakeOpenAddressBookNluHint() {
    return MakeFrameNluHint(OPEN_ADDRES_BOOK_HINT_FRAME);
}

TFrameNluHint MakeFrameNluHint(const TStringBuf frameName) {
    TFrameNluHint nluHint;
    nluHint.SetFrameName(TString{frameName});

    return nluHint;
}

} // namespace NAlice::NHollywood
