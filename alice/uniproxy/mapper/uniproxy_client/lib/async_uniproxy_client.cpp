#include "async_uniproxy_client.h"

#include <library/cpp/json/json_writer.h>

#include <alice/uniproxy/mapper/library/logging/log.h>

#include <util/charset/utf8.h>

using namespace NAlice::NUniproxy::NHelpers;
using namespace NDatetime;
using namespace NJson;
using namespace std;

namespace NAlice::NUniproxy {
    TResponses TAsyncUniproxyClient::SendVoiceRequest(TStringBuf topic,
                                                      IInputStream& inputStream,
                                                      const TExtraVoiceRequestParams& extraParams,
                                                      bool reloadTimestamp,
                                                      const TMaybe<size_t> voiceLength) {
        Y_ENSURE(StreamId + 2 > StreamId, "StreamId overflow");
        const auto streamId = StreamId;
        StreamId += 2;
        const auto messageId = SendVoiceInputMessage(topic, streamId, extraParams, reloadTimestamp);
        const auto isSendStream = (voiceLength.Defined() && *voiceLength > 0);
        if (isSendStream) {
            SendStream(inputStream, streamId, messageId);
        }

        TResponses responses(Reserve(5));
        ReceiveResponses(messageId, streamId, isSendStream, responses);
        return responses;
    }

    TResponses TAsyncUniproxyClient::SendCustomEventType(TStringBuf eventType,
                                                         const TExtraRequestParams& extraParams,
                                                         bool reloadTimestamp) {
        TJsonValue request = CreateVinsMessage(TStringBuf("TextInput"), eventType, extraParams, reloadTimestamp);
        const auto messageId = SendMessage(request);
        TResponses responses(Reserve(3));
        ReceiveResponses(messageId, responses);
        return responses;
    }

    void TAsyncUniproxyClient::ReceiveResponses(TStringBuf messageId, TResponses& responses) {
            return ReceiveResponses(messageId, Nothing(), false, responses);
    }

    void TAsyncUniproxyClient::ReceiveResponses(TStringBuf messageId,
                                                TMaybe<TStreamId> streamId,
                                                bool isSendStream,
                                                TResponses& responses) {
        TJsonValue recognition{};
        bool isTrash = false;
        bool should_stop = false;
        while (!should_stop) {
            const auto response = ReceiveTextResponse(&IsMessageToSkip);
            const auto* directive = GetDirectiveOrNull(response.JsonValue);

            // Handle stream control
            if (directive == nullptr) {
                ValidateStreamControl(response.JsonValue, *streamId);
                TJsonValue::TArray array{};
                bool good_recognition = (recognition.GetArray(&array) && !array.empty() && !isTrash && !IsEmptyHypothesis(array));
                if (!good_recognition) {
                    LOG_INFO("Nothing has been recognized. No more responses are expected\n");
                    break;
                }
                continue;
            }

            const auto* header = GetHeaderOrNull(*directive);
            if (header == nullptr) {
                ythrow TUniproxyInteractionError() << "Invalid response. No header";
            }
            const auto* uniproxyNamespace = GetMapStringValueOrNull(*header, "namespace");
            if (uniproxyNamespace == nullptr) {
                ythrow TUniproxyInteractionError() << "No uniproxy namespace in response header";
            }

            const auto condition = (ValidUniproxyNamespaces.contains(*uniproxyNamespace) ? ValidUniproxyNamespaces.at(*uniproxyNamespace) : AllUniproxyNamespaces::None);
            switch (condition) {
                case AllUniproxyNamespaces::ASR: {
                    ValidateDirectiveNamespaceNameAndRefMessageId(*directive, TStringBuf("ASR"),
                                                                  TStringBuf("Result"), messageId);
                    responses.emplace_back(EResponseType::Asr, response.Text);

                    const auto& payload = GetDirectivePayload(*directive);
                    if (isSendStream) {
                        TJsonValue responseCode{};
                        if (!payload.GetValue(TStringBuf("responseCode"), &responseCode)) {
                            ythrow TUniproxyInteractionError() << "Invalid ASR.Result payload: missing responseCode";
                        }
                        if (!responseCode.IsString()) {
                            ythrow TUniproxyInteractionError()
                            << "Invalid ASR.Result payload: responseCode should be a string";
                        }
                        const auto& responseCodeValue = responseCode.GetString();
                        if (responseCodeValue != TStringBuf("OK")) {
                            ythrow TUniproxyInteractionError() << "ASR.Result responseCode is not OK: "
                            << responseCodeValue;
                        }
                    }
                    payload.GetValue(TStringBuf("recognition"), &recognition);
                    isTrash = GetPayloadIsTrash(payload);
                    break;
                }
                case AllUniproxyNamespaces::Biometry: {
                    ValidateDirectiveNamespaceNameAndRefMessageId(*directive, TStringBuf("Biometry"),
                                                                  TStringBuf("Classification"), messageId);
                    responses.emplace_back(EResponseType::Bio, response.Text);
                    break;
                }
                case AllUniproxyNamespaces::Vins: {
                    ValidateDirectiveNamespaceNameAndRefMessageId(*directive, TStringBuf("Vins"),
                                                                  TStringBuf("VinsResponse"), messageId);
                    responses.emplace_back(EResponseType::Vins, response.Text);
                    if (FlagsContainer && FlagsContainer->Has("disable_tts")) {
                        LOG_INFO("Flag \"disable_tts\" has set. No TTS responses expected\n");
                        should_stop = true;
                        break;
                    }
                    if (FlagsContainer && FlagsContainer->Has("silent_vins")) {
                        LOG_INFO("Flag \"silent_vins\" has set. No TTS responses expected\n");
                        should_stop = true;
                        break;
                    }

                    if (HasSpeechMarker(response.JsonValue)) {
                        LOG_INFO("VinsResponse has a speech marker. TTS responses are expected\n");
                    } else {
                        LOG_INFO("VinsResponse has no speech marker. No TTS responses expected\n");
                        should_stop = true;
                    }
                    break;
                }
                case AllUniproxyNamespaces::TTS: {
                    HandleTtsResponses(response, messageId, responses);
                    should_stop = true;
                    break;
                }
                case AllUniproxyNamespaces::System: {
                    const auto* uniproxyName = GetMapStringValueOrNull(*header, "name");
                    if (uniproxyName != nullptr && *uniproxyName == "EventException") {
                        const auto& payload = GetDirectivePayload(*directive);
                        TJsonValue error{};
                        if (!payload.GetValue("error", &error)) {
                            ythrow TUniproxyInteractionError() << "No error in EventException event";
                        }
                        const auto* message = GetMapStringValueOrNull(error, "message");
                        ythrow TUniproxyInteractionError() << TString(SubstrUTF8(*message, 0, UniproxyErrorMessageBoundary));
                    } else if (uniproxyName != nullptr && *uniproxyName == "MatchVoicePrint") {
                        LOG_INFO("Ignoring event MatchVoicePrint https://st.yandex-team.ru/MEGAMIND-3364");
                    } else {
                        ythrow TUniproxyInteractionError() << "Invalid Uniproxy response namespace: "
                                                           << *uniproxyNamespace << ", name: "
                                                           << *uniproxyName;
                    }
                    break;
                }
                default:
                    ythrow TUniproxyInteractionError() << "Invalid Uniproxy response namespace: "
                                                       << *uniproxyNamespace;
            }
        }
    }
}
