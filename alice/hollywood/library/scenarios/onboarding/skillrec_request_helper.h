#pragma once

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/framework.h>

#include <dj/services/alisa_skills/server/proto/client/proactivity_request.pb.h>
#include <dj/services/alisa_skills/server/proto/client/request.pb.h>

namespace NAlice::NHollywoodFw::NOnboarding {

    constexpr inline TStringBuf SKILLREC_REQUEST_KEY = "onboarding_greetings_request";
    constexpr inline TStringBuf SKILLREC_RESPONSE_KEY = "onboarding_greetings_response";

    class ISkillRecRequest {
    public:
        virtual ~ISkillRecRequest() = default;
    public:
        virtual TString GetPath() const = 0;
        virtual TStringBuf GetContentType() const = 0;
        virtual TString GetBody() const = 0;
    };

    class TGreetingsRequestOld : public ISkillRecRequest {
    public:
        explicit TGreetingsRequestOld(const TRunRequest& request);
    public:
        TString GetPath() const override;
        TStringBuf GetContentType() const override;
        TString GetBody() const override;
    private:
        void SetupCgi(const TRunRequest&);
        void SetupProtoReq(const TRunRequest&);
        void FillClientProperties(const TClientInfo&);
        void FillFeatures(const NScenarios::TInterfaces&);
        void FillFlags(const TRequest::TFlags&);
    private:
        NDJ::NAS::TServiceRequest ProtoReq_;
        TCgiParameters Cgi_;
    };

    class TOnboardingRequest : public ISkillRecRequest {
    public:
        explicit TOnboardingRequest(const TRunRequest& request, const TStorage& storage, TStringBuf cardName);

        TStringBuf GetContentType() const override;
        TString GetBody() const override;

    protected:
        void SetupProtoReq(const TRunRequest& request, const TStorage& storage);
        void SetupCgi(const TRunRequest& request, TStringBuf cardName);

    protected:
        NDJ::NAS::TProactivityRequest ProtoReq_;
        TCgiParameters Cgi_;
    };

    class TGreetingsRequestNew : public TOnboardingRequest {
    public:
        explicit TGreetingsRequestNew(const TRunRequest& request, const TStorage& storage);

        TString GetPath() const override;
    };

    class TWhatCanYouDoRequest : public TOnboardingRequest {
    public:
        explicit TWhatCanYouDoRequest(const TRunRequest& request, const TStorage& storage);

        TString GetPath() const override;
    };

} // namespace NAlice::NHollywoodFw::NOnboarding
