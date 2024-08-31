#pragma once

#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/enrollment_repository.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/bio_context_sync.pb.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <alice/library/proto/protobuf.h>
#include <voicetech/library/messages/build.h>
#include <voicetech/library/proto_api/yabio.pb.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    // Client may want to check if new version of enrollments appear on server.
    // If yes, the result of OnCheckEnrollmentUpdate method execution will be TEnrollmentUpdateDirective sent.
    class TBioContextSyncProcessor {
    public:
        TBioContextSyncProcessor(TLogContext logContext, TSourceMetrics& metrics);
        TBioContextSyncProcessor(TLogContext logContext, TSourceMetrics& metrics, TIntrusivePtr<IEnrollmentRepository> enrollmentRepository);

        TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> Process(
            const NAlice::TEnrollmentHeaders& enrollmentHeaders,
            const YabioProtobuf::YabioContext& yabioContext
        );

    private:
        THolder<NAliceProtocol::TEnrollmentUpdateDirective> CheckEnrollmentUpdate(const NAlice::TEnrollmentHeader& enrollmentHeader) const;

    private:
        TLogContext LogContext;
        TSourceMetrics& Metrics;
        TIntrusivePtr<IEnrollmentRepository> EnrollmentRepository;
    };

}
