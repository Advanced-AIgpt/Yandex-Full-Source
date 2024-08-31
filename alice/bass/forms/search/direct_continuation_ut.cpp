#include "direct_continuation.h"

#include <alice/bass/ut/helpers.h>
#include <alice/library/network/headers.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

using namespace NBASS;
using namespace NBASS::NDirectGallery;
using namespace NHttpFetcher;
using namespace NAlice::NTestingHelpers;
using namespace NTestingHelpers;
using namespace testing;

namespace {

class TMockMultiRequest : public IMultiRequest {
public:
    MOCK_METHOD(THolder<TRequest>, AddRequest, (const NUri::TUri&, const TRequestOptions&), (override));
    MOCK_METHOD(void, WaitAll, (TInstant), (override));
};

class TMockRequestAPI : public NHttpFetcher::IRequestAPI {
public:
    MOCK_METHOD(TRequestPtr, Request, (const NUri::TUri&, const TRequestOptions&, NHttpFetcher::IRequestEventListener*), (override));
    MOCK_METHOD(TRequestPtr, Request, (TStringBuf, const TRequestOptions&, NHttpFetcher::IRequestEventListener*), (override));
    MOCK_METHOD(IMultiRequest::TRef, MultiRequest, (NHttpFetcher::IRequestEventListener*), (override));
    MOCK_METHOD(IMultiRequest::TRef, WeakMultiRequest, (NHttpFetcher::IRequestEventListener*), (override));
};

class TMockUidProvider : public IUidProvider {
public:
    MOCK_METHOD(TMaybe<TString>, GetUid, (TContext&), (const, override));
};

const auto CONTINUATION_JSON = NSc::TValue::FromJson(R"(
{
    "IsFinished" : false,
    "ObjectTypeName" : "TDirectGalleryHitConfirmContinuation",
    "State" : "{\"context\":{\"form\":{\"name\":\"unit_test_form_handler.default\",\"slots\":[]}},\"hit_counter\":\"hit_counter_ref\",\"link_head\":\"link_head_ref_\",\"link_tails\":[\"link_tail_1\",\"link_tail_2\"]}"
})");

Y_UNIT_TEST_SUITE(DirectContinuation) {
    Y_UNIT_TEST(ToJson) {
        NTestingHelpers::TRequestJson request;
        auto ctx = MakeContext(NSc::TValue(request));
        TDirectGalleryHitConfirmContinuation continuation{
            ctx, "hit_counter_ref", "link_head_ref_", {"link_tail_1", "link_tail_2"}};
        const auto actual = continuation.ToJson();
        UNIT_ASSERT(EqualJson(CONTINUATION_JSON, actual));
    }

    Y_UNIT_TEST(FromJson) {
        NTestingHelpers::TRequestJson request;
        auto ctx = MakeContext(NSc::TValue(request));
        auto globalCtx = NBASS::IGlobalContext::MakePtr<TTestGlobalContext>();
        const NSc::TValue configPatch{};
        auto continuation = TDirectGalleryHitConfirmContinuation::FromJson(
            CONTINUATION_JSON, globalCtx, NSc::TValue::Null(), /* authHeader= */ TString{},
            /* appInfoHeader= */ TString{}, /* fakeTimeHeader= */ TString{}, /* userTicketHeader= */ Nothing(), configPatch);
        UNIT_ASSERT(continuation);
        UNIT_ASSERT(continuation->ToJson() == CONTINUATION_JSON);
    }

    Y_UNIT_TEST(ApplyNoRedirect) {
        NTestingHelpers::TRequestJson request;
        auto ctx = MakeContext(NSc::TValue(request));
        auto firstMultiRequest = MakeIntrusive<StrictMock<TMockMultiRequest>>();
        auto secondMultiRequest = MakeIntrusive<StrictMock<TMockMultiRequest>>();

        TRequestOptions requestOptions;

        auto returnHttpOkResponse = []() { return MakeHolder<TFakeRequest>(TString{}); };
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("hit_counter_ref"), _))
            .WillOnce(Invoke(returnHttpOkResponse));
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("link_head_ref_link_tail_1"), _))
            .WillOnce(Invoke(returnHttpOkResponse));
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("link_head_ref_link_tail_2"), _))
            .WillOnce(Invoke(returnHttpOkResponse));

        auto uidProvider = std::make_unique<TMockUidProvider>();
        EXPECT_CALL(*uidProvider, GetUid(_)).WillOnce(Return(MakeMaybe<TString>("12345")));

        auto requestAPI = std::make_unique<TMockRequestAPI>();
        InSequence sequence;
        EXPECT_CALL(*requestAPI, WeakMultiRequest(_)).WillOnce(Return(firstMultiRequest));
        EXPECT_CALL(*requestAPI, WeakMultiRequest(_)).WillOnce(Return(secondMultiRequest));

        TDirectGalleryHitConfirmContinuation continuation{ctx,
                                                          "hit_counter_ref",
                                                          "link_head_ref_",
                                                          {"link_tail_1", "link_tail_2"},
                                                          std::move(requestAPI),
                                                          std::move(uidProvider)};
        UNIT_ASSERT(!continuation.ApplyIfNotFinished());
    }

    Y_UNIT_TEST(ApplyWithRedirect) {
        NTestingHelpers::TRequestJson request;
        auto ctx = MakeContext(NSc::TValue(request));
        auto firstMultiRequest = MakeIntrusive<StrictMock<TMockMultiRequest>>();
        auto secondMultiRequest = MakeIntrusive<StrictMock<TMockMultiRequest>>();

        TRequestOptions requestOptions;

        THttpHeaders redir1Headers;
        redir1Headers.AddHeader(TString{NAlice::NNetwork::HEADER_LOCATION}, "redir_1");
        redir1Headers.AddHeader(TString{NAlice::NNetwork::HEADER_SET_COOKIE}, "yandexuid=1111; more_data");
        THttpHeaders redir2Headers;
        redir2Headers.AddHeader(TString{NAlice::NNetwork::HEADER_LOCATION}, "redir_2");
        redir2Headers.AddHeader(TString{NAlice::NNetwork::HEADER_SET_COOKIE}, "yandexuid=2222; more_data");

        auto returnHttpOkResponse = []() { return MakeHolder<TFakeRequest>(TString{}); };
        auto returnHttpRedirectResponse = [](THttpHeaders&& headers) {
            return [&headers]() {
                auto request = MakeHolder<TFakeRequest>(TString{}, HTTP_FOUND, THttpHeaders{headers});
                return request;
            };
        };
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("hit_counter_ref"), _))
            .WillOnce(Invoke(returnHttpOkResponse));
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("link_head_ref_link_tail_1"), _))
            .WillOnce(Invoke(returnHttpRedirectResponse(std::move(redir1Headers))));
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("link_head_ref_link_tail_2"), _))
            .WillOnce(Invoke(returnHttpRedirectResponse(std::move(redir2Headers))));

        EXPECT_CALL(*secondMultiRequest, AddRequest(ParseUri("redir_1"), _)).WillOnce(Invoke(returnHttpOkResponse));
        EXPECT_CALL(*secondMultiRequest, AddRequest(ParseUri("redir_2"), _)).WillOnce(Invoke(returnHttpOkResponse));

        auto uidProvider = std::make_unique<TMockUidProvider>();
        EXPECT_CALL(*uidProvider, GetUid(_)).WillOnce(Return(Nothing()));

        auto requestAPI = std::make_unique<TMockRequestAPI>();
        InSequence sequence;
        EXPECT_CALL(*requestAPI, WeakMultiRequest(_)).WillOnce(Return(firstMultiRequest));
        EXPECT_CALL(*requestAPI, WeakMultiRequest(_)).WillOnce(Return(secondMultiRequest));

        TDirectGalleryHitConfirmContinuation continuation{ctx,
                                                          "hit_counter_ref",
                                                          "link_head_ref_",
                                                          {"link_tail_1", "link_tail_2"},
                                                          std::move(requestAPI),
                                                          std::move(uidProvider)};
        UNIT_ASSERT(!continuation.ApplyIfNotFinished());
    }

    Y_UNIT_TEST(BadRedirect) {
        NTestingHelpers::TRequestJson request;
        auto ctx = MakeContext(NSc::TValue(request));
        auto firstMultiRequest = MakeIntrusive<StrictMock<TMockMultiRequest>>();
        auto secondMultiRequest = MakeIntrusive<StrictMock<TMockMultiRequest>>();

        TRequestOptions requestOptions;

        THttpHeaders redirHeadersNoLocation;
        redirHeadersNoLocation.AddHeader(TString{NAlice::NNetwork::HEADER_SET_COOKIE}, "yandexuid=1111; more_data");

        auto returnHttpRedirectResponse = [](THttpHeaders&& headers) {
            return [&headers]() {
                auto request = MakeHolder<TFakeRequest>(TString{}, HTTP_FOUND, THttpHeaders{headers});
                return request;
            };
        };
        EXPECT_CALL(*firstMultiRequest, AddRequest(ParseUri("hit_counter_ref"), _))
            .WillOnce(Invoke(returnHttpRedirectResponse(std::move(redirHeadersNoLocation))));

        auto uidProvider = std::make_unique<TMockUidProvider>();
        EXPECT_CALL(*uidProvider, GetUid(_)).WillOnce(Return(Nothing()));

        auto requestAPI = std::make_unique<TMockRequestAPI>();
        InSequence sequence;
        EXPECT_CALL(*requestAPI, WeakMultiRequest(_)).WillOnce(Return(firstMultiRequest));
        EXPECT_CALL(*requestAPI, WeakMultiRequest(_)).WillOnce(Return(secondMultiRequest));

        TDirectGalleryHitConfirmContinuation continuation{ctx,
                                                          "hit_counter_ref",
                                                          "link_head_ref_",
                                                          /* linkTails= */ {},
                                                          std::move(requestAPI),
                                                          std::move(uidProvider)};
        UNIT_ASSERT(!continuation.ApplyIfNotFinished()); // For the user, it is still a success.
    }
}

} // namespace
