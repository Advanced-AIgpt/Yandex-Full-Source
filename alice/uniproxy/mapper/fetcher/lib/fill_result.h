#pragma once

#include <alice/uniproxy/mapper/uniproxy_client/lib/uniproxy_client.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>

namespace NAlice::NUniproxy::NFetcher {
    template <typename TOutput>
    void FillResult(TResponses const& responses, TOutput& result) {
        using namespace NJson;
        TVector<TStringBuf> asrResponses;
        asrResponses.reserve(2);
        for (auto const& r : responses) {
            switch (r.Type) {
                case EResponseType::Asr: {
                    asrResponses.emplace_back(r.Data);
                    break;
                }
                case EResponseType::Bio: {
                    result.SetBioResponse(r.Data);
                    break;
                }
                case EResponseType::Vins: {
                    result.SetVinsResponse(r.Data);
                    break;
                }
                case EResponseType::TtsText: {
                    result.SetTtsTextResponse(r.Data);
                    break;
                }
                case EResponseType::TtsStream: {
                    result.SetTtsSpeechResponse(r.Data);
                    break;
                }
                default:
                    ythrow NHelpers::TUniproxyInteractionError() << "Unsupported response type " << r.Type;
            }
        }
        if (asrResponses.empty()) {
            return;
        }
        TStringBuilder builder{};
        builder << "[" << asrResponses.front();
        for (size_t i = 1; i < asrResponses.size(); ++i) {
            builder << ", " << asrResponses[i];
        }
        builder << "]";
        result.SetAsrResponses(builder);
    }

}
