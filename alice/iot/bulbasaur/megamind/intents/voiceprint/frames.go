package voiceprint

import (
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	iotprotopb "a.yandex-team.ru/alice/protos/data/scenario/iot"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type StatusFrame struct {
	PUID          uint64
	Source        iotprotopb.TEnrollmentStatus_ESource
	StatusFailure *StatusFailure
}

func (f *StatusFrame) FromTypedSemanticFrame(p *megamindcommonpb.TTypedSemanticFrame) error {
	frame := p.GetEnrollmentStatusSemanticFrame()
	if frame == nil {
		return xerrors.New("failed to unmarshal frame: enrollment status semantic frame is nil")
	}
	puidString := frame.GetPuid().GetStringValue()
	parsedPUID, err := strconv.ParseUint(puidString, 10, 64)
	if err != nil {
		return xerrors.Errorf("failed to parse puid string: %w", err)
	}
	f.PUID = parsedPUID
	f.Source = frame.GetStatus().GetEnrollmentStatus().GetSource()
	success := frame.GetStatus().GetEnrollmentStatus().GetSuccess()
	if !success {
		f.StatusFailure = NewStatusFailure(
			frame.GetStatus().GetEnrollmentStatus().GetFailureReason(),
			frame.GetStatus().GetEnrollmentStatus().GetFailureReasonDetails(),
		)
	}
	return nil
}

func (f *StatusFrame) ToTypedSemanticFrame() *megamindcommonpb.TTypedSemanticFrame {
	failureReason := iotprotopb.TEnrollmentStatus_Unknown
	failureReasonDetails := ""
	if f.StatusFailure != nil {
		failureReason = f.StatusFailure.Reason
		failureReasonDetails = f.StatusFailure.Details
	}
	return &megamindcommonpb.TTypedSemanticFrame{
		Type: &megamindcommonpb.TTypedSemanticFrame_EnrollmentStatusSemanticFrame{
			EnrollmentStatusSemanticFrame: &megamindcommonpb.TEnrollmentStatusSemanticFrame{
				Puid: &megamindcommonpb.TStringSlot{Value: &megamindcommonpb.TStringSlot_StringValue{StringValue: strconv.FormatUint(f.PUID, 10)}},
				Status: &megamindcommonpb.TEnrollmentStatusTypeSlot{
					Value: &megamindcommonpb.TEnrollmentStatusTypeSlot_EnrollmentStatus{
						EnrollmentStatus: &iotprotopb.TEnrollmentStatus{
							Success:              f.StatusFailure == nil,
							Source:               f.Source,
							FailureReason:        failureReason,
							FailureReasonDetails: failureReasonDetails,
						},
					},
				},
			},
		},
	}
}

type StatusFailure struct {
	Reason  iotprotopb.TEnrollmentStatus_EFailureReason
	Details string
}

func NewStatusFailure(reason iotprotopb.TEnrollmentStatus_EFailureReason, details string) *StatusFailure {
	return &StatusFailure{
		Reason:  reason,
		Details: details,
	}
}

func errorCodeByStatusFailureReason(reason iotprotopb.TEnrollmentStatus_EFailureReason) model.ErrorCode {
	switch reason {
	case iotprotopb.TEnrollmentStatus_ClientTimeout:
		return model.VoiceprintAddingTimeoutErrorCode
	case iotprotopb.TEnrollmentStatus_ScenarioError:
		return model.VoiceprintScenarioErrorCode
	default:
		return model.InternalError
	}
}
