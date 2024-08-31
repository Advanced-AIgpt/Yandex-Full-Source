#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <library/cpp/json/json_value.h>

#include <alice/hollywood/library/scenarios/transform_face/proto/transform_request.pb.h>

namespace NAlice::NHollywood {

struct TTransformFaceRunImpl {
    TScenarioHandleContext& Ctx;
    const NScenarios::TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;

    explicit TTransformFaceRunImpl(TScenarioHandleContext& ctx);
    void RequestPhoto();
    void Reject(const TStringBuf &text, const bool isIrrelevant);
    TTransformFaceRequest GetTransformRequest() const;
    void Continue(const TTransformFaceRequest& args) const;
    bool IsFaceTransformRequest() const;
};

struct TTransformFaceContinueImpl {
    struct TSuggest {
        TString Caption;
        TString Transform;
        TString Url;
        TVector<TString> NluHints;
        TVector<TString> Suggests;
    };

    TScenarioHandleContext& Ctx;
    const NScenarios::TScenarioApplyRequest RequestProto;
    const TScenarioApplyRequestWrapper Request;
    const TTransformFaceRequest Transform;


    explicit TTransformFaceContinueImpl(TScenarioHandleContext& ctx);
    void RenderResult(const TString& original, const TString& originalPreview,
                      const TString& result, const TString& resultPreview);
    void RenderNoFaceFound();

    void AddSuggest(TResponseBodyBuilder& bodyBuilder,
                                    const TSuggest& suggest, int actionIdx, bool addSuggest,
                                    NJson::TJsonMap* galleryItem = nullptr) const;
    template<typename T>
    inline const T& RandomChoice(const TVector<T>& collection) const {
        return collection[Ctx.Rng.RandomInteger(collection.size())];
    }

};

} // namespace NAlice::NHollywood
