package steps

type ActionsResultApplyArguments struct {
	LaunchID      string               `json:"launch_id"`
	StepIndex     int                  `json:"step_index"`
	DeviceResults []DeviceActionResult `json:"device_results"`
}

func (a ActionsResultApplyArguments) ProcessorName() string {
	return ActionsResultCallbackProcessorName
}

func (a ActionsResultApplyArguments) IsUniversalApplyArguments() {}
