#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/processor.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    TBioContextSyncProcessor::TBioContextSyncProcessor(TLogContext logContext, TSourceMetrics& metrics)
        : TBioContextSyncProcessor(logContext, metrics, MakeIntrusive<TEnrollmentRepository>())
    {
    }

    TBioContextSyncProcessor::TBioContextSyncProcessor(
        TLogContext logContext,
        TSourceMetrics &metrics,
        TIntrusivePtr<IEnrollmentRepository> enrollmentRepository
    )
        : LogContext(logContext)
        , Metrics(metrics)
        , EnrollmentRepository(enrollmentRepository)
    {
    }

    TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> TBioContextSyncProcessor::Process(
        const NAlice::TEnrollmentHeaders& enrollmentHeaders,
        const YabioProtobuf::YabioContext& yabioContext
    ) {
        if (enrollmentHeaders.HeadersSize() == 0) {
            LogContext.LogEventErrorCombo<NEvClass::RecvFromAppHostEmptyEnrollmentHeadersError>();
            Metrics.SetError("empty_enrollment_headers");
            return {};
        }

        try {
            if (!EnrollmentRepository->TryLoad(yabioContext)) {
                Metrics.SetError("bio_context_has_no_enrollments_for_user");
                return {};
            }
        } catch (...) {
            Metrics.SetError("load_biometry_context_error");
            LogContext.LogEventErrorCombo<NEvClass::LoadBiometryContextError>(CurrentExceptionMessage());
            return {};
        }

        TVector<THolder<NAliceProtocol::TEnrollmentUpdateDirective>> updateDirectives;

        for (const auto& enrollmentHeader : enrollmentHeaders.GetHeaders()) {
            Metrics.PushRate("enrollment", enrollmentHeader.GetVersion(), ToString(enrollmentHeader.GetUserType()));

            auto updateDirectiveHolder = CheckEnrollmentUpdate(enrollmentHeader);
            if (updateDirectiveHolder) {
                Metrics.PushRate("enrollment_update", updateDirectiveHolder->GetHeader().GetVersion(), ToString(updateDirectiveHolder->GetHeader().GetUserType()));
                updateDirectives.emplace_back(std::move(updateDirectiveHolder));
            }
        }

        return updateDirectives;
    }

    THolder<NAliceProtocol::TEnrollmentUpdateDirective> TBioContextSyncProcessor::CheckEnrollmentUpdate(
        const NAlice::TEnrollmentHeader& enrollmentHeader
    ) const {
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostEnrollmentHeader>(enrollmentHeader);

        if (enrollmentHeader.GetUserType() == NAlice::GUEST) {
            Metrics.SetError("unexpected_user_type");
            LogContext.LogEventInfoCombo<NEvClass::GuestUserBiometrySyncIsNotSupportedWarning>();
            return nullptr;
        }

        try {
            return EnrollmentRepository->CheckUpdateFor(enrollmentHeader);
        } catch (...) {
            LogContext.LogEventErrorCombo<NEvClass::CheckBiometryContextUpdateError>(TStringBuilder() << "check_enrollment_update failed: " << CurrentExceptionMessage());
            Metrics.SetError("check_enrollment_update_error");
        }

        return nullptr;
    }
}
