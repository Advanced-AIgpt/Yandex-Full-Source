#pragma once

#include <library/cpp/langs/langs.h>

#include <util/generic/variant.h>

#include <memory>

namespace NAlice::NNlg {

namespace NPrivate {

TString NormalizeInflectionCase(const TString& inflCase);
TString NormalizeInflectionCases(const TString& cases);

}  // namespace NPrivate

class IInflector {
public:
    virtual ~IInflector();

    virtual TString Inflect(const TString& target, const TString& cases) const = 0;
};

std::unique_ptr<IInflector> CreateNormalInflector();
std::unique_ptr<IInflector> CreateFioInflector();

TString PluralizeWords(const IInflector& inflector,
                       const TString& text,
                       std::variant<double, ui64> number,
                       const TString& inflCase,
                       ELanguage lang = ::LANG_RUS);
TString SingularizeWords(TStringBuf text, ui64 number, ELanguage lang = ::LANG_RUS);

}  // namespace NAlice::NNlg
