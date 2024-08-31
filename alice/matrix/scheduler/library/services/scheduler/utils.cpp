#include "utils.h"

#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <alice/protos/api/matrix/action.pb.h>
#include <alice/protos/api/matrix/technical_push.pb.h>
#include <alice/protos/api/matrix/user_device.pb.h>


#include <library/cpp/protobuf/interop/cast.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <util/generic/guid.h>
#include <util/string/cast.h>


namespace NMatrix::NScheduler {

namespace {

static constexpr TDuration MAX_ALLOWED_START_AT_CLOCK_DRIFT = TDuration::Minutes(10);

static constexpr TDuration MIN_ALLOWED_PERIODICALLY_POLICY_PERIOD = TDuration::Seconds(1);
static constexpr TDuration MIN_ALLOWED_RESTART_PERIOD = TDuration::Seconds(1);

TExpected<void, TString> PatchMeta(NMatrix::NApi::TScheduledAction& scheduledAction) {
    auto& meta = *scheduledAction.MutableMeta();

    if (!meta.GetGuid().empty()) {
        return TString::Join("Guid must be empty, but actual value is '", meta.GetGuid(), '\'');
    }

    meta.SetGuid(CreateGuidAsString());

    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<void, TString> PatchSpec(NMatrix::NApi::TScheduledAction& scheduledAction) {
    auto& spec = *scheduledAction.MutableSpec();

    switch (scheduledAction.GetSpec().GetSendPolicy().GetPolicyCase()) {
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendOncePolicy: {
            auto& sendOncePolicy = *spec.MutableSendPolicy()->MutableSendOncePolicy();

            if (!sendOncePolicy.HasRetryPolicy()) {
                auto& retryPolicy = *sendOncePolicy.MutableRetryPolicy();

                retryPolicy.MutableMinRestartPeriod()->CopyFrom(
                    NProtoInterop::CastToProto(MIN_ALLOWED_RESTART_PERIOD)
                );
                retryPolicy.MutableMaxRestartPeriod()->CopyFrom(
                    spec.GetSendPolicy().GetSendOncePolicy().GetRetryPolicy().GetMinRestartPeriod()
                );
            }
            break;
        }
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendPeriodicallyPolicy: {
            // Nothing to do
            break;
        }
        default: {
            // Nothing to do
            break;
        }
    }

    switch (spec.GetAction().GetActionTypeCase()) {
        case NMatrix::NApi::TAction::kMockAction: {
            // Nothing to do
            break;
        }
        case NMatrix::NApi::TAction::kSendTechnicalPush: {
            auto& sendTechnicalPush = *spec.MutableAction()->MutableSendTechnicalPush();

            // Autogeneration for technical push id
            if (sendTechnicalPush.GetTechnicalPush().GetTechnicalPushId().empty()) {
                sendTechnicalPush.MutableTechnicalPush()->SetTechnicalPushId(CreateGuidAsString());
            }

            break;
        }
        default: {
            // Nothing to do
            break;
        }
    }

    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<void, TString> PatchStatus(NMatrix::NApi::TScheduledAction& scheduledAction) {
    auto& status = *scheduledAction.MutableStatus();

    TInstant now = TInstant::Now();

    status.MutableScheduledAt()->CopyFrom(
        NProtoInterop::CastToProto(
            Max(NProtoInterop::CastFromProto(scheduledAction.GetSpec().GetStartPolicy().GetStartAt()), now)
        )
    );

    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<void, TString> ValidateMeta(const NMatrix::NApi::TScheduledAction& scheduledAction) {
    const auto& meta = scheduledAction.GetMeta();

    if (meta.GetId().empty()) {
        static const TString error = "Action id must be non-empty";
        return error;
    }

    // Just sanity check
    if (meta.GetGuid().empty()) {
        static const TString error = "Action guid must be non-empty";
        return error;
    }

    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<void, TString> ValidateSpec(const NMatrix::NApi::TScheduledAction& scheduledAction) {
    const auto& spec = scheduledAction.GetSpec();

    {
        if (!spec.GetStartPolicy().HasStartAt()) {
            static const TString error = "Start policy type is not specified";
            return error;
        }

        TInstant startAt = NProtoInterop::CastFromProto(spec.GetStartPolicy().GetStartAt());
        if (TInstant minAllowedStartAt = TInstant::Now() - MAX_ALLOWED_START_AT_CLOCK_DRIFT; startAt < minAllowedStartAt) {
            return TString::Join(
                "StartPolicy's start at is less than min allowed start at ", ToString(minAllowedStartAt),
                ", actual value is ", ToString(startAt),
                ", max allowed clock drift is ", ToString(MAX_ALLOWED_START_AT_CLOCK_DRIFT)
            );
        }
    }

    switch (spec.GetSendPolicy().GetPolicyCase()) {
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendOncePolicy: {
            const auto& sendOncePolicy = spec.GetSendPolicy().GetSendOncePolicy();

            if (sendOncePolicy.HasRetryPolicy()) {
                const auto& retryPolicy = sendOncePolicy.GetRetryPolicy();

                const TDuration minRestartPeriod = NProtoInterop::CastFromProto(retryPolicy.GetMinRestartPeriod());
                const TDuration maxRestartPeriod = NProtoInterop::CastFromProto(retryPolicy.GetMaxRestartPeriod());

                if (minRestartPeriod > maxRestartPeriod) {
                    return TString::Join(
                        "SendOncePolicy's min restart period is greater than max restart period",
                        " (", ToString(minRestartPeriod), " > ", ToString(maxRestartPeriod), ')'
                    );
                }
                if (minRestartPeriod < MIN_ALLOWED_RESTART_PERIOD) {
                    return TString::Join(
                        "SendOncePolicy's min restart period is less than ", ToString(MIN_ALLOWED_RESTART_PERIOD),
                        ", actual value is ", ToString(minRestartPeriod)
                    );
                }
            }
            break;
        }
        case NMatrix::NApi::TScheduledActionSpec::TSendPolicy::kSendPeriodicallyPolicy: {
            const auto& sendPeriodicallyPolicy = spec.GetSendPolicy().GetSendPeriodicallyPolicy();

            const TDuration period = NProtoInterop::CastFromProto(sendPeriodicallyPolicy.GetPeriod());

            if (period < MIN_ALLOWED_PERIODICALLY_POLICY_PERIOD) {
                return TString::Join(
                    "SendPeriodicallyPolicy's period is less than ", ToString(MIN_ALLOWED_PERIODICALLY_POLICY_PERIOD),
                    ", actual value is ", ToString(period)
                );
            }
            break;
        }
        default: {
            static const TString error = "Send policy type is not specified";
            return error;
        }
    }

    switch (spec.GetAction().GetActionTypeCase()) {
        case NMatrix::NApi::TAction::kMockAction: {
            break;
        }
        case NMatrix::NApi::TAction::kSendTechnicalPush: {
            const auto& sendTechnicalPush = spec.GetAction().GetSendTechnicalPush();

            {
                // Validate user device identifier
                const auto& userDeviceIdentifier = sendTechnicalPush.GetUserDeviceIdentifier();

                if (userDeviceIdentifier.GetPuid().empty()) {
                    static const TString error = "SendTechnicalPush user device identifier puid must be non-empty";
                    return error;
                }

                if (userDeviceIdentifier.GetDeviceId().empty()) {
                    static const TString error = "SendTechnicalPush user device identifier device id must be non-empty";
                    return error;
                }
            }

            {
                // Validate technical push
                const auto& technicalPush = sendTechnicalPush.GetTechnicalPush();

                if (technicalPush.GetTechnicalPushId().empty()) {
                    static const TString error = "SendTechnicalPush technical push id must be non-empty";
                    return error;
                }

                if (!technicalPush.HasSpeechKitDirective()) {
                    static const TString error = "SendTechnicalPush action SpeechKitDirective not specified";
                    return error;
                }

                if (!technicalPush.GetSpeechKitDirective().Is<NAlice::NSpeechKit::TDirective>()) {
                    return TString::Join(
                        "SendTechnicalPush action SpeechKitDirective type must be: ", NAlice::NSpeechKit::TDirective::GetDescriptor()->full_name(),
                        ", actual type is ", technicalPush.GetSpeechKitDirective().type_url()
                    );
                }
            }

            break;
        }
        default: {
            static const TString error = "Action type is not specified";
            return error;
        }
    }

    return TExpected<void, TString>::DefaultSuccess();
}

TExpected<void, TString> ValidateStatus(const NMatrix::NApi::TScheduledAction& scheduledAction) {
    Y_UNUSED(scheduledAction);
    return TExpected<void, TString>::DefaultSuccess();
}

} // namespace

TExpected<NMatrix::NApi::TScheduledAction, TString> CreateScheduledActionFromMetaAndSpec(
    const NMatrix::NApi::TScheduledActionMeta& meta,
    const NMatrix::NApi::TScheduledActionSpec& spec
) {
    NMatrix::NApi::TScheduledAction scheduledAction;

    scheduledAction.MutableMeta()->CopyFrom(meta);
    scheduledAction.MutableSpec()->CopyFrom(spec);

    // WARNING: order is important
    if (auto res = PatchMeta(scheduledAction); !res) {
        return res.Error();
    }
    if (auto res = PatchSpec(scheduledAction); !res) {
        return res.Error();
    }
    if (auto res = PatchStatus(scheduledAction); !res) {
        return res.Error();
    }

    if (auto res = ValidateMeta(scheduledAction); !res) {
        return res.Error();
    }
    if (auto res = ValidateSpec(scheduledAction); !res) {
        return res.Error();
    }
    if (auto res = ValidateStatus(scheduledAction); !res) {
        return res.Error();
    }

    return scheduledAction;
}

} // namespace NMatrix::NScheduler
