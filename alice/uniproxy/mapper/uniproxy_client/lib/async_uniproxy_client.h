#pragma once

#include "uniproxy_client.h"

#include <unordered_map>

namespace NAlice::NUniproxy {
    enum class AllUniproxyNamespaces: ui32 { None,
                                             ASR,
                                             Biometry,
                                             Vins,
                                             TTS,
                                             System };

    class TAsyncUniproxyClient: public TUniproxyClient {
    public:
        using TUniproxyClient::TUniproxyClient;

        TResponses SendVoiceRequest(TStringBuf topic, IInputStream& inputStream,
                                    const TExtraVoiceRequestParams& extraParams = {},
                                    bool reloadTimestamp = true,
                                    const TMaybe<size_t> voiceLength = Nothing()) override;

    public:
        const std::unordered_map<TString, AllUniproxyNamespaces> ValidUniproxyNamespaces{
            {"ASR", AllUniproxyNamespaces::ASR},
            {"Biometry", AllUniproxyNamespaces::Biometry},
            {"Vins", AllUniproxyNamespaces::Vins},
            {"TTS", AllUniproxyNamespaces::TTS},
            {"System", AllUniproxyNamespaces::System},
        };
        const size_t UniproxyErrorMessageBoundary = 100;

    protected:
        TResponses SendCustomEventType(TStringBuf eventType, const TExtraRequestParams& extraParams,
                                       bool reloadTimestamp) override;

    private:
        void ReceiveResponses(TStringBuf messageId, TResponses& responses);
        void ReceiveResponses(TStringBuf messageId,
                              TMaybe<NHelpers::TStreamId> streamId,
                              bool isSendStream,
                              TResponses& responses);
    };
}
