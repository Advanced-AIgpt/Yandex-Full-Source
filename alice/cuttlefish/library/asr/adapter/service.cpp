#include "service.h"
#include "asr_callbacks_with_eventlog.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/protos/data/contacts.pb.h>

#include <google/protobuf/wrappers.pb.h>

#include <contrib/libs/pugixml/pugixml.hpp>

#include <library/cpp/neh/neh.h>
#include <library/cpp/json/json_reader.h>

#include <apphost/lib/proto_answers/http.pb.h>


namespace NAlice::NAsrAdapter {
    const TString TService::DefaultConfigResource = "/asr-adapter/default_config.json";
    TAtomicCounter TService::TRequestProcessor::NextProcNumber_ = 0;
}

using namespace NAlice;
using namespace NAlice::NAsrAdapter;

TIntrusivePtr<NAsr::TInterface::TCallbacks> TService::TRequestProcessor::CreateAsrCallbacks(
    TRequestHandlerPtr rh,
    const TString& requestId
) {
    return new TAsrCallbacksWithEventlog(rh, requestId, Number_, LogContext_.RtLogPtr(), LogContext_.Options());
}

bool TService::TRequestProcessor::OnAppHostProtoItem(
    const TString& type,
    const NAppHost::NService::TProtobufItem& item
) {
    Unistat().OnReceiveFromAppHostRaw(item.Raw().Size());
    return NAsr::TService::TRequestProcessor::OnAppHostProtoItem(type, item);
}

void TService::TRequestProcessor::OnBeginProcessInput() {
    Unistat().OnReceiveFromAppHostReqRaw(RequestHandler_->Context().GetRawRequestData().Size());
    auto earlyItems = RequestHandler_->Context().GetProtobufItemRefs("context_load_response", NAppHost::EContextItemSelection::Input);
    for (auto it = earlyItems.begin(); it != earlyItems.end(); ++it) {
        try {
            NAliceProtocol::TContextLoadResponse resp;
            NCuttlefish::ParseProtobufItem(*it, resp);
            OnContextLoadResponse(resp);
        } catch (...) {
            // This error is not critical
            // TODO some metrics
            OnWarning(CurrentExceptionMessage());
        }
    }
}

void TService::TRequestProcessor::OnContextLoadResponse(const NAliceProtocol::TContextLoadResponse& resp) {
    if (RequestHandler_->Context().HasItem("flag_use_contacts_in_asr")) {
        try {
            if (resp.HasContactsProto()) {
                const auto& list = resp.GetContactsProto().GetContacts();
                for (auto it = list.begin(); it != list.end(); ++it) {
                    UserInfo_.MutableContactBookItems()->Add()->SetDisplayName(it->GetDisplayName());
                }
            } else if (resp.HasContactsResponse()) {
                NJson::TJsonValue contacts = NJson::ReadJsonFastTree(resp.GetContactsResponse().GetContent());
                const auto& list = contacts["data"]["contacts"].GetArray();
                for (auto it = list.begin(); it != list.end(); ++it) {
                    UserInfo_.MutableContactBookItems()->Add()->SetDisplayName((*it)["display_name"].GetString());
                }
            }
        } catch (...) {
            OnWarning(CurrentExceptionMessage());
        }
    }

    if (resp.HasFlagsInfo()) {
        NVoice::NExperiments::TFlagsJsonFlagsConstRef flagsInfoRef(&resp.GetFlagsInfo());
        AsrAbFlagsSerializedJson_ = flagsInfoRef.GetAsrAbFlagsSerializedJson();
    }

    if (resp.HasPatchAsrOptionsForNextRequestDirective()) {
        PatchAsrOptionsForNextRequestDirective_ = resp.GetPatchAsrOptionsForNextRequestDirective();
    }
}

bool TService::TRequestProcessor::OnInitRequest(
    NAsr::NProtobuf::TRequest& request,
    TIntrusivePtr<NAsr::TInterface::TCallbacks> callbacks,
    const TString& requestId
) {
    try {
        OnInitRequestImpl(request, callbacks, requestId);
    } catch (...) {
        LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(CurrentExceptionMessage());
        // here we has not runned asio thread so can send response directly to apphost context
        NAsr::NProtobuf::TResponse response;
        auto& initResponse = *response.MutableInitResponse();
        NAsr::NProtobuf::FillRequiredDefaults(initResponse);
        initResponse.SetIsOk(false);
        initResponse.SetErrMsg(CurrentExceptionMessage());
        callbacks->OnInitResponse(response);
        callbacks->OnClosed();
        return false;
    }
    return true;
}

void TService::TRequestProcessor::OnInitRequestImpl(
    NAsr::NProtobuf::TRequest& request,
    TIntrusivePtr<NAsr::TInterface::TCallbacks> callbacks,
    const TString& requestId
) {
    if (requestId) {
        LogContext_.LogEventInfo(NEvClass::RequestId(requestId));
    }
    TAsrCallbacksWithEventlog* callbacksWithLog = dynamic_cast<TAsrCallbacksWithEventlog*>(callbacks.Get());

    request.MutableInitRequest()->MutableUserInfo()->MergeFrom(UserInfo_);

    if (AsrAbFlagsSerializedJson_.Defined()) {
        request.MutableInitRequest()->SetExperimentsAB(AsrAbFlagsSerializedJson_.GetRef());
    }

    if (PatchAsrOptionsForNextRequestDirective_.Defined()) {
        if (PatchAsrOptionsForNextRequestDirective_->HasAdvancedASROptionsPatch()) {
            const auto& advancedASROptionsPatch = PatchAsrOptionsForNextRequestDirective_->GetAdvancedASROptionsPatch();
            auto& advancedASROptions = *request.MutableInitRequest()->MutableAdvancedOptions();

            if (advancedASROptionsPatch.HasMaxSilenceDurationMS()) {
                advancedASROptions.SetMaxSilenceDurationMS(advancedASROptionsPatch.GetMaxSilenceDurationMS().value());
            }
            if (advancedASROptionsPatch.HasEnableSidespeechDetector()) {
                advancedASROptions.SetEnableSidespeechDetector(advancedASROptionsPatch.GetEnableSidespeechDetector().value());
            }
            if (advancedASROptionsPatch.HasEouThreshold()) {
                advancedASROptions.SetEouThreshold(advancedASROptionsPatch.GetEouThreshold().value());
            }
            if (advancedASROptionsPatch.HasInitialMaxSilenceDurationMS()) {
                advancedASROptions.SetInitialMaxSilenceDurationMS(advancedASROptionsPatch.GetInitialMaxSilenceDurationMS().value());
            }
        }
    }

    auto& initRequest = request.GetInitRequest();
    if (Service_.Config().asr().protocol_version() == NAliceAsrAdapterConfig::Asr::YALDI) {
        // support v1 (yaldi) protocol
        auto httpClient = Service_.GetHttpClient();
        TIntrusivePtr<TAsr2ViaAsr1Client> asr(new TAsr2ViaAsr1Client(
            httpClient,
            callbacks,
            callbacksWithLog->LogContext(),
            LogContext_,
            Service_.Config().asr().asr1().wait_spotter_after_eou()
        ));
        asr->StartRequest(
            Service_.AsrUrl(),
            initRequest,
            Service_.AsrInfo(),
            Service_.Config().asr().ignore_parsing_protobuf_error()
        );
        Asr_.Reset(asr.Get());
    } else if (Service_.Config().asr().protocol_version() == NAliceAsrAdapterConfig::Asr::ASR) {
        // support v2 asr protocol
        TAsr2* asr = new TAsr2(
            request,
            callbacks,
            callbacksWithLog->LogContext(),
            LogContext_
        );
        Asr_.Reset(asr);
        LogContext_.LogEventInfoCombo<NEvClass::Asr2Request>(Service_.AsrUrl());
        Service_.StartAsrRequest(Service_.GetHttpClient(), asr->Handler(), {});  // TODO: rtlog token
    } else if (Service_.Config().asr().protocol_version() == NAliceAsrAdapterConfig::Asr::INTERNAL_FAKE) {
        // default/base asr service implement fake asr (emulator for testing), so use it
        NAsr::TService::TRequestProcessor::OnInitRequest(request, callbacks, requestId);
    } else {
        throw yexception() << "for asr_adapter service configured unsupported asr.protocol_version";
    }
}

TService::TService(const TConfig& config)
    : Config_(config)
    , ExecutorsPool_(Config_.asr().client_threads())
{
    // here we can add options for http client (to asr-server&yaldi-server)
    NVoicetech::THttpClientConfig httpClientConfig;
    httpClientConfig.SetConnectTimeout(TFixedDuration(Config_.asr().connect_timeout()));

    HttpClients_.resize(ExecutorsPool_.Size());
    for (size_t i = 0; i < ExecutorsPool_.Size(); ++i) {
        HttpClients_[i].Reset(new NVoicetech::THttpClient(
            httpClientConfig, ExecutorsPool_.GetExecutor().GetIOService(), &ClientsCount_
        ));
    }
    TStringOutput so(AsrUrl_);
    so << TStringBuf("http://") << config.asr().host() << ':' << config.asr().port() << config.asr().path();

    // crutch for getting ASR (meta) info for yaldi servers: https://st.yandex-team.ru/VOICESERV-3385#60128129f83a44739dcc2ccc
    if (Config().asr().protocol_version() == NAliceAsrAdapterConfig::Asr::YALDI) {
        AsrInfo_.UseFakeTopic = Config().asr().asr1().use_fake_topic_in_response();
        // make curl to /info & fill AsrInfo_.Topic & etc
        TString error;
        TStringStream ss;
        ss << "http://" << config.asr().host() << ':' << config.asr().port() << "/info";
        for (size_t i = 0; i < 10; ++i) {
            NNeh::TResponseRef resp = NNeh::Request(ss.Str())->Wait(TDuration::Seconds(1));
            if (!resp) {
                if (!error) {
                    error = "timeout";
                }
            } else if (resp->IsError()) {
                Sleep(TDuration::Seconds(5));
                error = resp->GetErrorText();
                continue;
            } else {
                // handle response resp->Data; (Stroka)
                pugi::xml_document doc;
                pugi::xml_parse_result result = doc.load_string(resp->Data.c_str());
                if (!result) {
                    error = "could not parse response as xml";
                    continue;
                }

                pugi::xml_node res = doc.child("Info").child("Result");
                AsrInfo_.ServerVersion = res.child("Version").child_value();
                pugi::xml_node model = res.child("Lingware").child("Model");
                if (model.empty()) {
                    // can not get asr info about lingware/model, so generate fake topic/model info
                    AsrInfo_.UseFakeTopic = true;
                    AsrInfo_.TopicVersion = "0.fake";
                } else {
                    AsrInfo_.Topic = model.child("Name").child_value();
                    AsrInfo_.TopicVersion = model.child("Version").child_value();
                }
                error.clear();
                break;
            }
        }
        if (error) {
            ythrow yexception() << "asr /info request error: " << error;
        }
    }
}

void TService::StartAsrRequest(NVoicetech::THttpClient& client, const NVoicetech::TUpgradedHttpHandlerRef& handler, const TString& rtLogToken) {
    TString headers;
    if (rtLogToken) {
        TStringOutput so(headers);
        so << "\r\nX-RTLog-Token: " << rtLogToken;
    }
    client.RequestUpgrade(AsrUrl_, NVoicetech::TProtobufHandler::HttpUpgradeType, handler, headers);
}
