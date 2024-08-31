#include <alice/boltalka/generative/service/proto/bert_request.pb.h>
#include <alice/boltalka/generative/service/proto/bert_response.pb.h>
#include <alice/boltalka/generative/service/proto/embedding_request.pb.h>
#include <alice/boltalka/generative/service/proto/embedding_response.pb.h>
#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>
#include <alice/boltalka/generative/service/proto/phead.pb.h>
#include <alice/boltalka/generative/service/proto/scoring_request.pb.h>
#include <alice/boltalka/generative/service/proto/scoring_response.pb.h>
#include <alice/boltalka/generative/service/server/handlers/bert_factor_request_handler.h>
#include <alice/boltalka/generative/service/server/handlers/proto_request_handler.h>
#include <alice/boltalka/generative/service/server/handlers/ptune_storage.h>
#include <alice/boltalka/generative/service/server/lib/utils.h>

#include <alice/boltalka/generative/inference/core/external_info.h>
#include <alice/boltalka/generative/inference/core/model.h>

#include <alice/scenarios/lib/http_service.h>
#include <alice/library/logger/logger.h>

#include <dict/mt/libs/nn/ynmt/config_helper/model_reader.h>
#include <dict/mt/libs/nn/ynmt/extra/encoder_head.h>


using namespace NGenerativeBoltalka;
using NDict::NMT::NYNMT::TRegressionHead;
using NDict::NMT::NYNMT::TClassificationHead;
using NDict::NMT::NYNMT::TMultitargetHead;


template<typename ModelType, typename ConfigType>
void RegisterGenerativeHandler(NAlice::TProtoBasedHttpService& service, TGenerativeProtoRequestHandler<ModelType, ConfigType>& handler, const TString& handlerPath) {
    service.RegisterHandler<Proto::TGenerativeRequest, Proto::TGenerativeResponse>(
            handlerPath,
            [&handler](const Proto::TGenerativeRequest& request, NAlice::TRTLogger&) {
                return handler.HandleRequest(request);
            }
    );
}

void RegisterScoringHandler(NAlice::TProtoBasedHttpService& service, TScoringProtoRequestHandler& handler, const TString& handlerPath) {
    service.RegisterHandler<Proto::TScoringRequest, Proto::TScoringResponse>(
            handlerPath,
            [&handler](const Proto::TScoringRequest& request, NAlice::TRTLogger&) {
                return handler.HandleRequest(request);
            }
    );
}

void RegisterEmbeddingHandler(NAlice::TProtoBasedHttpService& service, TEmbeddingProtoRequestHandler& handler, const TString& handlerPath) {
    service.RegisterHandler<Proto::TEmbeddingRequest, Proto::TEmbeddingResponse>(
            handlerPath,
            [&handler](const Proto::TEmbeddingRequest& request, NAlice::TRTLogger&) {
                return handler.HandleRequest(request);
            }
    );
}

void RegisterPHeadHandler(NAlice::TProtoBasedHttpService& service, TPHeadProtoRequestHandler& handler, const TString& handlerPath) {
    service.RegisterHandler<Proto::TPHeadRequest, Proto::TPHeadResponse>(
            handlerPath,
            [&handler](const Proto::TPHeadRequest& request, NAlice::TRTLogger&) {
                return handler.HandleRequest(request);
            }
    );
}

void RegisterExternalInfoGenerativeHandler(NAlice::TProtoBasedHttpService& service, TExternalInfoGenerativeProtoRequestHandler& handler,
                                           const TString& handlerPath) {
    service.RegisterHandler<Proto::TGenerativeRequest, Proto::TGenerativeResponse>(
            handlerPath,
            [&handler](const Proto::TGenerativeRequest& request, NAlice::TRTLogger&) {
                return handler.HandleRequest(request);
            }
    );
}

void RegisterBertFactorHandler(NAlice::TProtoBasedHttpService& service, TBaseBertRequestHandler& handler, const TString& handlerPath) {
    service.RegisterHandler<Proto::TBertFactorRequest, Proto::TBertFactorResponse>(
            handlerPath,
            [&handler](const Proto::TBertFactorRequest& request, NAlice::TRTLogger&) {
                return handler.HandleRequest(request);
            }
    );
}

size_t GetPTuneTokensNum(const TString& modelPath) {
    auto modelBlob = TBlob::FromFile(modelPath);
    auto proto = NDict::NMT::NYNMT::ReadModelProtoWithMeta(modelBlob);
    if (proto.Meta.Has("NumberOfAddedPtuningVectors")) {
        return static_cast<size_t>(proto.Meta["NumberOfAddedPtuningVectors"].AsInt());
    }
    return 0;
}


int main(int argc, const char *argv[]) {
    const TConfig config{LoadConfig(argc, argv)};
    NAlice::TRTLogClient rtLogClient{config.GetRTLog()};
    NAlice::TProtoBasedHttpService service{config.GetHttpServerConfig(), rtLogClient};
    const auto storageConfig = config.GetPtuneStorage();
    THashMap<TString, TString> prechargedPtunes{};
    for (const auto& pair : storageConfig.GetPtunePrecharged()) {
        prechargedPtunes[pair.GetYtPath()] = pair.GetLocalPath();
    }
    TPtuneStorage ptuneStorage{storageConfig.GetSize(), prechargedPtunes};

    for (auto& boltalkaConfig : config.GetGenerativeBoltalkas()) {
        auto generativeBoltalkaParams = ParseBoltalkaParams(boltalkaConfig);
        TGenerativeBoltalka generativeBoltalka = TGenerativeBoltalka(generativeBoltalkaParams);

        if (generativeBoltalkaParams.ModelParams.ShouldInitializeTranslator) {
            auto* handler = new TGenerativeProtoRequestHandler<TGenerativeBoltalka, TGenerativeBoltalka::TParams>(generativeBoltalka, boltalkaConfig.GetIgnoreSeed(), boltalkaConfig.GetForceNumHypos(), &ptuneStorage);
            RegisterGenerativeHandler<TGenerativeBoltalka, TGenerativeBoltalka::TParams>(service, *handler, boltalkaConfig.GetServiceEndpointSuffixGenerative());
        }

        if (generativeBoltalkaParams.ModelParams.ShouldInitializeScorer) {
            auto* handler = new TScoringProtoRequestHandler(generativeBoltalka, &ptuneStorage);
            RegisterScoringHandler(service, *handler, boltalkaConfig.GetServiceEndpointSuffixScoring());
        }

        if (generativeBoltalkaParams.ModelParams.ShouldInitializeEmbedder) {
            auto* handler = new TEmbeddingProtoRequestHandler(generativeBoltalka, generativeBoltalkaParams, &ptuneStorage);
            RegisterEmbeddingHandler(service, *handler, boltalkaConfig.GetServiceEndpointSuffixEmbedding());
        }

        if (generativeBoltalkaParams.ModelParams.ShouldInitializePHead) {
            auto* handler = new TPHeadProtoRequestHandler(generativeBoltalka, &ptuneStorage);
            RegisterPHeadHandler(service, *handler, boltalkaConfig.GetServiceEndpointSuffixPHead());
        }
    }

    for (auto& externalInfoBoltalkaConfig : config.GetExternalInfoGenerativeBoltalkas()) {
        auto generatorBoltalkaConfig = externalInfoBoltalkaConfig.GetGeneratorConfig();

        auto embedderConfig = ParseBoltalkaParams(externalInfoBoltalkaConfig.GetEmbedderConfig());
        auto retrieverConfig = THnswRetriever::TParams{
            externalInfoBoltalkaConfig.GetRetrieverConfig().GetPathPrefix(),
            externalInfoBoltalkaConfig.GetRetrieverConfig().GetDocsPath(),
            externalInfoBoltalkaConfig.GetRetrieverConfig().GetSearchNeighborhoodSize(),
            externalInfoBoltalkaConfig.GetRetrieverConfig().GetDistanceCalcLimit()
        };
        auto generatorConfig = ParseBoltalkaParams(generatorBoltalkaConfig);

        TExternalInfoGenerativeBoltalka externalInfoGenerativeBoltalka = TExternalInfoGenerativeBoltalka(TExternalInfoGenerativeBoltalka::TParams{
            .EmbedderParams = embedderConfig,
            .RetrieverParams = retrieverConfig,
            .GeneratorParams = generatorConfig,
            .NumTopDocs = externalInfoBoltalkaConfig.GetNumTopDocs(),
            .NumHypsFromDoc = externalInfoBoltalkaConfig.GetNumHypsFromDoc()
        });

        auto* generativeHandler = new TGenerativeProtoRequestHandler<TExternalInfoGenerativeBoltalka, TExternalInfoGenerativeBoltalka::TParams>(
            externalInfoGenerativeBoltalka, generatorBoltalkaConfig.GetIgnoreSeed(), generatorBoltalkaConfig.GetForceNumHypos(), &ptuneStorage
        );
        RegisterGenerativeHandler<TExternalInfoGenerativeBoltalka, TExternalInfoGenerativeBoltalka::TParams>(service, *generativeHandler, generatorBoltalkaConfig.GetServiceEndpointSuffixGenerative());
    }

    for (auto& bertFactorConfig : config.GetBertFactors()) {
        TBaseBertRequestHandler* handler = nullptr;
        switch (bertFactorConfig.GetEncoderHead()) {
            case TConfig_TBertFactor_EEncoderHead_Regression:
                handler = new TBertRequestHandler<TRegressionHead>(bertFactorConfig);
                break;
            case TConfig_TBertFactor_EEncoderHead_Classification:
                handler = new TBertRequestHandler<TClassificationHead>(bertFactorConfig);
                break;
            case TConfig_TBertFactor_EEncoderHead_Multitarget:
                handler = new TBertRequestHandler<TMultitargetHead>(bertFactorConfig);
                break;
            default:
                Y_ENSURE(false);
        }
        RegisterBertFactorHandler(service, *handler, bertFactorConfig.GetServiceEndpointSuffix());
    }
    service.Start();
    service.Wait();
    return 0;
}
