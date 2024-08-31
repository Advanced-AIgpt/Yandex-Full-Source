#pragma once

#include <alice/boltalka/generative/service/server/handlers/ptune_storage.h>
#include <alice/boltalka/generative/service/server/handlers/request_handler.h>

#include <alice/boltalka/generative/service/proto/common.pb.h>
#include <alice/boltalka/generative/service/proto/embedding_request.pb.h>
#include <alice/boltalka/generative/service/proto/embedding_response.pb.h>
#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>
#include <alice/boltalka/generative/service/proto/phead.pb.h>
#include <alice/boltalka/generative/service/proto/scoring_request.pb.h>
#include <alice/boltalka/generative/service/proto/scoring_response.pb.h>

#include <alice/boltalka/generative/inference/core/external_info.h>
#include <alice/boltalka/generative/inference/core/model.h>

#include "library/cpp/string_utils/base64/base64.h"


namespace NGenerativeBoltalka {

namespace {
TVector<TVector<float>> ParsePtuneEmbeddings(const Proto::TPtuneEmbeddings& proto) {
    auto bytes = Base64Decode(proto.GetEmbeddings());
    size_t bytesLen = bytes.size() / proto.GetPrecision();
    TVector<float> res(bytesLen);
    if (proto.GetPrecision() == 2) {
        NFloat16Ops::UnpackFloat16SequenceAuto(
            reinterpret_cast<const TFloat16*>(bytes.data()),
            res.data(),
            res.size());
    } else if (proto.GetPrecision() == 4) {
        const float* tmp = reinterpret_cast<const float*>(bytes.data());
        res = {tmp, tmp + bytesLen};
    } else {
        ythrow yexception() << "Invalid p-tune embeddings precision: either 2 or 4 should be used";
    }
    TVector<TVector<float>> result;
    result.reserve(proto.GetNumTokens());
    auto rowSize = bytesLen / proto.GetNumTokens();
    for (auto i = res.begin(); i < res.end(); i += rowSize) {
        result.emplace_back(i, i + rowSize);
    }
    return result;
}

std::shared_ptr<TVector<TVector<float>>> GetPtuneFromRequest(const Proto::TPtuneEmbeddings& proto, TPtuneStorage* storage) {
    if (auto ytPath = proto.GetYtPath()) {
        return storage->Get(ytPath, TPtuneStorage::YT).PtuneEmbeddings;
    } else if (auto s3ObjectName = proto.GetS3ObjectName()) {
        return storage->Get(s3ObjectName, TPtuneStorage::S3).PtuneEmbeddings;
    } else {
        return std::make_shared<TVector<TVector<float>>>(ParsePtuneEmbeddings(proto));
    }
}

} // namespace anonymous


template <typename TModelType, typename TConfigType>
class TGenerativeProtoRequestHandler : public IRequestHandler<Proto::TGenerativeRequest, Proto::TGenerativeResponse> {
public:
    TGenerativeProtoRequestHandler(
        const TConfigType& generativeBoltalkaParams,
        bool ignoreSeed, size_t forceNumHypos, TPtuneStorage* ptuneStorage);
    TGenerativeProtoRequestHandler(
        const TModelType& generativeBoltalka,
        bool ignoreSeed, size_t forceNumHypos, TPtuneStorage* ptuneStorage);

    Proto::TGenerativeResponse HandleRequest(const Proto::TGenerativeRequest& request) override;

private:
    TModelType GenerativeBoltalka;
    bool IgnoreSeed;
    size_t ForceNumHypos;
    TPtuneStorage* PtuneStorage;
};

class TScoringProtoRequestHandler : public IRequestHandler<Proto::TScoringRequest, Proto::TScoringResponse> {
public:
    TScoringProtoRequestHandler(
        const TGenerativeBoltalka::TParams& generativeBoltalkaParams,
        TPtuneStorage* ptuneStorage);
    TScoringProtoRequestHandler(
        const TGenerativeBoltalka& generativeBoltalka,
        TPtuneStorage* ptuneStorage);

    Proto::TScoringResponse HandleRequest(const Proto::TScoringRequest& request) override;

private:
    TGenerativeBoltalka GenerativeBoltalka;
    TPtuneStorage* PtuneStorage;
};

class TEmbeddingProtoRequestHandler : public IRequestHandler<Proto::TEmbeddingRequest, Proto::TEmbeddingResponse> {
public:
    TEmbeddingProtoRequestHandler(
        const TGenerativeBoltalka::TParams& generativeBoltalkaParams,
        TPtuneStorage* ptuneStorage);
    TEmbeddingProtoRequestHandler(
        const TGenerativeBoltalka& generativeBoltalka,
        const TGenerativeBoltalka::TParams& params,
        TPtuneStorage* ptuneStorage);

    Proto::TEmbeddingResponse HandleRequest(const Proto::TEmbeddingRequest& request) override;

private:
    TGenerativeBoltalka GenerativeBoltalka;
    TPtuneStorage* PtuneStorage;
    bool AddMask, AddSep, DoReverseContext;
};

class TPHeadProtoRequestHandler : public IRequestHandler<Proto::TPHeadRequest, Proto::TPHeadResponse> {
public:
    TPHeadProtoRequestHandler(
        const TGenerativeBoltalka::TParams& generativeBoltalkaParams,
        TPtuneStorage* ptuneStorage);
    TPHeadProtoRequestHandler(
        const TGenerativeBoltalka& generativeBoltalka,
        TPtuneStorage* ptuneStorage);

    Proto::TPHeadResponse HandleRequest(const Proto::TPHeadRequest& request) override;

private:
    TGenerativeBoltalka GenerativeBoltalka;
    TPtuneStorage* PtuneStorage;
};

class TExternalInfoGenerativeProtoRequestHandler : public IRequestHandler<Proto::TGenerativeRequest, Proto::TGenerativeResponse> {
public:
    TExternalInfoGenerativeProtoRequestHandler(
        const TExternalInfoGenerativeBoltalka::TParams& externalInfoGenerativeBoltalkaParams,
        bool ignoreSeed, size_t forceNumHypos);
    TExternalInfoGenerativeProtoRequestHandler(
        const TExternalInfoGenerativeBoltalka& externalInfoGenerativeBoltalka,
        bool ignoreSeed, size_t forceNumHypos);

    Proto::TGenerativeResponse HandleRequest(const Proto::TGenerativeRequest& request) override;

private:
    TExternalInfoGenerativeBoltalka ExternalInfoGenerativeBoltalka;
    bool IgnoreSeed;
    size_t ForceNumHypos;
};


// implementing template classes here

template<typename TModelType, typename TConfigType>
TGenerativeProtoRequestHandler<TModelType, TConfigType>::TGenerativeProtoRequestHandler(
    const TConfigType& generativeBoltalkaParams,
    bool ignoreSeed,
    size_t forceNumHypos,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(TModelType(generativeBoltalkaParams))
    , IgnoreSeed(ignoreSeed)
    , ForceNumHypos(forceNumHypos)
    , PtuneStorage(ptuneStorage)
    {}

template<typename TModelType, typename TConfigType>
TGenerativeProtoRequestHandler<TModelType, TConfigType>::TGenerativeProtoRequestHandler(
    const TModelType& generativeBoltalka,
    bool ignoreSeed,
    size_t forceNumHypos,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(generativeBoltalka)
    , IgnoreSeed(ignoreSeed)
    , ForceNumHypos(forceNumHypos)
    , PtuneStorage(ptuneStorage)
    {}


template<typename TModelType, typename TConfigType>
Proto::TGenerativeResponse TGenerativeProtoRequestHandler<TModelType, TConfigType>::HandleRequest(const Proto::TGenerativeRequest& request) {
    TVector<TString> context;
    for (const auto& val : request.GetContext()) {
        context.push_back(val);
    }

    Y_ENSURE(context.size() > 0, "Context must not be empty");

    TMaybe<ui64> seed = Nothing();
    if (!IgnoreSeed && request.HasSeed()) {
        seed = request.GetSeed().GetValue();
    }

    TVector<TString> spanDelimiters;
    for (const auto& spanDelimiter : request.GetSpanDelimiters()) {
        spanDelimiters.push_back(spanDelimiter);
    }

    std::shared_ptr<TVector<TVector<float>>> ptuneEmbeddingsPtr;
    if (request.HasPtuneEmbeddings()) {
        ptuneEmbeddingsPtr = GetPtuneFromRequest(request.GetPtuneEmbeddings(), PtuneStorage);
    }

    size_t numHypos = 1;
    if (request.GetNumHypos() > 0) {
        numHypos = static_cast<size_t>(request.GetNumHypos());
    }

    if (ForceNumHypos > 0) {
        numHypos = ForceNumHypos;
    }

    TGenerativeRequest generativeRequest = {
            .Context = context,
            .Seed = seed,
            .NumHypos = numHypos,
            .PrefixOnly = request.GetPrefixTokensOnly(),
            .SpanDelimiters = spanDelimiters,
            .DiverseBeamSearch = request.GetDiverseBeamSearch(),
            .PtuneEmbeddings = ptuneEmbeddingsPtr.get()
    };
    if (request.HasMinOutLen()) {
        generativeRequest.MinOutLen = request.GetMinOutLen();
    }
    if (request.HasMaxOutLen()) {
        generativeRequest.MaxOutLen = request.GetMaxOutLen();
    }

    if (request.HasSamplerParams()) {
        const auto& requestSamplerParams = request.GetSamplerParams();
        auto& samplerParams = generativeRequest.SamplerParams.ConstructInPlace();

        if (requestSamplerParams.HasMode()) {
            samplerParams.Mode = FromString<NDict::NMT::TSamplerParams::EMode>(requestSamplerParams.GetMode());
        }
        if (requestSamplerParams.HasTemperature()) {
            samplerParams.Temperature = requestSamplerParams.GetTemperature();
        }
        if (requestSamplerParams.HasTopK()) {
            samplerParams.TopNLogits = requestSamplerParams.GetTopK();
        }
        if (requestSamplerParams.HasNucleus()) {
            samplerParams.NucleusSampling = requestSamplerParams.GetNucleus();
        }
    }

    Proto::TGenerativeResponse response;
    if (GenerativeBoltalka.CheckRequestHasBadWords(generativeRequest)) {
        response.SetBadWordInRequest(true);
        return response;
    }

    const auto responses = GenerativeBoltalka.GenerateResponses(generativeRequest);
    for (const auto& r : responses) {
        auto result = response.add_responses();
        result->SetResponse(r.Response);
        result->SetNumTokens(r.NumTokens);
        result->SetScore(r.Score);
        if (r.ExternalInfo.Defined()) {
            result->SetExternalInfo(r.ExternalInfo.GetRef());
        }
    }

    return response;
}

} // namespace NGenerativeBoltalka
