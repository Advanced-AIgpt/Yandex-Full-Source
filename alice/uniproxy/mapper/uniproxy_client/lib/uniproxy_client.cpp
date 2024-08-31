#include "uniproxy_client.h"
#include <alice/uniproxy/mapper/library/logging/log.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/string_utils/url/url.h>

#include <contrib/libs/poco/Foundation/include/Poco/Buffer.h>

#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/stream/str.h>
#include <util/system/byteorder.h>

#include <cstring>

using namespace NAlice::NUniproxy::NHelpers;
using namespace NDatetime;
using namespace NJson;
using namespace std;

namespace NAlice::NUniproxy {
    TUniproxyClient::TUniproxyClient(const TUniproxyClientParams& params)
        : Logger(params.Logger)
        , FlagsContainer(params.FlagsContainer)
        , Client(
              AddCgi(params.UniproxyUrl, {std::make_pair("srcrwr", "NOTIFICATOR:http://notificator-test.alice.yandex.net")}),
              params)
        , UniproxyClientParams(params)
    {
        if (UniproxyClientParams.VinsUrl && FlagsContainer && FlagsContainer->Has("apphost_waitall_timeout5s")) {
            UniproxyClientParams.VinsUrl = AddCgi(UniproxyClientParams.VinsUrl, {std::make_pair("waitall", "da"),
                                                                                 std::make_pair("timeout", "5000000")});
        }
        Init();
    }

    TUniproxyClient::~TUniproxyClient() {
    }

    TResponses TUniproxyClient::SendTextRequest(TStringBuf text, TExtraRequestParams extraParams, bool reloadTimestamp) {
        if (FlagsContainer && FlagsContainer->Has("handle_empty_text_ADD-39")) {
            const auto& payload_text = extraParams.PayloadTemplate["request"]["event"]["text"];
            if (!payload_text.IsDefined() || !payload_text.IsString() || payload_text.GetStringRobust().empty()) {
                return {};
            }
        } else {
            extraParams.PayloadTemplate["request"]["event"]["text"] = text;
        }
        return SendCustomEventType(TStringBuf("text_input"), extraParams, reloadTimestamp);
    }

    TResponses TUniproxyClient::SendVoiceRequest(TStringBuf topic, IInputStream& inputStream,
                                                 const TExtraVoiceRequestParams& extraParams, bool reloadTimestamp,
                                                 const TMaybe<size_t> voiceLength) {
        auto const streamId = StreamId;
        Y_ENSURE(StreamId + 2 > StreamId, "StreamId overflow");
        Y_ENSURE(voiceLength.Defined() && *voiceLength > 0, "Invalid voiceLength for VoiceRequest");
        StreamId += 2;
        auto const messageId = SendVoiceInputMessage(topic, streamId, extraParams, reloadTimestamp);
        SendStream(inputStream, streamId, messageId);

        // Normally we expect 2 ASR responses, 1 Bio response, 1 VINS response and 2 TTS responses (text and stream)
        TResponses responses(Reserve(5));
        if (!ReceiveASRResponses(messageId, streamId, responses)) {
            LOG_INFO("Nothing has been recognized. No more responses are expected\n");
            return responses;
        }
        ReceiveBioVinsTTSResponses(messageId, responses);
        return responses;
    }

    TResponses TUniproxyClient::SendServerAction(const TExtraRequestParams& extraParams, bool reloadTimestamp) {
        return SendCustomEventType(TStringBuf("server_action"), extraParams, reloadTimestamp);
    }

    TResponses TUniproxyClient::SendImageInput(const TExtraRequestParams& extraParams, bool reloadTimestamp) {
        return SendCustomEventType(TStringBuf("image_input"), extraParams, reloadTimestamp);
    }

    TResponses TUniproxyClient::GenerateVoice(TStringBuf text, const TExtraTtsGenerateParams& extraParams) {
        TJsonValue request = CreateMessageTemplate(TStringBuf("TTS"), TStringBuf("Generate"));
        auto& payload = request["event"]["payload"];
        {
            payload["text"] = text;
            payload["lang"] = UniproxyClientParams.Language;
            payload["format"] = extraParams.AudioFormat.has_value()
                                    ? extraParams.AudioFormat.value()
                                    : TStringBuf("audio/opus");
            payload["voice"] = extraParams.Voice.has_value()
                                   ? extraParams.Voice.value()
                                   : TStringBuf("shitova.gpu");
            payload["quality"] = extraParams.Quality.has_value()
                                     ? extraParams.Quality.value()
                                     : TStringBuf("UltraHigh");
            if (extraParams.Speed.has_value()) {
                payload["speed"] = extraParams.Speed.value();
            }
            if (extraParams.Emotion.has_value()) {
                payload["emotion"] = extraParams.Emotion.value();
            }
        }

        // Normally we expect 2 TTS responses (text and stream)
        const auto messageId = SendMessage(request);
        TResponses responses(Reserve(2));
        ReceiveTtsResponses(messageId, responses);
        return responses;
    }

    void TUniproxyClient::Init() {
        TJsonValue request = CreateMessageTemplate(TStringBuf("System"), TStringBuf("SynchronizeState"));
        auto& payload = request["event"]["payload"];
        {
            payload["auth_token"] = UniproxyClientParams.AuthToken;
            payload["uuid"] = UniproxyClientParams.Uuid;
            payload["disable_fallback"] = true;
            payload["speechkitVersion"] = "4.10.0";
            payload["save_to_mds"] = false;
            if (IsQuasar()) {
                // VOICESERV-3140
                payload["biometry_group"] = UniproxyClientParams.Uuid;
            }
            if (!UniproxyClientParams.OAuthToken.empty()) {
                payload["oauth_token"] = UniproxyClientParams.OAuthToken;
            }

            auto& app = payload["vins"]["application"];
            if (UniproxyClientParams.Application.IsDefined()) {
                app = UniproxyClientParams.Application;
            } else {
                app["app_id"] = UniproxyClientParams.ApplicationId;
                app["app_version"] = UniproxyClientParams.ApplicationVersion;
                app["os_version"] = UniproxyClientParams.OsVersion;
                app["platform"] = UniproxyClientParams.Platform;
            }
            app["lang"] = UniproxyClientParams.Language;
            app["uuid"] = UniproxyClientParams.Uuid;

            if (UniproxyClientParams.SyncStateExperiments.IsDefined()) {
                auto& request = payload["request"];
                request["experiments"] = UniproxyClientParams.SyncStateExperiments;
            }
            if (!UniproxyClientParams.VinsUrl.empty()) {
                payload["vinsUrl"] = UniproxyClientParams.VinsUrl;
            }
            if (UniproxyClientParams.DisableLocalExperiments) {
                payload["disable_local_experiments"] = true;
            }
            if (!UniproxyClientParams.ShootingSource.empty()) {
                payload["shooting_source"] = UniproxyClientParams.ShootingSource;
            }
            if (UniproxyClientParams.SupportedFeatures.IsDefined()) {
                payload["supported_features"] = UniproxyClientParams.SupportedFeatures;
            }
        }

        const auto messageId = SendMessage(request);
        const auto response = ReceiveTextResponse();
        const auto directive = GetDirectiveOrNull(response.JsonValue);
        Y_ENSURE(directive);
        ValidateDirectiveNamespaceNameAndRefMessageId(*directive, TStringBuf("System"), TStringBuf("SynchronizeStateResponse"), messageId);
    }

    TUniproxyClient::TResponseInternal TUniproxyClient::ReceiveResponse() {
        LOG_INFO("Receiving response...\n");
        Poco::Buffer<char> buffer(0);
        auto const result = Client.Receive(buffer);
        if (result.Size == 0) {
            ythrow TUniproxyInteractionError() << "Received nothing. Expected some data";
        }
        if (result.Type == TBaseClient::EResultType::Text) {
            TString const text{buffer.begin(), result.Size};
            LOG_INFO("Received text response %s\n", text.c_str());
            return {text, {}};
        }

        Y_ENSURE(result.Type == TBaseClient::EResultType::Binary, "Unexpected result type");
        constexpr size_t streamIdSize = sizeof(TStreamId);
        if (result.Size <= streamIdSize) {
            ythrow TUniproxyInteractionError() << "Invalid stream chunk size. At least " << streamIdSize + 1 << " bytes required";
        }
        TStreamId streamIdBigEndian;
        memcpy(&streamIdBigEndian, buffer.begin(), streamIdSize);
        TStreamId const streamId = InetToHost(streamIdBigEndian);
        size_t const dataSize = result.Size - streamIdSize;
        LOG_INFO("Received %lu bytes of the stream %d\n", dataSize, streamId);
        TString const data{buffer.begin() + streamIdSize, dataSize};
        return {data, streamId};
    }

    TUniproxyClient::TTextResponseResult TUniproxyClient::ReceiveTextResponse() {
        auto const result = ReceiveResponse();
        if (result.StreamId.has_value()) {
            ythrow TUniproxyInteractionError() << "Invalid response type. Expected text";
        }
        auto const responseJson = ParseTextResponse(result.Data);
        return {result.Data, responseJson};
    }

    TUniproxyClient::TTextResponseResult TUniproxyClient::ReceiveTextResponse(TSkipFilterFunc skipFilter) {
        for (;;) {
            auto const response = ReceiveTextResponse();
            if (!skipFilter(response.JsonValue)) {
                return response;
            }
            LOG_INFO("Text response skipped due to a filter\n");
        }
    }

    TJsonValue TUniproxyClient::CreateVinsMessage(TStringBuf name, TStringBuf eventType, const TExtraRequestParams& extraParams, bool reloadTimestamp) {
        auto message = CreateMessageTemplate(TStringBuf("Vins"), name);
        auto& payload = message["event"]["payload"];
        if (extraParams.PayloadTemplate.IsDefined()) {
            payload = extraParams.PayloadTemplate;
        }
        {
            payload["header"]["request_id"] = extraParams.RequestId.has_value()
                                                  ? extraParams.RequestId.value()
                                                  : CreateGuidAsString();
            auto& app = payload["application"];
            if (reloadTimestamp) {
                if (UniproxyClientParams.Application.IsDefined()) {
                    app["client_time"] = UniproxyClientParams.Application["client_time"];
                    app["timestamp"] = UniproxyClientParams.Application["timestamp"];
                } else {
                    auto const now = TInstant::Now();
                    auto const tzTime = ToCivilTime(now, GetTimeZone(UniproxyClientParams.Timezone));
                    app["client_time"] = tzTime.ToString("%Y%m%dT%H%M%S");
                    app["timestamp"] = ToString(now.Seconds());
                }
            }
            app["lang"] = UniproxyClientParams.Language;
            app["timezone"] = UniproxyClientParams.Timezone;
            app["uuid"] = UniproxyClientParams.Uuid;

            if (!UniproxyClientParams.VinsUrl.empty()) {
                payload["vinsUrl"] = UniproxyClientParams.VinsUrl;
            }

            auto& eventData = payload["request"]["event"];
            eventData["type"] = eventType;
        }
        return message;
    }

    TString TUniproxyClient::SendMessage(const TJsonValue& request) {
        const auto requestData = WriteJson(request);
        const auto& ns = request["event"]["header"]["namespace"].GetString();
        const auto& name = request["event"]["header"]["name"].GetString();
        LOG_INFO("Sending %s.%s\n", ns.c_str(), name.c_str());
        if (name == "VoiceInput" && FlagsContainer && FlagsContainer->Has("log_voice_input")) {
            LOG_INFO("%s\n", requestData.c_str());
        } else {
            LOG_DEBUG("%s\n", requestData.c_str());
        }
        Client.SendText(requestData);
        return GetMessageId(request);
    }

    TString TUniproxyClient::SendVoiceInputMessage(TStringBuf topic, TStreamId streamId, const TExtraVoiceRequestParams& extraParams, bool reloadTimestamp) {
        TJsonValue request = CreateVinsMessage(TStringBuf("VoiceInput"), TStringBuf("voice_input"), extraParams, reloadTimestamp);
        auto& event = request["event"];
        {
            auto& payload = event["payload"];
            if (IsQuasar()) {
                // VOICESERV-3140
                payload["biometry_group"] = UniproxyClientParams.Uuid;
            }
            payload["lang"] = UniproxyClientParams.Language;
            payload["topic"] = topic;
            payload["format"] = extraParams.AudioFormat.has_value()
                                    ? extraParams.AudioFormat.value()
                                    : TStringBuf("audio/opus");
        }
        event["header"]["streamId"] = streamId;
        return SendMessage(request);
    }

    TResponses TUniproxyClient::SendCustomEventType(TStringBuf eventType, const TExtraRequestParams& extraParams, bool reloadTimestamp) {
        TJsonValue request = CreateVinsMessage(TStringBuf("TextInput"), eventType, extraParams, reloadTimestamp);
        const auto messageId = SendMessage(request);

        // Normally we expect 1 VINS response and 2 TTS responses (text and stream)
        TResponses responses(Reserve(3));
        ReceiveBioVinsTTSResponses(messageId, responses);
        return responses;
    }

    void TUniproxyClient::SendStream(IInputStream& inputStream, TStreamId streamId, const TString& messageId) {
        constexpr size_t streamIdSize = sizeof(TStreamId);
        const size_t bufferSize = UniproxyClientParams.AsrChunkSize >= 0 ? UniproxyClientParams.AsrChunkSize + streamIdSize : streamIdSize;
        vector<char> buffer(bufferSize);
        TStreamId const streamIdBigEndian = HostToInet(streamId);
        memcpy(buffer.data(), &streamIdBigEndian, streamIdSize);
        if (UniproxyClientParams.AsrChunkSize >= 0) {
            for (;;) {
                auto* begin = buffer.data() + streamIdSize;
                auto const size = inputStream.Read(begin, UniproxyClientParams.AsrChunkSize);
                if (size == 0) {
                    break;
                }
                auto const sendSize = size + streamIdSize;
                LOG_INFO("Sending %lu bytes of stream %d\n", sendSize, streamId);
                Client.SendBinary({buffer.data(), sendSize});
                if (UniproxyClientParams.AsrChunkDelayMs > 0) {
                    Sleep(TDuration::MilliSeconds(UniproxyClientParams.AsrChunkDelayMs));
                }
            }
        } else {
            for (;;) {
                char c;
                auto const size = inputStream.Read(&c, 1);
                if (size == 0) {
                    break;
                }
                buffer.push_back(c);
            }
            auto const sendSize = buffer.size();
            LOG_INFO("Sending %lu bytes of stream %d\n", sendSize, streamId);
            Client.SendBinary({buffer.data(), sendSize});
        }

        TJsonValue streamControl{};
        {
            auto& body = streamControl["streamcontrol"];
            body["action"] = 0;
            body["reason"] = 0;
            body["streamId"] = streamId;
            body["messageId"] = messageId;
        }
        auto const messageData = WriteJson(streamControl);
        LOG_INFO("Sending streamcontrol: %s\n", messageData.c_str());
        Client.SendText(messageData);
    }

    void TUniproxyClient::ReceiveBioVinsTTSResponses(TStringBuf messageId, TResponses& responses) {
        auto vinsResponse = ReceiveTextResponse(&IsMessageToSkip);
        if (IsNamespaceNameInResponse(vinsResponse.JsonValue, TStringBuf("Biometry"), TStringBuf("Classification"))) {
            LOG_INFO("Biometry response received\n");
            const auto& bioResponse = vinsResponse;
            ValidateNamespaceNameAndRefMessageId(bioResponse.JsonValue, TStringBuf("Biometry"), TStringBuf("Classification"), messageId);
            responses.emplace_back(EResponseType::Bio, bioResponse.Text);
            vinsResponse = ReceiveTextResponse(&IsMessageToSkip);
        }

        ValidateNamespaceNameAndRefMessageId(vinsResponse.JsonValue, TStringBuf("Vins"), TStringBuf("VinsResponse"), messageId);
        responses.emplace_back(EResponseType::Vins, vinsResponse.Text);
        if (FlagsContainer && FlagsContainer->Has("disable_tts")) {
            LOG_INFO("Flag \"disable_tts\" has set. No TTS responses expected\n");
            return;
        }
        if (FlagsContainer && FlagsContainer->Has("silent_vins")) {
            LOG_INFO("Flag \"silent_vins\" has set. No TTS responses expected\n");
            return;
        }

        if (HasSpeechMarker(vinsResponse.JsonValue)) {
            LOG_INFO("VinsResponse has a speech marker. TTS responses are expected\n");
            ReceiveTtsResponses(messageId, responses);
        } else {
            LOG_INFO("VinsResponse has no speech marker. No TTS responses expected\n");
        }
    }

    bool TUniproxyClient::ReceiveASRResponses(TStringBuf messageId, TStreamId streamId, TResponses& responses) {
        TJsonValue recognition{};
        bool isTrash = false;
        for (;;) {
            auto const response = ReceiveTextResponse(&IsMessageToSkip);
            auto const directive = GetDirectiveOrNull(response.JsonValue);
            if (directive == nullptr) {
                ValidateStreamControl(response.JsonValue, streamId);
                TJsonValue::TArray array{};
                return recognition.GetArray(&array) && !isTrash && !array.empty() && !IsEmptyHypothesis(array);
            }

            const auto isBioResponse = IsNamespaceNameInResponse(response.JsonValue, TStringBuf("Biometry"),
                                                                 TStringBuf("Classification"));
            if (FlagsContainer && FlagsContainer->Has("check_bio_inside_asr_responses") && isBioResponse) {
                LOG_INFO("Biometry response received\n");
                ValidateNamespaceNameAndRefMessageId(response.JsonValue, TStringBuf("Biometry"),
                                                     TStringBuf("Classification"), messageId);
                responses.emplace_back(EResponseType::Bio, response.Text);
                continue;
            }
            ValidateDirectiveNamespaceNameAndRefMessageId(*directive, TStringBuf("ASR"), TStringBuf("Result"), messageId);
            responses.emplace_back(EResponseType::Asr, response.Text);

            auto const& payload = GetDirectivePayload(*directive);
            TJsonValue responseCode{};
            if (!payload.GetValue(TStringBuf("responseCode"), &responseCode)) {
                ythrow TUniproxyInteractionError() << "Invalid ASR.Result payload: missing responseCode";
            }
            if (!responseCode.IsString()) {
                ythrow TUniproxyInteractionError() << "Invalid ASR.Result payload: responseCode should be a string";
            }
            auto const& responseCodeValue = responseCode.GetString();
            if (responseCodeValue != TStringBuf("OK")) {
                ythrow TUniproxyInteractionError() << "ASR.Result responseCode is not OK: " << responseCodeValue;
            }
            payload.GetValue(TStringBuf("recognition"), &recognition);
            isTrash = GetPayloadIsTrash(payload);
        }
    }

    bool TUniproxyClient::HasSpeechMarker(const NJson::TJsonValue& vinsResponse) {
        auto const* textValue = vinsResponse.GetValueByPath(TStringBuf("directive.payload.voice_response.output_speech.text"));
        return textValue != nullptr;
    }

    void TUniproxyClient::ReceiveTtsResponses(TStringBuf messageId, TResponses& responses) {
        const auto response = ReceiveTextResponse(&IsMessageToSkip);
        HandleTtsResponses(response, messageId, responses);
    }

    void TUniproxyClient::HandleTtsResponses(const TUniproxyClient::TTextResponseResult& response, TStringBuf messageId,
                                             TResponses& responses) {
        ValidateNamespaceNameAndRefMessageId(response.JsonValue, TStringBuf("TTS"), TStringBuf("Speak"), messageId);
        responses.emplace_back(EResponseType::TtsText, response.Text);
        auto const* streamIdJson = GetValueOrNull(response.JsonValue["directive"]["header"], TStringBuf("streamId"));
        if (streamIdJson == nullptr) {
            ythrow TUniproxyInteractionError() << "Invalid response: missing streamId";
        }
        if (!streamIdJson->IsUInteger()) {
            ythrow TUniproxyInteractionError() << "Invalid response: streamId is not unsigned integer";
        }

        TStreamId const streamId = streamIdJson->GetUInteger();
        TString streamData;

        {
            TStringOutput output(streamData);
            for (;;) {
                auto const response = ReceiveResponse();
                if (!response.StreamId.has_value()) {
                    auto const responseJson = ParseTextResponse(response.Data);
                    if (IsMessageToSkip(responseJson)) {
                        LOG_INFO("Text response skipped due to a filter\n");
                        continue;
                    }
                    ValidateStreamControl(responseJson, streamId);
                    break;
                }
                if (response.StreamId.value() != streamId) {
                    ythrow TUniproxyInteractionError() << "Unexpected stream " << response.StreamId.value() << ". Expected " << streamId;
                }
                output.Write(response.Data.data(), response.Data.size());
            }
        }
        responses.emplace_back(EResponseType::TtsStream, streamData, streamId);
    }

    bool TUniproxyClient::IsQuasar() {
        return UniproxyClientParams.AuthToken == "51ae06cc-5c8f-48dc-93ae-7214517679e6";
    }

    TString TUniproxyClient::GetRemoteAddress() const {
        return Client.GetRemoteAddress();
    }

    ui16 TUniproxyClient::GetRemotePort() const {
        return Client.GetRemotePort();
    }

}
