#pragma once

#include <dict/mt/libs/nn_base/nn_base.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NGenerativeBoltalka {

    static const char* DELIMETERS = " \t\r\n!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

    struct TGenerativeRequest {
        TVector<TString> Context;
        TVector<int> InputIds;
        TMaybe<int> Seed;
        size_t NumHypos;
        bool PrefixOnly;
        TVector<TString> SpanDelimiters;
        TVector<TVector<int>> SpanDelimitersIds;
        bool DiverseBeamSearch;
        const TVector<TVector<float>>* PtuneEmbeddings;
        NDict::NMT::TPtuneBlock PtuneBlock;
        TMaybe<NDict::NMT::TSamplerParams> SamplerParams;
        TMaybe<size_t> MinOutLen;
        TMaybe<size_t> MaxOutLen;
    };

    struct TGenerativeResponse {
        TGenerativeResponse(TString response, float score, size_t numTokens, TMaybe<TString> externalInfo = Nothing())
            : Response(std::move(response))
            , Score(score)
            , NumTokens(numTokens)
            , ExternalInfo(externalInfo)
        {
            for (const auto& it : StringSplitter(Response).SplitBySet(DELIMETERS)) {
                const auto& token = it.Token();
                const auto& delim = it.Delim();

                if (token.Size() > 0) {
                    ResponseWords.push_back(TString(token));
                }
                if (delim.Size() > 0 && delim != " ") {
                    ResponseWords.push_back(TString(delim));
                }
            }
        }

        TString Response;
        TVector<TString> ResponseWords;
        float Score;
        size_t NumTokens;
        TMaybe<TString> ExternalInfo;
    };

} // namespace NGenerativeBoltalka

