#pragma once
#include "context_transform.h"
#include "rnn_model.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlgServer {

class TTokenDict : public TThrRefBase {
public:
    TTokenDict(const TFsPath &path);

    ui64 GetTokenIdx(TWtringBuf token) const;
    ui64 GetTokenOrUnkIdx(TWtringBuf token) const;
    const TUtf16String& GetToken(ui64 idx) const;
    bool HasToken(TWtringBuf token) const;
    size_t GetSize() const;

private:
    THashMap<TWtringBuf, ui64> Token2Idx;
    TVector<TUtf16String> Tokens;
    ui64 UnkIdx;
};
using TTokenDictPtr = TIntrusivePtr<TTokenDict>;

class INlgModel : public TThrRefBase {
public:
    virtual ~INlgModel() = default;
    virtual TUtf16String GetReply(const TUtf16String &context, ui64 maxLen, float temperature, TComputationContextPtr ctx) const = 0;
};
using INlgModelPtr = TIntrusivePtr<INlgModel>;

class TRnnNlgModel : public INlgModel {
public:
    TRnnNlgModel(TEncoderDecoderPtr encoderDecoder, TTokenDictPtr dict, IContextTransformPtr contextTransform);
    TUtf16String GetReply(const TUtf16String &context, ui64 maxLen, float temperature, TComputationContextPtr ctx) const;

private:
    TEncoderDecoderPtr EncoderDecoder;
    TTokenDictPtr Dict;
    IContextTransformPtr ContextTransform;
};

/*class TEnsembleRnnModel : public INlgModel {
public:
    TEnsembleRnnModel(TVector<TRnnModelSPtr> rnnModels, TVector<double> weights);
    TVector<TUtf16String> GetReplies(const TUtf16String &context, ui64 maxLen, double temperature, ui64 numSamples, ui64 workerId) const override;

private:
    TVector<TRnnModelSPtr> RnnModels;
    TVector<double> Weights;
};*/

}
