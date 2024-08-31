#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/folder/fwd.h>

namespace NAlice::NNlg {

struct ITranslationsContainer {
    virtual ~ITranslationsContainer() = default;
    virtual const TString& GetTranslation(const ELanguage language, const TStringBuf key) const = 0;
};

using ITranslationsContainerPtr = std::shared_ptr<ITranslationsContainer>;

ITranslationsContainerPtr CreateTranslationsContainerFromFile(const TFsPath& nlgTranslationsJsonFilePath);

}
