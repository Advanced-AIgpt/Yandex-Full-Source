#pragma once

#include "resources.h"
#include <alice/nlg/library/runtime_api/translations.h>

namespace NAlice::NHollywood {

class TNlgTranslationsResource final : public IResourceContainer {
public:
    NNlg::ITranslationsContainerPtr GetTranslationsContainer() const {
        return TranslationsContainer_;
    }

    void LoadFromPath(const TFsPath& dirPath) override;
private:
    NNlg::ITranslationsContainerPtr TranslationsContainer_;
};

}
