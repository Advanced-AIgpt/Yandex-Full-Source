#pragma once

#include "builder.h"

#include <alice/megamind/library/response/proto/response.pb.h>

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/scenarios/features/protos/features.pb.h>
#include <alice/megamind/library/scenarios/interface/response_builder.h>
#include <alice/megamind/library/scenarios/interface/scenario.h>

#include <alice/megamind/protos/common/frame.pb.h>

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/yexception.h>

#include <functional>

namespace NAlice {

class TScenarioResponse : public NNonCopyable::TMoveOnly {

    using TResponseBody = NScenarios::TScenarioResponseBody;

public:
    enum class EScenarioType {
        Protocol,
        Other
    };

    TScenarioResponse(const TString& scenarioName,
                      const TVector<TSemanticFrame>& scenarioSemanticFrames, bool scenarioAcceptsAnyUtterance);

    virtual ~TScenarioResponse() = default;

    TScenarioResponse(TScenarioResponse&& other) = default;

    TScenarioResponse& operator=(TScenarioResponse&& other) = default;

    const TString& GetScenarioName() const;

    bool GetRequestIsExpected() const {
        return RequestIsExpected;
    }

    void SetRequestIsExpected(bool requestIsExpected) {
        RequestIsExpected = requestIsExpected;
    }

    bool GetScenarioAcceptsAnyUtterance() const {
        return CommonProto().GetScenarioAcceptsAnyUtterance();
    }

    TSemanticFrame GetResponseSemanticFrame() const;

    TResponseBuilder* BuilderIfExists();
    const TResponseBuilder* BuilderIfExists() const;

    virtual TResponseBuilder& ForceBuilder(const TSpeechKitRequest& skr, const TRequest& requestModel,
                                           const NMegamind::IGuidGenerator& guidGenerator,
                                           const TMaybe<TString>& serializerScenarioName = Nothing());
    TResponseBuilder& ForceBuilder(const TSpeechKitRequest& skr, const TRequest& requestModel,
                                   const NMegamind::IGuidGenerator& guidGenerator,
                                   TResponseBuilderProto&& initializedProto);
    TResponseBuilder* ForceBuilderFromSession(const TSpeechKitRequest& skr,
                                              const TRequest& requestModel,
                                              const NMegamind::IGuidGenerator& guidGenerator,
                                              const ISession& session);

    void SetFeatures(const TFeatures& features) {
        *CommonProto().MutableFeatures() = features;
    }

    const TFeatures& GetFeatures() const {
        return CommonProto().GetFeatures();
    }

    const TString& GetIntentFromFeatures() const {
        return GetFeatures().GetScenarioFeatures().GetIntent();
    }

    void SetResponseBody(const TResponseBody& responseBody);
    TResponseBody* ResponseBodyIfExists();
    const TResponseBody* ResponseBodyIfExists() const;

    void SetHttpCode(HttpCodes httpCode, TMaybe<TString> reason = Nothing());
    TMaybe<HttpCodes> GetHttpCode() const;
    TMaybe<TString> GetHttpErrorReason() const;

    void SetScenarioType(EScenarioType scenarioType);
    EScenarioType GetScenarioType() const;

    const TVector<TSemanticFrame>& GetRequestSemanticFrames() const {
        return RequestSemanticFrames;
    }

    void SetContinueResponse(const NScenarios::TScenarioContinueResponse& continueResponse);
    NScenarios::TScenarioContinueResponse* ContinueResponseIfExists();
    const NScenarios::TScenarioContinueResponse* ContinueResponseIfExists() const;

protected:
    struct TResponseBuilderCase : public NNonCopyable::TMoveOnly {
        TResponseBuilderCase(const TSpeechKitRequest& skr, const TRequest& requestModel,
                             const NMegamind::IGuidGenerator& guidGenerator,
                             TResponseBuilderProto&& builderProto, TScenarioResponseCommonProto&& proto);
        TResponseBuilderCase(const TSpeechKitRequest& skr, const TRequest& requestModel,
                             const NMegamind::IGuidGenerator& guidGenerator,
                             TString&& scenarioName, TScenarioResponseCommonProto&& proto,
                             const TMaybe<TString>& serializerScenarioName);

        virtual ~TResponseBuilderCase() = default;

    protected:
        TResponseBuilderCase(TScenarioResponseCommonProto&& proto);

    public:

        const TString& Name() const {
            return Proto.GetProto().GetScenarioName();
        }

        const TScenarioResponseCommonProto& CommonProto() const {
            return Proto.GetCommon();
        }

        TScenarioResponseCommonProto& CommonProto() {
            return *Proto.MutableCommon();
        }

        TSemanticFrame GetResponseSemanticFrame() const {
            return Builder->GetSemanticFrame() ? Builder->GetSemanticFrame().GetRef() : TSemanticFrame{};
        }

        TScenarioResponseBuilderProto Proto;
        std::unique_ptr<TResponseBuilder> Builder;
    };
protected:
    const TScenarioResponseCommonProto& CommonProto() const;
    TScenarioResponseCommonProto& CommonProto();

    std::unique_ptr<TResponseBuilderCase> Response;
private:
    TVector<TSemanticFrame> RequestSemanticFrames;
    EScenarioType ScenarioType;
    bool RequestIsExpected;
    TMaybe<TResponseBody> ResponseBody;
    TMaybe<NScenarios::TScenarioContinueResponse> ContinueResponse;
protected:
    TString ScenarioName;
private:
    TScenarioResponseCommonProto CommonProtoInit;
};

} // namespace NAlice
