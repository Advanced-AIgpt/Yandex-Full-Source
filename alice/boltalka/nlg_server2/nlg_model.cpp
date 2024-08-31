#include "nlg_model.h"

#include <util/charset/wide.h>
#include <util/stream/file.h>

namespace NNlgServer {

namespace {

static TUtf16String OneHotToText(const TVector<ui64> &oneHot, TTokenDictPtr dict) {
    TUtf16String result;
    for (ui64 idx : oneHot) {
        if (!result.empty()) {
            result += ' ';
        }
        TUtf16String token = dict->GetToken(idx);
        result += token;
        if (token == u"_EOS_") {
            break;
        }
    }
    return result;
}

static TVector<ui64> TextToOneHot(TWtringBuf text, TTokenDictPtr dict) {
    TVector<ui64> result;
    for (TWtringBuf tok; text.NextTok(' ', tok); ) {
        result.push_back(dict->GetTokenOrUnkIdx(tok));
    }
    return result;
}

}

TTokenDict::TTokenDict(const TFsPath &path) {
    TFileInput in(path);
    TUtf16String line;
    for (ui64 idx = 0; in.ReadLine(line); ++idx) {
        Tokens.push_back(line);
        Token2Idx[Tokens.back()] = idx;
    }
    Y_VERIFY(Tokens.size() == Token2Idx.size());
    Y_VERIFY(Token2Idx.contains(u"_UNK_"));
    UnkIdx = Token2Idx[u"_UNK_"];
}

ui64 TTokenDict::GetTokenIdx(TWtringBuf token) const {
    Y_ASSERT(HasToken(token));
    return Token2Idx.find(token)->second;
}

ui64 TTokenDict::GetTokenOrUnkIdx(TWtringBuf token) const {
    auto it = Token2Idx.find(token);
    return it == Token2Idx.end() ? UnkIdx : it->second;
}

const TUtf16String& TTokenDict::GetToken(ui64 idx) const {
    Y_ASSERT(idx < Tokens.size());
    return Tokens[idx];
}

bool TTokenDict::HasToken(TWtringBuf token) const {
    return Token2Idx.contains(token);
}

size_t TTokenDict::GetSize() const {
    Y_ASSERT(Tokens.size() == Token2Idx.size());
    return Tokens.size();
}

TRnnNlgModel::TRnnNlgModel(TEncoderDecoderPtr encoderDecoder, TTokenDictPtr dict, IContextTransformPtr contextTransform)
    : EncoderDecoder(encoderDecoder)
    , Dict(dict)
    , ContextTransform(contextTransform)
{
}

TUtf16String TRnnNlgModel::GetReply(const TUtf16String &context, ui64 maxLen, float temperature, TComputationContextPtr ctx) const {
    TUtf16String newContext = ContextTransform->Transform(context);
    TVector<ui64> seed = TextToOneHot(newContext, Dict);
    TVector<ui64> sample = EncoderDecoder->GetSample(seed, maxLen, temperature, ctx);
    TUtf16String reply = OneHotToText(sample, Dict);
    Cout << "Context: " << newContext << " Reply: " << reply << Endl;
    return reply;
}

/*TEnsembleRnnModel::TEnsembleRnnModel(TVector<TRnnModelSPtr> rnnModels, TVector<double> weights)
    : RnnModels(std::move(rnnModels))
    , Weights(std::move(weights))
{
    NN_VERIFY(!RnnModels.empty());
    NN_VERIFY(RnnModels.size() == Weights.size());
    for (const auto &rnnModel : RnnModels) {
        NN_VERIFY(rnnModel->GetSamplingParams().CanBeEnsembledWith(RnnModels.front()->GetSamplingParams()));
    }
}

TVector<TUtf16String> TEnsembleRnnModel::GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples, ui64 workerId) const {
    TVector<TUtf16String> result;
    for (const auto &sample : samples) {
        result.push_back(OneHotToText(sample, dict));
    }
    return result;
}*/

}

