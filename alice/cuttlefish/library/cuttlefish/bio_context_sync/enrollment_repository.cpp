#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/enrollment_repository.h>
#include <alice/library/proto/protobuf.h>
#include <voicetech/bio/ondevice/lib/proto_convertor/convert.h>
#include <util/string/cast.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    bool TEnrollmentRepository::TryLoad(const YabioProtobuf::YabioContext& yabioContext) {
        IsEnrollmentLoaded = NBio::NDevice::ExtractEnrollmentFromYabioContext(
            yabioContext,
            &Enrollment,
            &ActualVersion);

        return IsEnrollmentLoaded;
    }

    THolder<NAliceProtocol::TEnrollmentUpdateDirective> TEnrollmentRepository::CheckUpdateFor(NAlice::TEnrollmentHeader enrollmentHeader) const {
        Y_ASSERT(IsEnrollmentLoaded);

        if (IsActual(enrollmentHeader.GetVersion())) {
            return nullptr;
        }

        auto enrollmentUpdateDirectiveHolder = MakeHolder<NAliceProtocol::TEnrollmentUpdateDirective>();
        enrollmentUpdateDirectiveHolder->MutableHeader()->Swap(&enrollmentHeader);
        enrollmentUpdateDirectiveHolder->MutableHeader()->SetVersion(ActualVersion);
        enrollmentUpdateDirectiveHolder->SetEnrollment(NAlice::ProtoToBase64String(Enrollment));

        return enrollmentUpdateDirectiveHolder;
    }

    bool TEnrollmentRepository::IsActual(const TString& version) const {
        return version == ActualVersion;
    }
}
