#include <alice/hollywood/library/scenarios/hardcoded_response/proto/hardcoded_response.pb.h>

#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>

namespace NAlice::NHollywood {

Y_UNIT_TEST_SUITE(HardcodedResponse) {
    Y_UNIT_TEST(SanityCheck) {
        THardcodedResponseFastDataProto proto;

        auto file = TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/shards/common/prod/fast_data/hardcoded_response/hardcoded_response.pb.txt";

        Y_ENSURE(TryParseFromTextFormat(file, proto), "Failed to parse text proto");

        UNIT_ASSERT(proto.GetResponses().size() > 5);

        for (const auto& response : proto.GetResponses()) {
            UNIT_ASSERT(response.GetName().size() > 5);
            UNIT_ASSERT(!response.GetRegexps().empty());
            UNIT_ASSERT(!response.GetResponses().empty());
            for (const auto& resp : response.GetResponses()) {
                UNIT_ASSERT(!resp.GetText().empty());
            }
            for (const auto& link : response.GetLinks()) {
                UNIT_ASSERT(!link.GetTitle().empty());
                UNIT_ASSERT(!link.GetUrl().empty());
            }
        }
    }
    Y_UNIT_TEST(SanityCheckGranetPipeline) {
        THardcodedResponseFastDataProto proto;

        auto file = TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/shards/common/prod/fast_data/hardcoded_response/hardcoded_response.pb.txt";

        Y_ENSURE(TryParseFromTextFormat(file, proto), "Failed to parse text proto");

        UNIT_ASSERT(proto.GetGranetResponses().size() > 5);
        const auto checkResponses = [](const google::protobuf::RepeatedPtrField<THardcodedResponseFastDataProto::TResponse >& responses) -> void {
            UNIT_ASSERT(!responses.empty());
            for (const auto& resp : responses) {
                UNIT_ASSERT(!resp.GetText().empty());
            }
        };
        for (const auto& response : proto.GetGranetResponses()) {
            UNIT_ASSERT(response.GetName().size() > 5);
            UNIT_ASSERT(response.GetIntent().size() > 5);
            checkResponses(response.GetResponses());
            if (response.HasLink()) {
                const auto& link = response.GetLink();
                UNIT_ASSERT(!link.GetTitle().empty());
                UNIT_ASSERT(!link.GetUrl().empty());
            }
            if (response.HasFallbackResponse()) {
                const auto& fallbackResponse = response.GetFallbackResponse();
                checkResponses(fallbackResponse.GetResponses());
                if (fallbackResponse.HasPushDirective()) {
                    const auto& directive = fallbackResponse.GetPushDirective();
                    UNIT_ASSERT(!directive.GetText().empty());
                    UNIT_ASSERT(!directive.GetUrl().empty());
                    UNIT_ASSERT(!directive.GetTitle().empty());
                }
            }
            if (response.HasApplicabilityInfo()) {
                const auto& applicability = response.GetApplicabilityInfo();
                if (applicability.HasExperiment()) {
                    UNIT_ASSERT(applicability.GetExperiment().size() > 5);
                }
                if (applicability.HasAppIdRegexp()) {
                    UNIT_ASSERT(applicability.GetAppIdRegexp().size() > 5);
                }
                if (applicability.HasSupportedFeature()) {
                    UNIT_ASSERT(applicability.GetSupportedFeature().size() > 5);
                }
            }
        }
    }
}

} // namespace NAlice::NHollywood
