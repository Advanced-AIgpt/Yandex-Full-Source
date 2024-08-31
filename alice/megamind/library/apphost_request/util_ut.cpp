#include "util.h"

#include <alice/megamind/library/apphost_request/protos/uniproxy_request.pb.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/json/json.cpp>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

class TTestContext : public NAppHost::NService::TTestContext {
public:
    using NAppHost::NService::TTestContext::TTestContext;

    MOCK_METHOD(void, AddLogLine, (TStringBuf), (override));
};

Y_UNIT_TEST_SUITE_F(AppHostUtil, TAppHostFixture) {
    // TODO (petrk) Enable and improve this test when guys from apphost add ability send streaming item to test apphost.
    // Until it is impossible to test TAppHostItemStreamer :(
    /*
    Y_UNIT_TEST(ItemStreamer) {
        struct TTestItem : public TAppHostItemStreamer::IItem {
            TTestItem(TAppHostItemStreamer& streamer, TString name)
                : TAppHostItemStreamer::IItem{streamer}
                , Name{name}
            {
            }

            bool CheckAndApply() override {
                return false;
            }

            TString Name;
        };

        {
            auto ahCtx = CreateAppHostContext();
            auto& ctx = ahCtx.TestCtx();

            NAlice::NMegamindAppHost::TErrorProto proto;
            proto.SetMessage("hello");
            ctx.AddProtobufItem(proto, "item1");

            TAppHostItemStreamer streamer{ctx};
            TTestItem item{streamer, "item1"};

            streamer.NextInput(item);
        }

    }
    */

    Y_UNIT_TEST(GetFirstProtoItem) {
        auto ahCtx = CreateAppHostContext();
        auto& ctx = ahCtx.TestCtx();
        NMegamindAppHost::TUniproxyRequestInfoProto srcProto;
        srcProto.SetUri("first");
        ctx.AddProtobufItem(srcProto, "test1", NAppHost::EContextItemKind::Input);
        srcProto.SetUri("second");
        ctx.AddProtobufItem(srcProto, "test1", NAppHost::EContextItemKind::Input);
        NMegamindAppHost::TUniproxyRequestInfoProto checkProto;
        const auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), "test1", checkProto);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(checkProto.GetUri(), "first");
    }

    Y_UNIT_TEST(ErrorProtoConversion) {
        TVector<TError> errors = {
            TError{TError::EType::Logic} << "test",
            TError{TError::EType::Logic},
            TError{} << "test",
            TError{},
            TError{HttpCodes::HTTP_BAD_REQUEST} << "test",
            TError{HttpCodes::HTTP_BAD_REQUEST},
            [] {
                TError error;
                error.Type = TError::EType::Begemot;
                error.ErrorMsg << "some error";
                error.HttpCode = HttpCodes::HTTP_ACCEPTED;
                return error;
            }()
        };
        for (const auto& expected : errors) {
            auto actual = ErrorFromProto(ErrorToProto(expected));
            UNIT_ASSERT_VALUES_EQUAL(expected.Type, actual.Type);
            UNIT_ASSERT_VALUES_EQUAL(expected.HttpCode, actual.HttpCode);
            UNIT_ASSERT_VALUES_EQUAL(ToString(expected.ErrorMsg), ToString(actual.ErrorMsg));
        }
    }

    Y_UNIT_TEST(LogSkrInfo) {
        TTestContext ctx;
        TMockGlobalContext GlobalCtx;
        TItemProxyAdapter itemProxyAdapter{ctx, TRTLogger::NullLogger(), GlobalCtx, false};

        TString logLine;
        EXPECT_CALL(ctx, AddLogLine(_)).WillRepeatedly([&logLine](const auto& value) { logLine = value; });

        LogSkrInfo(itemProxyAdapter, "", "", 0);
        UNIT_ASSERT(logLine.empty());

        LogSkrInfo(itemProxyAdapter, "uuid", "message_id", 123);
        UNIT_ASSERT_VALUES_EQUAL(logLine, R"(skr_info{"hypothesis_number":123,"message_id":"message_id","uuid":"uuid"})");

        LogSkrInfo(itemProxyAdapter, "", "message_id", 456);
        UNIT_ASSERT_VALUES_EQUAL(logLine, R"(skr_info{"hypothesis_number":456,"message_id":"message_id","uuid":""})");

        LogSkrInfo(itemProxyAdapter, "uuid", "", 789);
        UNIT_ASSERT_VALUES_EQUAL(logLine, R"(skr_info{"hypothesis_number":789,"message_id":"","uuid":"uuid"})");
    }
}

} // namespace
