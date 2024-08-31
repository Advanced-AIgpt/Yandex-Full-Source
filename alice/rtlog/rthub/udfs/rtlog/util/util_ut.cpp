#include "util.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

    Y_UNIT_TEST_SUITE(NeedToSendTests) {
        Y_UNIT_TEST(YexceptionTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::NeedToSend(
                    "hel:lo (yexception) some_file.kek:flexline_number924: Some : Increadible :Error Text:: YOU ARE AMAZING",
                    {}
                ),
                true
            );
        }
        Y_UNIT_TEST(ScenarioErrorTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::NeedToSend(
                    "Scenario bla bla bla error: some_file.kek:flexline_number924: Some : Increadible :Error Text:: YOU ARE AMAZING",
                    {}
                ),
                true
            );
        }
        Y_UNIT_TEST(ScenarioErrorNotErrorTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::NeedToSend(
                    "Scenario bla bla bla some_file.kek:flexline_number924: Some : Increadible :Error Text:: YOU ARE AMAZING",
                    {}
                ),
                false
            );
        }
        Y_UNIT_TEST(ScenarioErrorNotScenarioTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::NeedToSend(
                    "bla bla bla error: some_file.kek:flexline_number924: Some : Scenario Increadible :Error Text:: YOU ARE AMAZING",
                    {}
                ),
                false
            );
        }
        Y_UNIT_TEST(TestAllServicesWithBacktrace) {
            UNIT_ASSERT_EQUAL(
                NAlice::NeedToSend(
                    "bla bla bla error: some_file.kek:flexline_number924: Some : Increadible :Error Text:: YOU ARE AMAZING",
                    "some backtrace"
                ),
                true
            );
        }
    }

    Y_UNIT_TEST_SUITE(GetEnvTests) {
        Y_UNIT_TEST(ProductionServicesTest) {
            for (const auto& nannyServiceId : NAlice::PRODUCTION_NANNY_SERVICES) {
                UNIT_ASSERT_EQUAL(
                    NAlice::GetEnv(TString(nannyServiceId)),
                    TString(NAlice::PRODUCTION)
                );
            }
        }
        Y_UNIT_TEST(HamsterServicesTest) {
            for (const auto& nannyServiceId : NAlice::HAMSTER_NANNY_SERVICES) {
                UNIT_ASSERT_EQUAL(
                    NAlice::GetEnv(TString(nannyServiceId)),
                    TString(NAlice::HAMSTER)
                );
            }
        }
        Y_UNIT_TEST(NannyServiceWhichWillNeverExistTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::GetEnv("nanny_service_id_which_will_never_exist_924!?*;^,"),
                Nothing()
            );
        }
    }

    Y_UNIT_TEST_SUITE(FromPosTests) {
        Y_UNIT_TEST(FileFromPosTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::FileFromPos("alice/hollywood/library/dispatcher/common_handles/unpack_http_handles/unpack_http.h:174"),
                "alice/hollywood/library/dispatcher/common_handles/unpack_http_handles/unpack_http.h"
            );
        }
        Y_UNIT_TEST(LineFromPosTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::LineFromPos("alice/hollywood/library/dispatcher/common_handles/unpack_http_handles/unpack_http.h:174"),
                174
            );
        }
    }

    Y_UNIT_TEST_SUITE(DataCenterFromHostTests) {
        Y_UNIT_TEST(DataCenterFromHostTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::DataCenterFromHost("hollywood-sas-1.sas.yp-c.yandex.net"),
                "sas"
            );
        }
        Y_UNIT_TEST(LegacyDataCenterFromHostTest) {
            UNIT_ASSERT_EQUAL(
                NAlice::DataCenterFromHost("sas3-5349.search.yandex.net"),
                "sas"
            );
        }
    }

    Y_UNIT_TEST_SUITE(GetLanguageFromBacktrace) {
        Y_UNIT_TEST(CppFromBacktrace) {
            UNIT_ASSERT_VALUES_EQUAL(NAlice::GetLanguage(
                "\t#0 0xcb7d09c in grpc_core::(anonymous namespace)::PickFirst::PickFirstSubchannelData::ProcessUnselectedReadyLocked() /home/g-kostin/arcadia/contrib/libs/cxxsupp/libcxx/include/memory:2621\n"
                "\t#1 0xcb7d09c in std::__y1::unique_ptr<grpc_core::LoadBalancingPolicy::SubchannelPicker, std::__y1::default_delete<grpc_core::LoadBalancingPolicy::SubchannelPicker> >::~unique_ptr() /home/g-kostin/arcadia/contrib/libs/cxxsupp/libcxx/include/memory:2577\n"
                "\t#2 0xcb7d09c in grpc_core::(anonymous namespace)::PickFirst::PickFirstSubchannelData::ProcessUnselectedReadyLocked() /home/g-kostin/arcadia/contrib/libs/grpc/src/core/ext/filters/client_channel/lb_policy/pick_first/pick_first.cc:447\n"
                "\t#3 0xc74c274 in NYP::NServiceDiscovery::NApi::TPod::InternalSerializeWithCachedSizesToArray(bool, unsigned char*) const /home/g-kostin/arcadia/contrib/libs/protobuf/src/google/protobuf/io/coded_stream.h:1169\n"
                "\t#4 0xc74c274 in ?? ??:0\n"
                "\t#5 0xb17a4e1 in NYP::NServiceDiscovery::NApi::TPod::InternalSerializeWithCachedSizesToArray(bool, unsigned char*) const /-B/infra/yp_service_discovery/api/api.pb.cc:6479\n"
                "\t#6 0xb17ac8a in BrotliSplitBlock block_splitter.c:?\n"
                "\t#7 0xb17ac8a in InitialEntropyCodesCommand /home/g-kostin/arcadia/contrib/libs/brotli/enc/./block_splitter_inc.h:28\n"
                "\t#8 0xb17ac8a in SplitByteVectorCommand /home/g-kostin/arcadia/contrib/libs/brotli/enc/./block_splitter_inc.h:391\n"
                "\t#9 0xb17ac8a in BrotliSplitBlock /home/g-kostin/arcadia/contrib/libs/brotli/enc/block_splitter.c:158\n"
                "\t#10 0xa6e16b7 in HistogramAddVectorCommand /home/g-kostin/arcadia/contrib/libs/brotli/enc/./histogram_inc.h:39\n"
                "\t#11 0xa6e16b7 in RandomSampleCommand /home/g-kostin/arcadia/contrib/libs/brotli/enc/./block_splitter_inc.h:43\n"
                "\t#12 0xa6e16b7 in RefineEntropyCodesCommand /home/g-kostin/arcadia/contrib/libs/brotli/enc/./block_splitter_inc.h:58\n"
                "\t#13 0xa6e16b7 in SplitByteVectorCommand /home/g-kostin/arcadia/contrib/libs/brotli/enc/./block_splitter_inc.h:394\n"
                "\t#14 0xa6e16b7 in BrotliSplitBlock /home/g-kostin/arcadia/contrib/libs/brotli/enc/block_splitter.c:158\n"
                "\t#15 0x7f1cc7cce6da in start_thread /build/glibc-2ORdQG/glibc-2.27/nptl/pthread_create.c:463\n"
                "\t#16 0x7f1cc79f7a3e in clone /build/glibc-2ORdQG/glibc-2.27/misc/../sysdeps/unix/sysv/linux/x86_64/clone.S:95"
            ), "c++");
        }
        Y_UNIT_TEST(PythonFromBacktrace) {
            UNIT_ASSERT_VALUES_EQUAL(NAlice::GetLanguage(
                "Traceback (most recent call last):\n"
                "  File \"alice/vins/apps/personal_assistant/personal_assistant/app.py\", line 1691, in _make_cards\n"
                "    req_info=req_info, schema=self._cards_schema)\n"
                "  File \"alice/vins/sdk/vins_sdk/app.py\", line 116, in render_card\n"
                "    return self._dm.nlg.render_card(card_id=card_id, schema=schema, form=form, context=context, req_info=req_info)\n"
                "  File \"alice/vins/core/vins_core/nlg/template_nlg.py\", line 373, in render_card\n"
                "    return template.render_card(card_id, schema=schema, **render_context)\n"
                "  File \"alice/vins/core/vins_core/nlg/template_nlg.py\", line 296, in render_card\n"
                "    json_obj = json.loads(rendered)\n"
                "  File \"contrib/tools/python/src/Lib/json/__init__.py\", line 339, in loads\n"
                "  File \"contrib/tools/python/src/Lib/json/decoder.py\", line 364, in decode\n"
                "  File \"contrib/tools/python/src/Lib/json/decoder.py\", line 380, in raw_decode\n"
                "ValueError: Invalid control character at: line 1 column 461 (char 460)"
            ), "python");
        }
        Y_UNIT_TEST(JavaFromBacktrace) {
            UNIT_ASSERT_VALUES_EQUAL(NAlice::GetLanguage(
                "java.util.concurrent.CompletionException: java.util.concurrent.TimeoutException\n"
                "\tat java.base/java.util.concurrent.CompletableFuture.reportJoin(CompletableFuture.java:412)\n"
                "\tat java.base/java.util.concurrent.CompletableFuture.join(CompletableFuture.java:2044)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.ydb.YdbClient.execute(YdbClient.kt:82)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.service.state.SkillStateDaoImpl$storeSessionAndUserAndApplicationState$2.get(SkillStateDaoImpl.kt:334)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.service.state.SkillStateDaoImpl$storeSessionAndUserAndApplicationState$2.get(SkillStateDaoImpl.kt:178)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.solomon.Instrument.time(Instrument.java:37)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.service.state.SkillStateDaoImpl.storeSessionAndUserAndApplicationState(SkillStateDaoImpl.kt:333)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessorImpl.updateStates(SkillRequestProcessorImpl.java:312)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessorImpl.process(SkillRequestProcessorImpl.java:249)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.processor.SkillRequestProcessorImpl.process(SkillRequestProcessorImpl.java:171)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier.processApply(MegaMindRequestSkillApplier.java:315)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.MegaMindRequestSkillApplier.processApply(MegaMindRequestSkillApplier.java:182)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.ProcessSkillRunProcessor.apply(ProcessSkillRunProcessor.java:80)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.scenarios.dialogovo.processor.ProcessSkillRunProcessor.apply(ProcessSkillRunProcessor.java:27)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl$processApply$1.invoke(MegaMindServiceImpl.kt:341)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl$processApply$1.invoke(MegaMindServiceImpl.kt:35)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl$applyMonitored$1.get(MegaMindServiceImpl.kt:349)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl$applyMonitored$1.get(MegaMindServiceImpl.kt:35)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.solomon.RequestSensors.monitored(RequestSensors.java:63)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl.applyMonitored(MegaMindServiceImpl.kt:347)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl.processApply(MegaMindServiceImpl.kt:340)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.megamind.MegaMindServiceImpl.apply(MegaMindServiceImpl.kt:198)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.controller.MegamindController.applyImpl(MegamindController.java:246)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.controller.MegamindController.dialogovoApply(MegamindController.java:214)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.controller.MegamindController.withLogging(MegamindController.java:259)\n"
                "\tat ru.yandex.alice.paskill.dialogovo.controller.MegamindController.apply(MegamindController.java:90)\n"
                "\tat jdk.internal.reflect.GeneratedMethodAccessor160.invoke(Unknown Source)\n"
                "\tat java.base/jdk.internal.reflect.DelegatingMethodAccessorImpl.invoke(DelegatingMethodAccessorImpl.java:43)\n"
                "\tat java.base/java.lang.reflect.Method.invoke(Method.java:566)\n"
                "\tat org.springframework.web.method.support.InvocableHandlerMethod.doInvoke(InvocableHandlerMethod.java:190)\n"
                "\tat org.springframework.web.method.support.InvocableHandlerMethod.invokeForRequest(InvocableHandlerMethod.java:138)\n"
            ), "java");
        }
        Y_UNIT_TEST(GoFromBacktrace) {
            UNIT_ASSERT_VALUES_EQUAL(NAlice::GetLanguage(
                "runtime error: invalid memory address or nil pointer dereference goroutine 117813199 [running]:\n"
                "runtime/debug.Stack(0xc003aaf098, 0x3f0e40, 0x99b0890)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/contrib/go/_std/src/runtime/debug/stack.go:24 +0x9f\n"
                "a.yandex-team.ru/alice/library/go/middleware.Recoverer.func1.1.1(0x7f4760, 0xc000199720, 0xc0023d9b00, 0x7da260, 0xc00eb400e0)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/library/go/middleware/recoverer.go:20 +0x7b\n"
                "panic(0x3f0e40, 0x99b0890)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/contrib/go/_std/src/runtime/panic.go:969 +0x1b9\n"
                "a.yandex-team.ru/alice/library/go/metrics.RouteMetricsTracker.func1.1.1(0xc0005f4450, 0xc0023d9b00)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/library/go/metrics/middleware.go:26 +0x9b\n"
                "panic(0x3f0e40, 0x99b0890)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/contrib/go/_std/src/runtime/panic.go:969 +0x1b9\n"
                "a.yandex-team.ru/alice/library/go/megamind.SmartHomeInfo.GetRoomNameBySpeakerQuasarID(0xc008f95200, 0xc010258300, 0x18, 0xc0081814c0, 0x1)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/library/go/megamind/smart_home.go:13 +0x2a\n"
                "a.yandex-team.ru/alice/iot/vulpix/controller/megamind.(*StartRunProcessor).Run(0xc0002d4f40, 0x7e4520, 0xc008f95380, 0x4b58c231, 0xc00808f780, 0x0, 0x0, 0xc0089e69d8)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/iot/vulpix/controller/megamind/start_processors.go:62 +0x2ec\n"
                "a.yandex-team.ru/alice/iot/vulpix/controller/megamind.(*RunProcessorWithMetrics).Run(0xc000289ae0, 0x7e4520, 0xc008f95380, 0x4b58c231, 0xc00808f780, 0x0, 0x0, 0x0)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/iot/vulpix/controller/megamind/metrics.go:76 +0xf6\n"
                "a.yandex-team.ru/alice/iot/vulpix/controller/megamind.(*Controller).Run(0xc0003db050, 0x7e4520, 0xc008f95380, 0xc00808f780, 0x4b58c231, 0x0, 0x0, 0xc00259a600, 0x1f8, 0x514ce0, ...)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/iot/vulpix/controller/megamind/controller.go:172 +0x9a9\n"
                "a.yandex-team.ru/alice/library/go/megamind.Dispatcher.Run(0xc00000f780, 0x2, 0x2, 0x7f4760, 0xc000199720, 0x5df849, 0x6, 0x5dcca7, 0x3, 0x9a82be0, ...)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/library/go/megamind/dispatcher.go:34 +0x362\n"
                "a.yandex-team.ru/alice/iot/bulbasaur/server.(*Server).mmRunHandler(0xc00027b400, 0x7f838c48f5f0, 0xc00f7a1d80, 0xc0023d9b00)\n"
                "\t/place/sandbox-data/tasks/3/5/846361753/__FUSE/mount_point_d365728d-dfd8-4a52-ab20-9bc370aac8c6/alice/iot/bulbasaur/server/megamind_handlers.go:65 +0xbe5\n"
                "net/http.HandlerFunc.ServeHTTP(0xc0002d5ea0, 0x7f838c48f5f0, 0xc00f7a1d80, 0xc0023d9b00)"
            ), "go");
        }
    }

} // namespace
