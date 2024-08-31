#include "server.h"
#include "thread_pool.h"

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
    , Ctxs(threadCount)
{
    for (auto &ctx : Ctxs) {
        ctx = new TComputationContext();
    }
}

void TNlgServer::LoadModels(const TFsPath &modelDir, TStringBuf mode) {
    Cout << "Loading models for " << mode << "..." << Endl;
    NlgModels.clear();
    TVector<std::tuple<TString, IContextTransformPtr, TTokenDictPtr, bool>> modelsToLoad;
    if (mode == "south_park") {
        IContextTransformPtr separatePunctuation = new TSeparatePunctuation();
        IContextTransformPtr translate = new TTranslateWithDict(modelDir / "slang_acronyms_misspellings.dict");
        IContextTransformPtr limitNumTokens = new TLimitNumTokens(100);
        IContextTransformPtr speakersTransform = new TCompoundTransform({ separatePunctuation, translate, new TAddCartman(), limitNumTokens });
        IContextTransformPtr allSpeakersTransform = new TCompoundTransform({ separatePunctuation, translate, new TAddKyleAndCartman(), limitNumTokens });

        TTokenDictPtr dict = new TTokenDict(modelDir / "sp_and_subs.speakers.dict");

        const bool lstmCorrectLayerOrder = false;
        modelsToLoad = {
            { "encoder_decoder_sp_speakers", speakersTransform, dict, lstmCorrectLayerOrder },
            { "encoder_decoder_sp_allspeakers", allSpeakersTransform, dict, lstmCorrectLayerOrder },
            { "encoder_decoder_sp_speakers_myname", speakersTransform, dict, lstmCorrectLayerOrder },
            { "encoder_decoder_sp_allspeakers_myname", allSpeakersTransform, dict, lstmCorrectLayerOrder }
        };
    } else if (mode == "general_conversation") {
        IContextTransformPtr separatePunctuation = new TSeparatePunctuation();
        IContextTransformPtr limitNumTokens = new TLimitNumTokens(300);
        IContextTransformPtr transform = new TCompoundTransform({ separatePunctuation, limitNumTokens });

        TTokenDictPtr dict = new TTokenDict(modelDir / "twitter.159996.dict");

        modelsToLoad = {
            { "ed_tw_c3", transform, dict, true },
            { "ed_tw_cx", transform, dict, true },
            { "ed_tw_cx_from_subs", transform, dict, false }
        };
    } else {
        Y_FAIL("Unsupported mode %s", mode.data());
    }

    for (const auto &tuple : modelsToLoad) {
        const auto &modelName = std::get<0>(tuple);
        const auto &transform = std::get<1>(tuple);
        const auto &dict = std::get<2>(tuple);
        bool lstmCorrectLayerOrder = std::get<3>(tuple);
        TFsPath dir = modelDir / modelName;
        if (!dir.Exists()) {
            continue;
        }
        TEncoderDecoderPtr encoderDecoder = new TEncoderDecoder(dir, lstmCorrectLayerOrder);
        NlgModels[modelName] = new TRnnNlgModel(encoderDecoder, dict, transform);
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
    INlgModelPtr model = NlgModels[modelName];

    ui64 maxLen = FromString<ui64>(params.Get("max_len"));
    float temperature = FromString<float>(params.Get("temperature"));

    try {
        TUtf16String reply = model->GetReply(context, maxLen, temperature, Ctxs[workerId]);
        TString response = WideToUTF8(reply);
        HttpReplyPlainText(socket, response.data());
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

