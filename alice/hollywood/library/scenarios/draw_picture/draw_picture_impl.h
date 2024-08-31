#pragma once

#include "draw_picture_resources.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>


namespace NAlice::NHollywood {

    class TDrawPictureImpl {
    public:
        static const size_t RankingSize = 100;
        static const size_t SuggestsNum = 3;

        class TResponse {
        public:
            TResponse(const TString& url, const TDrawPictureResources::TComment& comment) : Url(&url), Comment(&comment) {
            }

            inline const TString& GetUrl() const {
                return *Url;
            }

            inline const TDrawPictureResources::TComment& GetComment() const {
                return *Comment;
            }

        private:
            const TString* Url;
            const TDrawPictureResources::TComment* Comment;
        };

        explicit TDrawPictureImpl(TScenarioHandleContext& ctx);
        void RenderDrawPicture(const TResponse &response);
        void RejectDrawPicture(const TStringBuf &text, bool isIrrelevant);

        TResponse GetRankedRandomImage(const NMilab::NI2t::TI2tVector &features);
        TMaybe<TResponse> LookupSubstitutes(const TString& request);
        TResponse GetRandomImage();

        template<typename T>
        inline const T& RandomChoice(const TVector<T>& collection) {
            return collection[Ctx.Rng.RandomInteger(collection.size())];
        }

    private:
        void AddSuggest(
                TResponseBodyBuilder& bodyBuilder,
                const TString& actionId,
                const TString& phrase,
                const TString& title,
                bool addHints);

    public:
        TScenarioHandleContext& Ctx;
        const NScenarios::TScenarioRunRequest RequestProto;
        const TScenarioRunRequestWrapper Request;
        const TFrame Frame;
        const TDrawPictureResources& Data;
    };

} // namespace NAlice::NHollywood
