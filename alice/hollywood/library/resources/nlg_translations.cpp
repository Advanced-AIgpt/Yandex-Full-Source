#include "nlg_translations.h"

namespace NAlice::NHollywood {

void TNlgTranslationsResource::LoadFromPath(const TFsPath& dirPath) {
    const auto nlgTranslationsFilePath = dirPath / "nlg_translations.json";
    Y_ENSURE(nlgTranslationsFilePath.Exists(), "File not found: " << nlgTranslationsFilePath);

    TranslationsContainer_ = NNlg::CreateTranslationsContainerFromFile(nlgTranslationsFilePath);
}

}
