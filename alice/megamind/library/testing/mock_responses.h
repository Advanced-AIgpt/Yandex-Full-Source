#pragma once

#include <alice/megamind/library/context/responses.h>

#include <dj/services/alisa_skills/server/proto/client/proactivity_response.pb.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

struct TMockResponses : public IResponses {
    TMockResponses();

    MOCK_METHOD(const TPolyglotTranslateUtteranceResponse&, PolyglotTranslateUtteranceResponse, (TStatus*), (const, override));
    MOCK_METHOD(const TWizardResponse&, WizardResponse, (TStatus*), (const, override));
    MOCK_METHOD(const TBlackBoxFullUserInfoProto&, BlackBoxResponse, (TStatus*), (const, override));
    MOCK_METHOD(const TEntitySearchResponse&, EntitySearchResponse, (TStatus*), (const, override));
    MOCK_METHOD(const NKvSaaS::TPersonalIntentsResponse&, PersonalIntentsResponse, (TStatus*), (const, override));
    MOCK_METHOD(const TMisspellProto&, MisspellResponse, (TStatus*), (const, override));
    MOCK_METHOD(const NKvSaaS::TTokensStatsResponse&, QueryTokensStatsResponse, (TStatus*), (const, override));
    MOCK_METHOD(const NScenarios::TSkillDiscoverySaasCandidates&, SaasSkillDiscoveryResponse, (TStatus*), (const, override));
    MOCK_METHOD(const NDJ::NAS::TProactivityResponse&, ProactivityResponse, (TStatus*), (const, override));
    MOCK_METHOD(const TSearchResponse&, WebSearchResponse, (TStatus*), (const, override));
    MOCK_METHOD(const NMegamindAppHost::TWebSearchQueryProto&, WebSearchQueryResponse, (TStatus*), (const, override));
    MOCK_METHOD(const NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest&, BegemotResponseRewrittenRequestResponse, (TStatus*), (const, override));
    MOCK_METHOD(const TSessionProto&, SpeechKitSessionResponse, (TStatus*), (const, override));

    void SetWizardResponse(TWizardResponse&& wizardResponse);
    void SetBlackBoxResponse(TBlackBoxFullUserInfoProto&& blackBoxResponse);
    void SetPersonalIntentsResponse(NKvSaaS::TPersonalIntentsResponse&& personalIntentsResponse);

    //static TMockResponses& Create();

private:
    TWizardResponse WizardResponse_;
    TBlackBoxFullUserInfoProto BlackBoxResponse_;
    NKvSaaS::TPersonalIntentsResponse PersonalIntentsResponse_;
    NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest BegemotResponseRewrittenRequestResponse_;
};

} // namespace NAlice
