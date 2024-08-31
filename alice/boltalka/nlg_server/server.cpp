#include "server.h"
#include "context_transform.h"
#include "thread_pool.h"
#include "rnn_model.h"
#include "dssm_model.h"

#include <util/string/vector.h>
#include <util/system/file.h>

namespace NNlgServer {

struct TParsedRequest {
    TString Url;
    TCgiParameters CgiParams;

    TParsedRequest(TStringBuf url)
        : Url(url.After('/').Before('?'))
        , CgiParams(url.Contains('?') ? url.After('?') : "")
    {
    }
};

TNlgServer::TNlgServer(int port, int threadCount)
    : Port(port)
    , ThreadCount(threadCount)
{
}

void TNlgServer::LoadModels(const TFsPath &modelDir, TStringBuf mode) {
    Cout << "Loading models for " << mode << "..." << Endl;
    NlgModels.clear();
    // name, transform, samplingSubNetIdx
    TVector<std::tuple<TString, IContextTransformPtr, ui64>> rnnModelsToLoad;
    // name, transform, contextFeaturesLayerName
    TVector<std::tuple<TString, IContextTransformPtr, TString>> dssmModelsToLoad;
    if (mode == "south_park") {
        IContextTransformPtr separatePunctuation = new TSeparatePunctuation();
        IContextTransformPtr translate = new TTranslateWithDict(modelDir / "slang_acronyms_misspellings.dict");
        IContextTransformPtr limitNumTokens = new TLimitNumTokens(100);
        IContextTransformPtr speakersTransform = new TCompoundTransform({ separatePunctuation, translate, new TAddCartman(), limitNumTokens });
        IContextTransformPtr allSpeakersTransform = new TCompoundTransform({ separatePunctuation, translate, new TAddKyleAndCartman(), limitNumTokens });

        const ui64 samplingSubNetIdx = 1;
        rnnModelsToLoad = {
            { "encoder_decoder_sp_speakers", speakersTransform, samplingSubNetIdx },
            { "encoder_decoder_sp_allspeakers", allSpeakersTransform, samplingSubNetIdx },
            { "encoder_decoder_sp_speakers_myname", speakersTransform, samplingSubNetIdx },
            { "encoder_decoder_sp_allspeakers_myname", allSpeakersTransform, samplingSubNetIdx }
        };
    } else if (mode == "general_conversation") {
        IContextTransformPtr separatePunctuation = new TSeparatePunctuation();
        IContextTransformPtr limitNumTokens = new TLimitNumTokens(300);
        IContextTransformPtr transform = new TCompoundTransform({ separatePunctuation, limitNumTokens });

        const ui64 samplingSubNetIdx = 1;
        rnnModelsToLoad = {
            { "ed_tw_c3", transform, samplingSubNetIdx },
            { "ed_tw_cx", transform, samplingSubNetIdx },
            { "ed_tw_cx_from_subs", transform, samplingSubNetIdx }
        };

        IContextTransformPtr dssmSeparatePunctuation = new TSeparatePunctuation(/*lowerCase*/ true, /*eosString*/ u"\t");
        const TString contextFeaturesLayerName = "features_query";
        dssmModelsToLoad = {
            { "dssm_tw_c1", new TCompoundTransform({ new TLimitContextLength(1), dssmSeparatePunctuation }), contextFeaturesLayerName },
            { "dssm_tw_c2", new TCompoundTransform({ new TLimitContextLength(2), dssmSeparatePunctuation }), contextFeaturesLayerName },
            { "dssm_tw_c2_filtered", new TCompoundTransform({ new TLimitContextLength(2), dssmSeparatePunctuation }), contextFeaturesLayerName },
            { "dssm_tw_c2_librusec", new TCompoundTransform({ new TLimitContextLength(2), dssmSeparatePunctuation }), contextFeaturesLayerName }
        };
    } else {
        Y_FAIL("Unsupported mode %s", mode.data());
    }

    for (const auto &tuple : rnnModelsToLoad) {
        const auto &modelName = std::get<0>(tuple);
        const auto &transform = std::get<1>(tuple);
        ui64 samplingSubNetIdx = std::get<2>(tuple);
        TFsPath dir = modelDir / modelName;
        if (!dir.Exists()) {
            continue;
        }
        NlgModels[modelName] = new TRnnModel(dir, transform, samplingSubNetIdx, ThreadCount);
    }
    for (const auto &tuple : dssmModelsToLoad) {
        const auto &modelName = std::get<0>(tuple);
        const auto &transform = std::get<1>(tuple);
        const auto &contextFeaturesLayerName = std::get<2>(tuple);
        TFsPath dir = modelDir / modelName;
        if (!dir.Exists()) {
            continue;
        }
        NlgModels[modelName] = new TDssmModel(dir, transform, contextFeaturesLayerName, ThreadCount);
    }

    Cout << "Loaded " << NlgModels.size() << " models:" << Endl;
    for (const auto &pair : NlgModels) {
        const TString &modelName = pair.first;
        Cout << '\t' << modelName << Endl;
    }
}

static TUtf16String EscapeNewLine(const TUtf16String &s) {
    TUtf16String result;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\n') {
            result += u" _EOL_ ";
        } else {
            result += s[i];
        }
    }
    return result;
}

void TNlgServer::Respond(SOCKET socket, TCgiParameters params, ui64 workerId) {
    Cout << "I'M " << workerId << Endl;
    TUtf16String context = UTF8ToWide(params.Get("context"));
    Cout << workerId << " GOT: " << EscapeNewLine(context) << Endl;

    TString modelName = params.Get("model");
    if (!NlgModels.contains(modelName)) {
        HttpReplyPlainText(socket, "");
        return;
    }
    INlgModelSPtr model = NlgModels[modelName];

    ui64 maxLen = FromString<ui64>(params.Get("max_len"));
    float temperature = FromString<float>(params.Get("temperature"));
    ui64 numSamples = FromString<ui64>(params.Get("num_samples"));

    try {
        TVector<TUtf16String> replies = model->GetReplies(context, maxLen, temperature, numSamples);
        TString response = WideToUTF8(JoinStrings(replies, u"\n"));
        HttpReplyPlainText(socket, response.data());
        Cout << workerId << " REPLY: " << replies.front() << Endl;
        Cout << workerId << " OK" << Endl;
    } catch (const yexception &e) {
        Cout << e.what() << Endl;
        std::abort();
    }
}

void TNlgServer::HandleRequests() {
    TNLHttpServer server(Port);

    TThreadPool taskQueue(ThreadCount);

    Cout << "Waiting for requests..." << Endl;

    while (!server.IsQuit()) {
        THttpRequest request;
        SOCKET socket = server.Accept(&request, 250);
        if (socket == INVALID_SOCKET) {
            continue;
        }
        TParsedRequest parsedRequest(request.GetUrl());
        Cout << "Received request: " << parsedRequest.Url << Endl;

        if (parsedRequest.Url == "respond") {
            taskQueue.Add([this, socket, parsedRequest](int workerId) {
                Respond(socket, parsedRequest.CgiParams, workerId);
            });
        }
    }
}

}

