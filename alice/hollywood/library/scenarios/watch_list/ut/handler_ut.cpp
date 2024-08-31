#include <alice/hollywood/library/scenarios/watch_list/setup_add.h>
#include <alice/hollywood/library/scenarios/watch_list/setup_delete.h>
#include <alice/hollywood/library/scenarios/watch_list/process.h>

#include <alice/hollywood/library/framework/unittest/test_environment.h>
#include <alice/hollywood/library/testing/mock_global_context.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/protos/data/tv/watch_list/wl_answer.pb.h>
#include <alice/protos/data/tv/watch_list/wl_requests.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/output.h>
#include <contrib/libs/googleapis-common-protos/google/rpc/status.pb.h>
#include <contrib/libs/googleapis-common-protos/google/rpc/error_details.pb.h>

namespace NAlice::NHollywood::NWatchList {

    namespace {
#define PREPARE_CONTEXT                                                    \
    NAppHost::NService::TTestContext serviceCtx;                           \
    NScenarios::TRequestMeta meta;                                         \
    meta.SetUserTicket("some_ticket");                                     \
    TMockGlobalContext globalCtx;                                          \
    TContext ctx_{globalCtx, TRTLogger::StderrLogger(), nullptr, nullptr}; \
    NJson::TJsonValue appHostParams;                                       \
    TFakeRng rng;                                                          \
    TScenarioHandleContext ctx{serviceCtx, meta, ctx_, rng, ELanguage::LANG_RUS, ELanguage::LANG_RUS, appHostParams, nullptr};
    }

    Y_UNIT_TEST_SUITE(SetupHandleTests) {
        Y_UNIT_TEST(BasicSetupAdd) {
            PREPARE_CONTEXT

            NAlice::NTv::TTvWatchListAddItemRequest add;
            add.SetUuid("dead-beaf");
            google::protobuf::Any wrapped;
            wrapped.PackFrom(add);
            ctx.ServiceCtx.AddProtobufItem(wrapped, "request");

            TTvWatchListAddSetupHandle setupHandler;
            setupHandler.Do(ctx);

            NAppHostHttp::THttpRequest reqProto;
            ctx.ServiceCtx.GetOnlyProtobufItem("wl_http_request").Fill(&reqProto);

            UNIT_ASSERT_EQUAL(reqProto.GetMethod(), NAppHostHttp::THttpRequest_EMethod_Post);
            UNIT_ASSERT(reqProto.GetPath().EndsWith("/films-to-watch/dead-beaf"));
        }

        Y_UNIT_TEST(BasicSetupDelete) {
            PREPARE_CONTEXT

            NAlice::NTv::TTvWatchListDeleteItemRequest del;
            del.SetUuid("foo-bar");
            google::protobuf::Any wrapped;
            wrapped.PackFrom(del);
            ctx.ServiceCtx.AddProtobufItem(wrapped, "request");

            TTvWatchListDeleteSetupHandle setupHandler;
            setupHandler.Do(ctx);

            NAppHostHttp::THttpRequest reqProto;
            ctx.ServiceCtx.GetOnlyProtobufItem("wl_http_request").Fill(&reqProto);

            UNIT_ASSERT_EQUAL(reqProto.GetMethod(), NAppHostHttp::THttpRequest_EMethod_Delete);
            UNIT_ASSERT(reqProto.GetPath().EndsWith("/films-to-watch/foo-bar"));
        }

        Y_UNIT_TEST(SetupWithoutUserTicket) {
            PREPARE_CONTEXT
            meta.ClearUserTicket();

            NAlice::NTv::TTvWatchListAddItemRequest add;
            add.SetUuid("dead-beaf");
            google::protobuf::Any wrapped;
            wrapped.PackFrom(add);
            ctx.ServiceCtx.AddProtobufItem(wrapped, "request");

            TTvWatchListAddSetupHandle setupHandler;
            setupHandler.Do(ctx);

            const auto maybeRequest = GetMaybeOnlyProto<NAppHostHttp::THttpRequest>(ctx.ServiceCtx, "wl_http_request");
            UNIT_ASSERT_EQUAL(maybeRequest, Nothing());

            const auto maybeError = GetMaybeOnlyProto<google::rpc::Status>(ctx.ServiceCtx, "error");
            UNIT_ASSERT(maybeError.Defined());
            UNIT_ASSERT_EQUAL(maybeError->Getcode(), grpc::UNAUTHENTICATED);
        }

    }

    Y_UNIT_TEST_SUITE(SetupProcessDoTests) {
        Y_UNIT_TEST(BasicProcess) {
            PREPARE_CONTEXT
            NAppHostHttp::THttpResponse response;
            response.SetStatusCode(200);
            ctx.ServiceCtx.AddProtobufItem(response, "http_response");

            TTvWatchListProcessHandle processHandle;
            processHandle.Do(ctx);

            google::protobuf::Any resultContainer = ctx.ServiceCtx.GetOnlyProtobufItem<google::protobuf::Any>("response");
            UNIT_ASSERT(resultContainer.Is<NAlice::NTv::TTvWatchListSwitchItemResultData>());
            NAlice::NTv::TTvWatchListSwitchItemResultData result;
            resultContainer.UnpackTo(&result);

            UNIT_ASSERT(result.HasSuccess());
            UNIT_ASSERT(!result.HasFailure());
        }

        Y_UNIT_TEST(Process500) {
            PREPARE_CONTEXT
            NAppHostHttp::THttpResponse response;
            response.SetStatusCode(500);
            ctx.ServiceCtx.AddProtobufItem(response, "http_response");

            TTvWatchListProcessHandle processHandle;
            processHandle.Do(ctx);

            google::protobuf::Any resultContainer = ctx.ServiceCtx.GetOnlyProtobufItem<google::protobuf::Any>("response");
            UNIT_ASSERT(resultContainer.Is<NAlice::NTv::TTvWatchListSwitchItemResultData>());
            NAlice::NTv::TTvWatchListSwitchItemResultData result;
            resultContainer.UnpackTo(&result);

            UNIT_ASSERT(result.HasFailure());
            UNIT_ASSERT_EQUAL(result.GetFailure().GetReason(), "OTT internal error");
        }

        Y_UNIT_TEST(Process401) {
            PREPARE_CONTEXT
            NAppHostHttp::THttpResponse response;
            response.SetStatusCode(401);
            ctx.ServiceCtx.AddProtobufItem(response, "http_response");

            TTvWatchListProcessHandle processHandle;
            processHandle.Do(ctx);

            google::protobuf::Any resultContainer = ctx.ServiceCtx.GetOnlyProtobufItem<google::protobuf::Any>("response");
            UNIT_ASSERT(resultContainer.Is<NAlice::NTv::TTvWatchListSwitchItemResultData>());
            NAlice::NTv::TTvWatchListSwitchItemResultData result;
            resultContainer.UnpackTo(&result);

            UNIT_ASSERT(result.HasFailure());
            UNIT_ASSERT_EQUAL(result.GetFailure().GetReason(), "Authorization failed");
        }

        Y_UNIT_TEST(ProcessNoAnswer) {
            PREPARE_CONTEXT

            TTvWatchListProcessHandle processHandle;
            processHandle.Do(ctx);

            UNIT_ASSERT_EQUAL(GetMaybeOnlyProto<google::protobuf::Any>(ctx.ServiceCtx, "response"), Nothing());

            const auto maybeError = GetMaybeOnlyProto<google::rpc::Status>(ctx.ServiceCtx, "error");
            UNIT_ASSERT(maybeError.Defined());
            UNIT_ASSERT_EQUAL(maybeError->Getcode(), grpc::UNAVAILABLE);
        }
    }
}
