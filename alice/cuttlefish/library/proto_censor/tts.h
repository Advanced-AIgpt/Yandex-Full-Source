#pragma once

#include <alice/cuttlefish/library/protos/tts.pb.h>

namespace NAlice::NCuttlefish {
    void Censore(::NTts::TRequest& ttsRequest);
    void Censore(::NTts::TBackendRequest& ttsBackendRequest);
    void Censore(::NTts::TRequestSenderRequest& ttsRequestSenderRequest);
    void Censore(::NTts::TAggregatorRequest& ttsAggregatorRequest);

    template<typename T>
    inline TString GetCensoredTtsRequestStr(const T& proto) {
        T protoCopy = proto;
        Censore(protoCopy);
        return protoCopy.ShortUtf8DebugString();
    }

    inline TString GetCensoredTtsAudioPartToGenerateStr(const ::NTts::TAudioPartToGenerate& proto, bool censore) {
        auto protoCopy = proto;
        if (censore) {
            if (protoCopy.HasText() && !protoCopy.GetText().empty()) {
                protoCopy.SetText("<CENSORED>");
            }
        }
        return protoCopy.ShortUtf8DebugString();
    }
}
