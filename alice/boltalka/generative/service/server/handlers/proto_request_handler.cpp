#include "proto_request_handler.h"

#include <util/string/cast.h>

namespace NGenerativeBoltalka {

TScoringProtoRequestHandler::TScoringProtoRequestHandler(
    const TGenerativeBoltalka::TParams& generativeBoltalkaParams,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(TGenerativeBoltalka(generativeBoltalkaParams))
    , PtuneStorage(ptuneStorage)
    {}

TScoringProtoRequestHandler::TScoringProtoRequestHandler(
    const TGenerativeBoltalka& generativeBoltalka,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(generativeBoltalka)
    , PtuneStorage(ptuneStorage)
    {}

Proto::TScoringResponse TScoringProtoRequestHandler::HandleRequest(const Proto::TScoringRequest& request) {
    TVector<TString> context{};
    for (const auto& val : request.GetContext()) {
        context.push_back(val);
    }
    if (auto s = request.GetPrefix()) {
        if (!context.empty()) {
            ythrow yexception() << "Use either only Context or Prefix/Suffix fields";
        }
        context.push_back(s);
    }
    if (auto s = request.GetSuffix()) {
        context.push_back(s);
    }

    std::shared_ptr<TVector<TVector<float>>> ptuneEmbeddingsPtr;
    if (request.HasPtuneEmbeddings()) {
        Y_ENSURE(context.size() == 2, "Context with 2 strings should be provided with p-tune model.");
        ptuneEmbeddingsPtr = GetPtuneFromRequest(request.GetPtuneEmbeddings(), PtuneStorage);
    }

    const auto scores = GenerativeBoltalka.GenerateScores(context, ptuneEmbeddingsPtr.get(), request.GetForceEos());

    Proto::TScoringResponse response;

    for (const auto& r : scores) {
        *response.add_segments()->mutable_scores() = {r.begin(), r.end()};
    }
    return response;
}

TEmbeddingProtoRequestHandler::TEmbeddingProtoRequestHandler(
    const TGenerativeBoltalka::TParams& generativeBoltalkaParams,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(TGenerativeBoltalka(generativeBoltalkaParams))
    , PtuneStorage(ptuneStorage)
    , AddMask(generativeBoltalkaParams.EmbeddingsAddMask)
    , AddSep(generativeBoltalkaParams.EmbeddingsAddSep)
    , DoReverseContext(generativeBoltalkaParams.EmbeddingsDoReverseContext)
    {}


TEmbeddingProtoRequestHandler::TEmbeddingProtoRequestHandler(
    const TGenerativeBoltalka& generativeBoltalka,
    const TGenerativeBoltalka::TParams& params,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(generativeBoltalka)
    , PtuneStorage(ptuneStorage)
    , AddMask(params.EmbeddingsAddMask)
    , AddSep(params.EmbeddingsAddSep)
    , DoReverseContext(params.EmbeddingsDoReverseContext)
    {}

Proto::TEmbeddingResponse TEmbeddingProtoRequestHandler::HandleRequest(const Proto::TEmbeddingRequest& request) {
    TVector<TString> context{};
    for (const auto& val : request.GetContext()) {
        context.push_back(val);
    }

    std::shared_ptr<TVector<TVector<float>>> ptuneEmbeddingsPtr;
    if (request.HasPtuneEmbeddings()) {
        ptuneEmbeddingsPtr = GetPtuneFromRequest(request.GetPtuneEmbeddings(), PtuneStorage);
    }

    const auto embeddings = GenerativeBoltalka.GenerateEmbed(
        context,
        ptuneEmbeddingsPtr.get(),
        nullptr,
        AddMask,
        AddSep,
        DoReverseContext
    );

    Proto::TEmbeddingResponse response;

    *response.mutable_embedding() = {embeddings.begin(), embeddings.end()};

    return response;
}

TPHeadProtoRequestHandler::TPHeadProtoRequestHandler(
    const TGenerativeBoltalka::TParams& generativeBoltalkaParams,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(TGenerativeBoltalka(generativeBoltalkaParams))
    , PtuneStorage(ptuneStorage)
    {
        Y_VERIFY(generativeBoltalkaParams.ModelParams.HeadMode == NDict::NMT::NYNMT::EHeadMode::Linear);
    }

TPHeadProtoRequestHandler::TPHeadProtoRequestHandler(
    const TGenerativeBoltalka& generativeBoltalka,
    TPtuneStorage* ptuneStorage)
    : GenerativeBoltalka(generativeBoltalka)
    , PtuneStorage(ptuneStorage)
    {}

Proto::TPHeadResponse TPHeadProtoRequestHandler::HandleRequest(const Proto::TPHeadRequest& request) {
    TVector<TString> context{};
    context.push_back(request.GetText());

    std::vector<float> embeddings;
    if (auto path = request.GetPtuneYtPath()) {
        auto model = PtuneStorage->Get(path, TPtuneStorage::YT);
        embeddings = GenerativeBoltalka.GenerateEmbed(context, model.PtuneEmbeddings.get(), model.Proto.get());
    } else if (auto s3ObjectName = request.GetPtuneS3ObjectName()) {
        auto model = PtuneStorage->Get(s3ObjectName, TPtuneStorage::S3);
        embeddings = GenerativeBoltalka.GenerateEmbed(context, model.PtuneEmbeddings.get(), model.Proto.get());
    } else {
        embeddings = GenerativeBoltalka.GenerateEmbed(context);
    }

    Proto::TPHeadResponse response;
    *response.MutableScores() = {embeddings.begin(), embeddings.end()};
    return response;
}

} // namespace NGenerativeBoltalka
