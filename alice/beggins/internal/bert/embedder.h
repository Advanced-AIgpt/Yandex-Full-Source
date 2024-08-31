#pragma once

#include <kernel/search_query/cnorm.h>

#include <quality/relev_tools/bert_models/input_parsing_lib/input_parsing.h>
#include <quality/relev_tools/bert_models/lib/eval_context.h>
#include <quality/relev_tools/bert_models/lib/compressions/compressions.h>
#include <quality/relev_tools/bert_models/lib/multi_tool.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <memory>
#include <utility>

namespace NAlice::NBeggins::NInternal {

class TBertEmbedder {
public:
    using TBertTokenizer = THolder<NBertModels::TMultiTool>;
    using TBertModel = THolder<NBertModels::TMultiTool>;

public:
    TBertEmbedder(TBertTokenizer tokenizer, TBertModel model)
        : BertTokenizer(std::move(tokenizer))
        , BertModel(std::move(model)) {
        Y_ENSURE(BertTokenizer, "BertTokenizer must not be null");
        Y_ENSURE(BertModel, "BertModel must not be null");
    }

    NBertModels::NProto::TExportableDocResult Process(const TStringBuf query) const {
        return Process(NBertModels::TTextualDocumentRepresentation{
            {"QueryBertNormed", NCnorm::BNorm(query)},
            {"BaseRegionNamesRus_BetaBertNormed", "россия"}, // TODO: think about region
        });
    }

    static TVector<float> ExtractEmbeddings(const NBertModels::NProto::TExportableDocResult& result) {
        TVector<float> embeddings;
        for(const auto& part : result.GetDataParts()) {
            if (part.GetDataTag() == NBertModels::NProto::EDocumentResultTag::QueryEmbed) {
                embeddings = NBertModels::DeCompressEmbed(part);
                break;
            }
        }
        // TODO: raise error on invalid embeddings
        return embeddings;
    }

private:
    NBertModels::NProto::TExportableDocResult Process(NBertModels::TTextualDocumentRepresentation&& data) const {
        auto preprocessedData = Preprocess(std::move(data));
        NBertModels::TMetaGraphEvaluationMutableContext context(/* maxDocsNum= */ 1);
        const auto docId = context.AddDocument();
        BertModel->UpdateDocumentInContext(docId, std::move(preprocessedData), context);
        BertModel->DoRunOverContext(context);
        return BertModel->ExtractExportableDocResult(docId, context);
    }

    NBertModels::NProto::TExportableDocResult Preprocess(NBertModels::TTextualDocumentRepresentation&& data) const {
        NBertModels::TMetaGraphEvaluationMutableContext context(/* maxDocsNum= */ 1);
        const auto docId = context.AddDocumentByTextInputs(std::move(data));
        BertTokenizer->DoRunOverContext(context);
        return BertTokenizer->ExtractExportableDocResult(docId, context);
    }

private:
    TBertTokenizer BertTokenizer;
    TBertModel BertModel;
};

std::unique_ptr<TBertEmbedder> LoadEmbedder(TBlob file, const NBertModels::TTechnicalOpenModelOptions& options = {});

} // namespace NAlice::NBeggins::NInternal
