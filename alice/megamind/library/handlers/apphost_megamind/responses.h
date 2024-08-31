#pragma once

#include <alice/megamind/library/apphost_request/node.h>

#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/walker/source_response_holder.h>

namespace NAlice::NMegamind {

class TAppHostSourceResponses : public IResponses {
public:
    // Overrides of IResponses.
    const TPolyglotTranslateUtteranceResponse& PolyglotTranslateUtteranceResponse(TStatus* status = nullptr) const override {
        if (!PolyglotTranslateUtteranceResponse_.Defined()) {
            PolyglotTranslateUtteranceResponse_.ConstructInPlace();
            InitPolyglotTranslateUtteranceResponse();
        }
        return PolyglotTranslateUtteranceResponse_->Get(status);
    }
    const TBlackBoxFullUserInfoProto& BlackBoxResponse(TStatus* status = nullptr) const override {
        if (!BlackBoxResponse_.Defined()) {
            BlackBoxResponse_.ConstructInPlace();
            InitBlackBoxResponse();
        }
        return BlackBoxResponse_->Get(status);
    }
    const TEntitySearchResponse& EntitySearchResponse(TStatus* status = nullptr) const override {
        if (!EntitySearchResponse_.Defined()) {
            EntitySearchResponse_.ConstructInPlace();
            InitEntitySearchResponse();
        }
        return EntitySearchResponse_->Get(status);
    }
    const TMisspellProto& MisspellResponse(TStatus* status = nullptr) const override {
        if (!MisspellResponse_.Defined()) {
            MisspellResponse_.ConstructInPlace();
            InitMisspellResponse();
        }
        return MisspellResponse_->Get(status);
    }
    const NKvSaaS::TPersonalIntentsResponse& PersonalIntentsResponse(TStatus* status = nullptr) const override {
        if (!PersonalIntentsResponse_.Defined()) {
            PersonalIntentsResponse_.ConstructInPlace();
            InitPersonalIntentsResponse();
        }
        return PersonalIntentsResponse_->Get(status);
    }
    const NKvSaaS::TTokensStatsResponse& QueryTokensStatsResponse(TStatus* status = nullptr) const override {
        if (!QueryTokensStatsResponse_.Defined()) {
            QueryTokensStatsResponse_.ConstructInPlace();
            InitQueryTokensStatsResponse();
        }
        return QueryTokensStatsResponse_->Get(status);
    }
    const NScenarios::TSkillDiscoverySaasCandidates&
    SaasSkillDiscoveryResponse(TStatus* status = nullptr) const override {
        if (!SaasSkillDiscoveryResponse_.Defined()) {
            SaasSkillDiscoveryResponse_.ConstructInPlace();
            InitSaasSkillDiscoveryResponse();
        }
        return SaasSkillDiscoveryResponse_->Get(status);
    }
    const TWizardResponse& WizardResponse(TStatus* status = nullptr) const override {
        if (!WizardResponse_.Defined()) {
            WizardResponse_.ConstructInPlace();
            InitWizardResponse();
        }
        return WizardResponse_->Get(status);
    }
    const NDJ::NAS::TProactivityResponse& ProactivityResponse(TStatus* status = nullptr) const override {
        if (!ProactivityResponse_.Defined()) {
            ProactivityResponse_.ConstructInPlace();
            InitProactivityResponse();
        }
        return ProactivityResponse_->Get(status);
    }
    const TSearchResponse& WebSearchResponse(TStatus* status = nullptr) const override {
        if (!WebSearchResponse_.Defined()) {
            InitWebSearchResponse();
        }
        return WebSearchResponse_->Get(status);
    }
    const NMegamindAppHost::TWebSearchQueryProto& WebSearchQueryResponse(TStatus* status = nullptr) const override {
        if (!WebSearchQueryResponse_.Defined()) {
            WebSearchQueryResponse_.ConstructInPlace();
            InitWebSearchQueryResponse();
        }
        return WebSearchQueryResponse_->Get(status);
    }
    const NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest& BegemotResponseRewrittenRequestResponse(TStatus* status = nullptr) const override {
        if (!BegemotResponseRewrittenRequestResponse_.Defined()) {
            BegemotResponseRewrittenRequestResponse_.ConstructInPlace();
            InitBegemotResponseRewrittenRequestResponse();
        }
        return BegemotResponseRewrittenRequestResponse_->Get(status);
    }
    const TSessionProto& SpeechKitSessionResponse(TStatus* status = nullptr) const override {
        if (!SpeechKitSessionResponse_.Defined()) {
            SpeechKitSessionResponse_.ConstructInPlace();
            InitSpeechKitSessionResponse();
        }
        return SpeechKitSessionResponse_->Get(status);
    }

private:
    virtual void InitPolyglotTranslateUtteranceResponse() const {}
    virtual void InitBlackBoxResponse() const {}
    virtual void InitEntitySearchResponse() const {}
    virtual void InitMisspellResponse() const {}
    virtual void InitPersonalIntentsResponse() const {}
    virtual void InitQueryTokensStatsResponse() const {}
    virtual void InitSaasSkillDiscoveryResponse() const {}
    virtual void InitWizardResponse() const {}
    virtual void InitProactivityResponse() const {}
    virtual void InitWebSearchResponse() const {}
    virtual void InitWebSearchQueryResponse() const {}
    virtual void InitBegemotResponseRewrittenRequestResponse() const {}
    virtual void InitSpeechKitSessionResponse() const {}

protected:
    mutable TMaybe<TSourceResponseHolder<TPolyglotTranslateUtteranceResponse>> PolyglotTranslateUtteranceResponse_;
    mutable TMaybe<TSourceResponseHolder<TWizardResponse>> WizardResponse_;
    mutable TMaybe<TSourceResponseHolder<TMisspellProto>> MisspellResponse_;
    mutable TMaybe<TSourceResponseHolder<TEntitySearchResponse>> EntitySearchResponse_;
    mutable TMaybe<TSourceResponseHolder<TBlackBoxFullUserInfoProto>> BlackBoxResponse_;
    mutable TMaybe<TSourceResponseHolder<NKvSaaS::TPersonalIntentsResponse>> PersonalIntentsResponse_;
    mutable TMaybe<TSourceResponseHolder<NKvSaaS::TTokensStatsResponse>> QueryTokensStatsResponse_;
    mutable TMaybe<TSourceResponseHolder<NScenarios::TSkillDiscoverySaasCandidates>> SaasSkillDiscoveryResponse_;
    mutable TMaybe<TSourceResponseHolder<NDJ::NAS::TProactivityResponse>> ProactivityResponse_;
    mutable TMaybe<TSourceResponseHolder<TSearchResponse>> WebSearchResponse_;
    mutable TMaybe<TSourceResponseHolder<NMegamindAppHost::TWebSearchQueryProto>> WebSearchQueryResponse_;
    mutable TMaybe<TSourceResponseHolder<NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest>> BegemotResponseRewrittenRequestResponse_;
    mutable TMaybe<TSourceResponseHolder<TSessionProto>> SpeechKitSessionResponse_;

};

} // namepspace NAlice::NMegamind
