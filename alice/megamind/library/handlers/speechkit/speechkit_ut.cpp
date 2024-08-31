#include "speechkit.h"

#include <alice/megamind/library/testing/mock_http_response.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

constexpr TStringBuf X_YANDEX_VINS_OK = "x-yandex-vins-ok";


Y_UNIT_TEST_SUITE(Speechkit) {
    Y_UNIT_TEST(AddHeader512RaiseError) {
        TMockHttpResponse httpResponse{true};
        httpResponse.SetHttpCode(HTTP_UNASSIGNED_512);
        NImpl::AddYandexVinsOkHeaderIfNeeded(httpResponse, /* raiseErrorOnFailedScenarios= */ true);
        auto* header = httpResponse.Headers().FindHeader(X_YANDEX_VINS_OK);
        UNIT_ASSERT(!header || header->Value() != "true");
    }

    Y_UNIT_TEST(AddHeader512DoNotRaiseError) {
        TMockHttpResponse httpResponse{true};
        httpResponse.SetHttpCode(HTTP_UNASSIGNED_512);
        NImpl::AddYandexVinsOkHeaderIfNeeded(httpResponse, /* raiseErrorOnFailedScenarios= */ false);
        const auto* header = httpResponse.Headers().FindHeader(X_YANDEX_VINS_OK);
        UNIT_ASSERT(header && header->Value() == "true");
    }

    Y_UNIT_TEST(AddHeader200RaiseError) {
        TMockHttpResponse httpResponse{true};
        httpResponse.SetHttpCode(HTTP_OK);
        NImpl::AddYandexVinsOkHeaderIfNeeded(httpResponse, /* raiseErrorOnFailedScenarios= */ true);
        auto* header = httpResponse.Headers().FindHeader(X_YANDEX_VINS_OK);
        UNIT_ASSERT(!header || header->Value() != "true");
    }

    Y_UNIT_TEST(AddHeader200DoNotRaiseError) {
        TMockHttpResponse httpResponse{true};
        httpResponse.SetHttpCode(HTTP_OK);
        NImpl::AddYandexVinsOkHeaderIfNeeded(httpResponse, /* raiseErrorOnFailedScenarios= */ false);
        auto* header = httpResponse.Headers().FindHeader(X_YANDEX_VINS_OK);
        UNIT_ASSERT(!header || header->Value() != "true");
    }
}

} // namespace
