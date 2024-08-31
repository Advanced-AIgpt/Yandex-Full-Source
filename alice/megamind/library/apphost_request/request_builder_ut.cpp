#include "request_builder.h"
#include "util.h"

#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/util/status.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE(AppHostRequestBuilder) {
    Y_UNIT_TEST_F(CreateAndPushRequestSmoke, TAppHostFixture) {
        const TStringBuf itemName{"test_item_http_request"};

        auto ahCtx = CreateAppHostContext();
        TAppHostHttpProxyMegamindRequestBuilder builder;

        auto origProto = builder.CreateAndPushRequest(ahCtx, itemName);

        NAppHostHttp::THttpRequest proto;
        UNIT_ASSERT(ahCtx.TestCtx().GetOnlyProtobufItem(itemName).Fill(&proto));
        UNIT_ASSERT_VALUES_EQUAL(origProto.ShortUtf8DebugString(), proto.ShortUtf8DebugString());
    }
}

} // namespace
