package arguments

import (
	"context"
	"time"

	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type ExtractedActionIntent struct {
	Devices       model.Devices `json:"devices"`
	CreatedTime   time.Time     `json:"created_time"`
	RequestedTime time.Time     `json:"requested_time,omitempty"`
}

func (eai *ExtractedActionIntent) ToProto(ctx context.Context) *protos.ExtractedActionIntent {
	return &protos.ExtractedActionIntent{
		Devices:       eai.Devices.ToUserInfoProto(ctx),
		CreatedTime:   timestamppb.New(eai.CreatedTime),
		RequestedTime: timestamppb.New(eai.RequestedTime),
	}
}

func (eai *ExtractedActionIntent) FromProto(p *protos.ExtractedActionIntent) {
	actionIntent := ExtractedActionIntent{}

	actionIntent.Devices = make(model.Devices, 0, len(p.GetDevices()))
	for _, protoDevice := range p.GetDevices() {
		device := model.Device{}
		device.FromUserInfoProtoSimple(protoDevice)
		actionIntent.Devices = append(actionIntent.Devices, device)
	}

	actionIntent.CreatedTime = p.GetCreatedTime().AsTime()
	actionIntent.RequestedTime = p.GetRequestedTime().AsTime()

	*eai = actionIntent
}

// ToScenarioLaunch is used when applying delayed actions
func (eai ExtractedActionIntent) ToScenarioLaunch() model.ScenarioLaunch {
	stepAction := eai.toScenarioStepAction()

	launch := model.ScenarioLaunch{
		LaunchTriggerType: model.TimerScenarioTriggerType,
		Steps:             model.ScenarioSteps{&stepAction},
		Created:           timestamp.FromTime(eai.CreatedTime),
		Scheduled:         timestamp.FromTime(eai.RequestedTime),
		Status:            model.ScenarioLaunchScheduled,
	}
	launch.ScenarioName = launch.GetTimerScenarioName()
	launch.Icon = model.ScenarioIcon(launch.ScenarioSteps().AggregateDeviceType())

	return launch
}

func (eai ExtractedActionIntent) toScenarioStepAction() model.ScenarioStepActions {
	stepAction := model.MakeScenarioStepByType(model.ScenarioStepActionsType).(*model.ScenarioStepActions)
	stepAction.SetParameters(model.ScenarioStepActionsParameters{
		Devices: eai.Devices.ToTimerScenarioLaunchDevices(),
	})
	return *stepAction
}
