#pragma once

#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/processor.h>
#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/service.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <voicetech/library/proto_api/yabio.pb.h>
#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/gtest_protobuf/matcher.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAlice::NCuttlefish::NAppHostServices::Tests::Util {

    using namespace NAlice::NCuttlefish;
    using namespace NAlice::NCuttlefish::NAppHostServices;

    using ::testing::_;
    using ::testing::Return;

    class MockBioContextSyncProcessor {
    public:
        MOCK_METHOD(TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>>, Process, (const NAlice::TEnrollmentHeaders&, const YabioProtobuf::YabioContext&), (const));
    };

    struct BioContextSyncServiceContext {
        MOCK_METHOD(bool, HasProtobufItem, (const TStringBuf), (const));
        MOCK_METHOD(const NAlice::TEnrollmentHeaders&, GetEnrollmentHeaders, (TStringBuf), (const));
        MOCK_METHOD(const YabioProtobuf::YabioContext&, GetYabioContext, (TStringBuf), (const));
        MOCK_METHOD(void, AddEnrollmentUpdateDirective, (const NAliceProtocol::TEnrollmentUpdateDirective&, TStringBuf), (const));

        // gmock is not able to substitute template methods. So using a hack with partial template specialisation:
        // https://stackoverflow.com/questions/3426067/how-to-mock-templated-methods-using-google-mock
        template <typename T>
        T GetOnlyProtobufItem(TStringBuf type) const;

        template <>
        NAlice::TEnrollmentHeaders GetOnlyProtobufItem<NAlice::TEnrollmentHeaders>(TStringBuf type) const {
            return GetEnrollmentHeaders(type);
        }

        template <>
        YabioProtobuf::YabioContext GetOnlyProtobufItem<YabioProtobuf::YabioContext>(TStringBuf type) const {
            return GetYabioContext(type);
        }

        template <typename T>
        void AddProtobufItem(const T& proto, TStringBuf type) const;

        template <>
        void AddProtobufItem<NAliceProtocol::TEnrollmentUpdateDirective>(const NAliceProtocol::TEnrollmentUpdateDirective& proto, TStringBuf type) const {
            AddEnrollmentUpdateDirective(proto, type);
        }
    };

    class BioContextSyncServiceTest : public ::testing::Test {
    protected:
        void SetUp() override {
            YabioContext.set_group_id("123");

            testing::DefaultValue<const NAlice::TEnrollmentHeaders&>::Set(EnrollmentHeaders);
            testing::DefaultValue<const YabioProtobuf::YabioContext&>::Set(YabioContext);

            testing::DefaultValue<TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>>>::SetFactory(&MakeEnrollmentUpdateDirectives);
        }

        void TearDown() override {
            testing::DefaultValue<const NAlice::TEnrollmentHeaders&>::Clear();
            testing::DefaultValue<const YabioProtobuf::YabioContext&>::Clear();
            testing::DefaultValue<TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>>>::Clear();
        }

    public:
        static TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> MakeEnrollmentUpdateDirectives() {
            TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> directives(3);

            directives[0] = MakeHolder<NAliceProtocol::TEnrollmentUpdateDirective>();
            directives[1] = MakeHolder<NAliceProtocol::TEnrollmentUpdateDirective>();
            directives[2] = MakeHolder<NAliceProtocol::TEnrollmentUpdateDirective>();

            directives[0]->MutableHeader()->SetPersonId("PersId-0");
            directives[1]->MutableHeader()->SetPersonId("PersId-1");
            directives[2]->MutableHeader()->SetPersonId("PersId-2");

            return directives;
        }

        void RunTest() {
            TSourceMetrics metrics("test_metrics");
            BioContextSyncInternal(MockContext, &MockProcessor, TLogContext(new TSelfFlushLogFrame(nullptr), nullptr),  metrics);
        }

    public:
        NAlice::TEnrollmentHeaders EnrollmentHeaders;
        YabioProtobuf::YabioContext YabioContext;

        ::testing::NiceMock<BioContextSyncServiceContext> MockContext;
        MockBioContextSyncProcessor MockProcessor;
    };

}
