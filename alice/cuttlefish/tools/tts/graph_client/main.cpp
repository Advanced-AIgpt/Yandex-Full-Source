#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/api/client/stream_timeout.h>

#include <apphost/lib/grpc/client/grpc_client.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/utf8.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/system/condvar.h>
#include <util/system/fs.h>

namespace {

static const TString DEFAULT_REQUEST_ID = "default_request_id";
static const TString DEFAULT_SESSION_ID = "default_session_id";

} // namespace


int main(int argc, char** argv) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    // AppHost location
    TString host;
    opts.AddLongOption("host").StoreResult(&host).DefaultValue("localhost");
    ui32 port;
    opts.AddLongOption("port").StoreResult(&port).DefaultValue(20001);
    TString path;
    opts.AddLongOption("path").StoreResult(&path).DefaultValue("/tts");

    // Tts request
    TString text;
    opts.AddLongOption("text").StoreResult(&text).DefaultValue("Привет, как дела?<speaker audio=\"check1.opus\"><speaker>Очень длинный текст чтобы ответ состоял из нескольких чанков");

    bool replaceShitovaWithShitovaGpu;
    opts.AddLongOption("replace-shitova-with-shitova-gpu").StoreResult(&replaceShitovaWithShitovaGpu).DefaultValue(true);
    bool needTtsBackendTimings;
    opts.AddLongOption("need-tts-backend-timings").StoreResult(&needTtsBackendTimings).DefaultValue(true);
    bool enableTtsBackendTimings;
    opts.AddLongOption("enable-tts-backend-timings").StoreResult(&enableTtsBackendTimings).DefaultValue(true);
    bool enableGetFromCache;
    opts.AddLongOption("enable-get-from-cache").StoreResult(&enableGetFromCache).DefaultValue(true);
    bool enableCacheWarmUp;
    opts.AddLongOption("enable-cache-warm-up").StoreResult(&enableCacheWarmUp).DefaultValue(true);
    bool enableSaveToCache;
    opts.AddLongOption("enable-save-to-cache").StoreResult(&enableSaveToCache).DefaultValue(true);
    bool doNotLogTexts;
    opts.AddLongOption("do-not-log-texts").StoreResult(&doNotLogTexts).DefaultValue(false);

    // Request context
    TString requestId;
    opts.AddLongOption("request-id").StoreResult(&requestId).DefaultValue(DEFAULT_REQUEST_ID);
    TString sessionId;
    opts.AddLongOption("session-id").StoreResult(&sessionId).DefaultValue(DEFAULT_SESSION_ID);
    TString audioFormat;
    opts.AddLongOption("audio-format").StoreResult(&audioFormat).DefaultValue("opus");
    TMaybe<double> voiceVolume = Nothing();
    opts.AddLongOption("voice-volume").StoreResult<double>(&voiceVolume);
    TMaybe<double> voiceSpeed = Nothing();
    opts.AddLongOption("voice-speed").StoreResult<double>(&voiceSpeed);
    TMaybe<TString> voiceLang = Nothing();
    opts.AddLongOption("voice-lang").StoreResult<TString>(&voiceLang);
    TMaybe<TString> voiceVoice = Nothing();
    opts.AddLongOption("voice-voice").StoreResult<TString>(&voiceVoice);
    TMaybe<TString> voiceEmotion = Nothing();
    opts.AddLongOption("voice-emotion").StoreResult<TString>(&voiceEmotion);
    TMaybe<TString> voiceQuality = Nothing();
    opts.AddLongOption("voice-quality").StoreResult<TString>(&voiceQuality);
    bool allowBackgroundAudio;
    opts.AddLongOption("allow-background-audio").StoreResult(&allowBackgroundAudio).DefaultValue(true);
    bool allowWhisper;
    opts.AddLongOption("allow-whisper").StoreResult(&allowWhisper).DefaultValue(true);

    // AppHost params
    TString appHostRequestId;
    opts.AddLongOption("apphost-request-id").StoreResult(&appHostRequestId).DefaultValue("");
    TVector<std::pair<TString, TString>> srcrwrs;
    opts
        .AddLongOption("add-srcrwr", "(repeatable), add for apphost (source, target)")
        .Handler1T<TString>([&srcrwrs](const TString& srcrwr) {
            TVector<TString> parts = SplitString(srcrwr, " ");
            Y_ENSURE(parts.size() == 2u, "expected (source, target) but found '" << srcrwr << "'");
            srcrwrs.push_back(std::make_pair(parts[0], parts[1]));
        })
    ;

    // Save options
    TString resultPath;
    opts.AddLongOption("result-path").StoreResult(&resultPath).DefaultValue("result");
    TString fileNameSuffix;
    opts.AddLongOption("file-name-suffix").StoreResult(&fileNameSuffix).DefaultValue(".opus");
    TString chunkNamePrefix;
    opts.AddLongOption("chunk-name-prefix").StoreResult(&chunkNamePrefix).DefaultValue("chunk");
    TString finalResultFileName;
    opts.AddLongOption("final-result-file-name").StoreResult(&finalResultFileName).DefaultValue("result");

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    resultPath = TStringBuilder() << NFs::CurrentWorkingDirectory() << "/" << resultPath;
    if (NFs::Exists(resultPath)) {
        NFs::RemoveRecursive(resultPath);
    }
    NFs::MakeDirectoryRecursive(resultPath);

    if (requestId == DEFAULT_REQUEST_ID) {
        requestId = TStringBuilder() << requestId << "_" << TInstant::Now().MicroSeconds();
    }
    if (sessionId == DEFAULT_SESSION_ID) {
        sessionId = TStringBuilder() << sessionId << "_" << TInstant::Now().MicroSeconds();
    }

    NAppHost::NClient::TClient client;
    NAppHost::NClient::TFixedGrpcBackend backend(host, port);
    client.AddOrUpdateBackend("APPHOST", backend);

    TInstant startTime = TInstant::Now();
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Path = path;
    streamOptions.MaxAttempts = 1;
    streamOptions.Timeout = TDuration::Minutes(1);
    streamOptions.ChunkTimeout = TDuration::Minutes(1);
    auto stream = client.CreateStream("APPHOST", std::move(streamOptions));

    {
        NAppHost::NClient::TInputDataChunk dataChunk;

        {
            NJson::TJsonValue appHostParams = NJson::TJsonMap();

            if (!appHostRequestId.empty()) {
                appHostParams["reqid"] = appHostRequestId;
            }

            if (!srcrwrs.empty()) {
                auto& srcrwrNode = appHostParams["srcrwr"];
                for (const auto& [source, target] : srcrwrs) {
                    srcrwrNode[source] = target;
                }
            }

            if (!appHostParams.GetMap().empty()) {
                dataChunk.AddItem(NAppHost::APP_HOST_PARAMS_SOURCE_NAME, NAppHost::APP_HOST_PARAMS_TYPE, appHostParams);
            }
        }

        {
            NAliceProtocol::TRequestContext requestContext;

            {
                NAliceProtocol::TRequestContext::THeader& header = *requestContext.MutableHeader();
                header.SetMessageId(requestId);
                header.SetSessionId(sessionId);
            }

            {
                NAliceProtocol::TAudioOptions& audioOptions = *requestContext.MutableAudioOptions();
                audioOptions.SetFormat(audioFormat);
            }

            {
                NAliceProtocol::TVoiceOptions& voiceOptions = *requestContext.MutableVoiceOptions();

                if (voiceVolume.Defined()) {
                    voiceOptions.SetVolume(*voiceVolume);
                }
                if (voiceSpeed.Defined()) {
                    voiceOptions.SetSpeed(*voiceSpeed);
                }
                if (voiceLang.Defined()) {
                    voiceOptions.SetLang(*voiceLang);
                }
                if (voiceVoice.Defined()) {
                    voiceOptions.SetVoice(*voiceVoice);
                }
                if (voiceEmotion.Defined()) {
                    voiceOptions.SetUnrestrictedEmotion(*voiceEmotion);
                }
                if (voiceQuality.Defined()) {
                    NAliceProtocol::TVoiceOptions::EVoiceQuality quality;
                    if (NAliceProtocol::TVoiceOptions::EVoiceQuality_Parse(ToUpperUTF8(*voiceQuality), &quality)) {
                        voiceOptions.SetQuality(quality);
                    }
                }
            }

            {
                NVoicetech::NSettings::TManagedSettings& settingsFromManager = *requestContext.MutableSettingsFromManager();

                settingsFromManager.SetTtsBackgroundAudio(allowBackgroundAudio);
                settingsFromManager.SetTtsAllowWhisper(allowWhisper);
            }

            dataChunk.AddItem("INIT", TString(NAlice::NCuttlefish::ITEM_TYPE_REQUEST_CONTEXT), requestContext);
        }

        {
            NTts::TRequest req;
            req.SetText(text);
            req.SetPartialNumber(0);
            req.SetRequestId(requestId);
            req.SetReplaceShitovaWithShitovaGpu(replaceShitovaWithShitovaGpu);
            req.SetNeedTtsBackendTimings(needTtsBackendTimings);
            req.SetEnableTtsBackendTimings(enableTtsBackendTimings);
            req.SetEnableGetFromCache(enableGetFromCache);
            req.SetEnableCacheWarmUp(enableCacheWarmUp);
            req.SetEnableSaveToCache(enableSaveToCache);
            req.SetDoNotLogTexts(doNotLogTexts);

            dataChunk.AddItem("INIT", TString(NAlice::NCuttlefish::ITEM_TYPE_TTS_REQUEST), req);
        }

        stream.Write(std::move(dataChunk), true);
    }

    {

        TString allData = "";
        ui32 audioChunkId = 0;
        bool isFirstAppHostChunk = true;
        TInstant lastChunkTime = TInstant::Zero();
        TCondVar quitCondVar;
        TMutex mutex;
        bool needQuit = false;
        bool wasError = false;
        bool hasEndOfStream = false;

        auto finish = [&]() {
            TGuard<TMutex> g(mutex);
            needQuit = true;
            quitCondVar.Signal();
            Cout << "Send need quit signal" << Endl;
        };

        std::function<void(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>>)> onAppHostChunkCallback = [&](NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
            TGuard<TMutex> g(mutex);
            try {
                TInstant currentTime = TInstant::Now();

                auto response = future.GetValueSync();
                if (!response.Defined()) {
                    if (isFirstAppHostChunk) {
                        Cout << "Got EOS in " << ToString(currentTime - startTime)  << Endl;
                        isFirstAppHostChunk = false;
                    } else {
                        Cout << "Got EOS in " << ToString(currentTime - lastChunkTime)  << Endl;
                    }

                    TUnbufferedFileOutput outputFile(TStringBuilder() << resultPath << "/" << finalResultFileName << fileNameSuffix);
                    outputFile.Write(allData);

                    finish();
                    return;
                }

                if (isFirstAppHostChunk) {
                    Cout << "Got first apphost chunk in " << ToString(currentTime - startTime) << Endl;
                    isFirstAppHostChunk = false;
                } else {
                    Cout << "Got new apphost chunk in " << ToString(currentTime - lastChunkTime) << Endl;
                }

                for (const auto& item : response->GetAllItems()) {
                    if (item.GetType() == NAlice::NCuttlefish::ITEM_TYPE_AUDIO) {
                        NAliceProtocol::TAudio audio = item.ParseProtobuf<NAliceProtocol::TAudio>();
                        hasEndOfStream |= (audio.GetMessageCase() == NAliceProtocol::TAudio::kEndStream);

                        if (audio.HasChunk()) {
                            {
                                NAliceProtocol::TAudio audioExceptChunk = audio;
                                audioExceptChunk.ClearChunk();
                                Cout << "Audio chunk: size=" << audio.GetChunk().GetData().size() << ", meta_info=" << audioExceptChunk.ShortUtf8DebugString()  << Endl;
                            }

                            TUnbufferedFileOutput outputFile(TStringBuilder() << resultPath << "/" << chunkNamePrefix << "." << audioChunkId++ << fileNameSuffix);
                            outputFile.Write(audio.GetChunk().GetData());

                            allData += audio.GetChunk().GetData();
                        } else {
                            Cout << "Audio item: " << audio.ShortUtf8DebugString() << Endl;
                        }
                    } else if (item.GetType() == NAlice::NCuttlefish::ITEM_TYPE_TTS_TIMINGS) {
                        NTts::TTimings ttsTimings = item.ParseProtobuf<NTts::TTimings>();
                        Cout << "Tts timings item: " << ttsTimings.ShortUtf8DebugString() << Endl;
                    } else {
                        Cout << "Unknown item: item_type=" << item.GetType() << Endl;
                    }
                }

                lastChunkTime = currentTime;
                stream.Read().Subscribe(onAppHostChunkCallback);
            } catch (const NAppHost::NClient::TStreamTimeout&) {
                Cout << "Got stream timeout: " << CurrentExceptionMessage() << Endl;
                wasError = true;
                stream.Cancel();
                finish();
            } catch (...) {
                Cout << "Got stream exception: " << CurrentExceptionMessage() << Endl;
                wasError = true;
                finish();
            }
        };

        stream.Read().Subscribe(onAppHostChunkCallback);
        {
            TGuard<TMutex> g(mutex);
            if (!needQuit) {
                quitCondVar.WaitT(mutex, TDuration::Max(), [&needQuit](){
                    return needQuit;
                });
                Cout << "Got need quit signal" << Endl;
            } else {
                Cout << "Finished faster than mutex acquired" << Endl;
            }
        }
        Cout << "Total time: " << ToString(TInstant::Now() - startTime) << Endl;

        if (wasError) {
            Cout << Endl << "WARNING: Request failed" << Endl;
            return 1;
        }

        if (!hasEndOfStream) {
            Cout << Endl << "WARNING: Stream finished without error and EndOfStream item in response, probably tts aggregator crashed or timedout" << Endl;
            return 1;
        }
    }

    return 0;
}
