#include "embedder.h"

#include <quality/relev_tools/bert_models/lib/one_file_package.h>

namespace NAlice::NBeggins::NInternal {

std::unique_ptr<TBertEmbedder> LoadEmbedder(TBlob file, const NBertModels::TTechnicalOpenModelOptions& options) {
    auto evaluationModel = NBertModels::ReadPackedModel(file,
                                                        options,
                                                        /* layout= */ "BegemotPartWithoutTokenization");
    auto tokenizationModel =
        evaluationModel->ReOpen(evaluationModel->GetConfig(), /* layout= */ "BegemotPartTokenization");
    return std::make_unique<TBertEmbedder>(std::move(tokenizationModel), std::move(evaluationModel));
}

} // namespace NAlice::NBeggins::NInternal
