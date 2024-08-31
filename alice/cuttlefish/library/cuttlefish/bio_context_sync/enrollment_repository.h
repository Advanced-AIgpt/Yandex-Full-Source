#pragma once

#include <alice/cuttlefish/library/protos/bio_context_sync.pb.h>
#include <alice/megamind/protos/guest/enrollment_headers.pb.h>
#include <voicetech/library/proto_api/yabio.pb.h>
#include <util/generic/ptr.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    class IEnrollmentRepository : public TThrRefBase {
    public:
        virtual bool TryLoad(const YabioProtobuf::YabioContext& yabioContext) = 0;
        virtual THolder<NAliceProtocol::TEnrollmentUpdateDirective> CheckUpdateFor(NAlice::TEnrollmentHeader enrollmentHeader) const = 0;

        virtual ~IEnrollmentRepository() = default;
    };

    class TEnrollmentRepository : public IEnrollmentRepository {
    public:
        bool TryLoad(const YabioProtobuf::YabioContext& yabioContext) override;
        THolder<NAliceProtocol::TEnrollmentUpdateDirective> CheckUpdateFor(NAlice::TEnrollmentHeader enrollmentHeader) const override;

    private:
        bool IsActual(const TString& version) const;

    private:
        mutable bool IsEnrollmentLoaded = false;
        YabioOndeviceProtobuf::TEnrollment Enrollment;
        TString ActualVersion;
    };
}
