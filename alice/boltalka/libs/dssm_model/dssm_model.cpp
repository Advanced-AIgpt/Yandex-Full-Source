#include "dssm_model.h"

#include <util/string/join.h>
#include <library/cpp/json/json_writer.h>

namespace NNlg {

namespace {

static TVector<TString> GetInputFields(size_t contextLength) {
    TVector<TString> result(contextLength + 1);
    for (size_t i = 0; i < contextLength; ++i) {
        result[i] = "context_" + ToString(contextLength - i - 1);
    }
    result.back() = "reply";
    return result;
}

}

TDssmModel::TDssmModel(const TString& modelFilename)
    : TDssmModel(NDssm3::TModelFactory<float>::Load(modelFilename))
{
}

TDssmModel::TDssmModel(NDssm3::TModelFactoryPtr<float> modelFactory)
{
    SampleReader = modelFactory->Subparams[0]->SampleProcessor;
    IsAggregatingModel = SampleReader.QueryFields[0] == "context";
    size_t contextLength = SampleReader.QueryFields.size();
    SetNumTurns = MakeHolder<NNlgTextUtils::TSetContextNumTurns>(contextLength);
    if (IsAggregatingModel) {
        SampleReader = NDssm3::TSampleProcessor(SampleReader.QueryFields, SampleReader.DocFields, {"context", "reply"});
    } else {
        auto inputFields = GetInputFields(contextLength);
        SampleReader = NDssm3::TSampleProcessor(SampleReader.QueryFields, SampleReader.DocFields, inputFields);
    }
    Model = modelFactory->CreateModel();
}

TVector<TVector<float>> TDssmModel::Fprop(const TVector<TString>& context, const TString& reply, const TVector<TString>& output) const {
    return FpropBatch({context}, {reply}, output);
}

TVector<TVector<float>> TDssmModel::FpropBatch(const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, const TVector<TString>& output) const {
    Y_VERIFY(contexts.size() == replies.size());
    const size_t batchSize = contexts.size();
    TVector<NDssm3::TSample> batch(batchSize);
    for (size_t i = 0; i < batchSize; ++i) {
        auto context = SetNumTurns->Transform(contexts[i]);
        for (auto& turn : context) {
            turn = ReplacePunct.Transform(turn);
        }
        std::reverse(context.begin(), context.end());
        auto reply = ReplacePunct.Transform(replies[i]);

        TString sampleString;
        if (IsAggregatingModel) {
            TStringStream builder;
            {
                NJson::TJsonWriter writer(&builder, false);
                writer.OpenMap();
                writer.OpenArray("context");
                for (auto& turn : context) {
                    writer.Write(turn);
                }
                writer.CloseArray();
                writer.CloseMap();
            }
            sampleString = builder.Str() + "\t" + reply;
        } else {
            sampleString = JoinSeq("\t", context) + "\t" + reply;
        }
        SampleReader.Sample(sampleString, &batch[i]);
    }

    auto fropResult = Model->ApplyFprop(batch, output);
    TVector<TVector<float>> result;
    for (const auto* matrix : fropResult) {
        result.emplace_back(matrix->data(), matrix->data() + matrix->size());
    }
    return result;
}

}

