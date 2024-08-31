#pragma once

#include "helpers.h"
#include "base_client.h"

#include <alice/uniproxy/mapper/library/flags/container.h>
#include <alice/uniproxy/mapper/library/sensors/constants.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/system/types.h>

#include <optional>

namespace NAlice::NUniproxy {
    /// @brief Common parameters
    struct TUniproxyClientParams: public TBaseClientParams {
        TString UniproxyUrl;                    //!< Uniproxy url
        TString AuthToken;                      //!< Uniproxy auth token of device
        TString Uuid;                           //!< Uuid of fake-user
        i64 AsrChunkSize = 0;                   //!< Size of sound chunk for sending to ASR
        ui64 AsrChunkDelayMs = 0;               //!< Timeout ms between sending of sound chunks to ASR
        NJson::TJsonValue Application;          //!< The application data
        TString ApplicationId;                  //!< The application id (deprecated)
        TString ApplicationVersion;             //!< The application version (deprecated)
        TString OsVersion;                      //!< The OS version (deprecated)
        TString Platform;                       //!< The application platform (deprecated)
        TString Language;                       //!< The client language
        TString Timezone;                       //!< The client timezone
        TString OAuthToken;                     //!< The user OAuth token
        TString VinsUrl;                        //!< The VINS service URL
        NJson::TJsonValue SyncStateExperiments; //!< Default download experiments
        bool DisableLocalExperiments = false;   //!< Do not apply experiments on uniproxy
        TString ShootingSource;                 //!< Shooting source
        NJson::TJsonValue SupportedFeatures;    //!< Supported client features
    };

    /// @brief Optional request parameters
    struct TExtraRequestParams {
        std::optional<TString> RequestId;                 //!< The request id. A new request id will be generated, if not specified
        NJson::TJsonValue PayloadTemplate; //!< The request payload template
    };

    /// @brief Optional voice request parameters
    struct TExtraVoiceRequestParams: public TExtraRequestParams {
        std::optional<TString> AudioFormat; //!< The audio format. "audio/opus", if not specified
    };

    /// @brief Optional voice request parameters
    struct TExtraTtsGenerateParams {
        std::optional<TString> AudioFormat; //!< The audio format. "audio/opus", if not specified
        std::optional<TString> Voice;       //!< Name of the voice. "shitova.gpu", if not specified
        std::optional<TString> Speed;       //!< Speed of the voice. "1", if not specified
        std::optional<TString> Emotion;     //!< Emotion of the voice. "neutral", if not specified
        std::optional<TString> Quality;     //!< Quality of the voice. "UltraHigh", if not specified
    };

    /// @brief The response type
    enum class EResponseType {
        Asr,      //!< ASR text response
        Bio,      //!< Bio text response
        Vins,     //!< VINS text response
        TtsText,  //!< TTS text response
        TtsStream //!< TTS stream response
    };
    const THashSet<EResponseType> TextTypes{EResponseType::Asr, EResponseType::Bio, EResponseType::Vins,
                                            EResponseType::TtsText};
    const THashSet<EResponseType> VoiceTypes{EResponseType::TtsStream};

    /// @brief A single response
    struct TResponse {
        EResponseType Type = EResponseType::Asr;     //!< The response type
        TString Data;                                //!< The response data
        std::optional<NHelpers::TStreamId> StreamId; //!< The stream id, optional

        TResponse(EResponseType type, TString data)
            : Type(type)
            , Data(std::move(data))
        {
        }

        TResponse(EResponseType type, TString data, NHelpers::TStreamId streamId)
            : Type(type)
            , Data(std::move(data))
            , StreamId(streamId)
        {
        }

        size_t textSize() const {
            return TextTypes.contains(Type) ? Data.length() : 0;
        }

        size_t voiceSize() const {
            return VoiceTypes.contains(Type) ? Data.length() : 0;
        }
    };

    /// @brief The responses type
    using TResponses = TVector<TResponse>;

    /// @brief The VINS client
    class TUniproxyClient {
    protected:
        TLog* Logger;
        TFlagsContainer* FlagsContainer;
        NAlice::NUniproxy::TSensorContainer* Sensors;
        TBaseClient Client;
        TUniproxyClientParams UniproxyClientParams;
        NHelpers::TStreamId StreamId = 1;

    public:
        /** @brief Constructs a new Uniproxy client and send SynchronizeState
         *
         * @param uniproxyUri The uniproxy URL
         * @param apiKey The API key (AuthToken)
         * @param instanceUuid The application instance UUID
         * @param params Additional common parameters
         * @param asrChunkSize ASR sound chunk size
         */
        explicit TUniproxyClient(const TUniproxyClientParams& params);

        virtual ~TUniproxyClient();

        /** @brief Send a text requests and receive responses
         *
         * @param text The request text
         * @param extraParams Additional request parameters
         * @return Responses
         */
        virtual TResponses SendTextRequest(TStringBuf text,
                                           TExtraRequestParams extraParams = {},
                                           bool reloadTimestamp = true);
        /** @brief Send a voice request
         *
         * @param topic The request topic
         * @param inputStream The voice input
         * @param extraParams Additional request parameters
         * @return Responses
         */
        virtual TResponses SendVoiceRequest(TStringBuf topic, IInputStream& inputStream,
                                            const TExtraVoiceRequestParams& extraParams = {},
                                            bool reloadTimestamp = true,
                                            const TMaybe<size_t> voiceLength = Nothing());
        /** @brief Send a request with server action and receive responses
         *
         * @param extraParams Additional request parameters
         * @return Responses
         */
        virtual TResponses SendServerAction(const TExtraRequestParams& extraParams, bool reloadTimestamp = true);
        /** @brief Send a request with image input, almost same as SendServerAction
         */
        virtual TResponses SendImageInput(const TExtraRequestParams& extraParams, bool reloadTimestamp = true);
        /** @brief Send a TTS.Generate request and receive responses
         *
         * @param text The request text
         * @param extraParams Additional request parameters
         * @return Responses
         */
        virtual TResponses GenerateVoice(TStringBuf text, const TExtraTtsGenerateParams& extraParams);

        virtual bool IsQuasar();

        /// @brief Returns the remote IP address
        virtual TString GetRemoteAddress() const;
        /// @brief Returns the remote port
        virtual ui16 GetRemotePort() const;

    protected:
        using TSkipFilterFunc = bool (*)(const NJson::TJsonValue&);
        struct TTextResponseResult {
            TString Text;
            NJson::TJsonValue JsonValue;
        };
        struct TResponseInternal {
            TString Data;
            std::optional<NHelpers::TStreamId> StreamId;
        };

        void Init();
        NJson::TJsonValue CreateVinsMessage(TStringBuf name, TStringBuf eventType,
                                            const TExtraRequestParams& extraParams, bool reloadTimestamp = true);
        TResponseInternal ReceiveResponse();
        TTextResponseResult ReceiveTextResponse();
        TTextResponseResult ReceiveTextResponse(TSkipFilterFunc skipFilter);

        virtual TResponses SendCustomEventType(TStringBuf eventType, const TExtraRequestParams& extraParams,
                                               bool reloadTimestamp);
        TString SendVoiceInputMessage(TStringBuf topic, NHelpers::TStreamId streamId,
                                      const TExtraVoiceRequestParams& extraParams, bool reloadTimestamp);
        TString SendMessage(const TJsonValue& request);
        void SendStream(IInputStream& inputStream, NHelpers::TStreamId streamId, const TString& messageId);

        bool ReceiveASRResponses(TStringBuf messageId, NHelpers::TStreamId streamId, TResponses& responses);
        void ReceiveBioVinsTTSResponses(TStringBuf messageId, TResponses& responses);
        void ReceiveTtsResponses(TStringBuf messageId, TResponses& responses);
        void HandleTtsResponses(const TTextResponseResult& response, TStringBuf messageId, TResponses& responses);
        bool HasSpeechMarker(const NJson::TJsonValue& vinsResponse);
    };
}
