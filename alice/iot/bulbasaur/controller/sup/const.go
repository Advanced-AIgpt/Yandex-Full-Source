package sup

const (
	IOTProjectName string = "iot"
)

const (
	defaultSecondsTTL uint64 = 3600
)

const (
	AliceLogoIconID    string = "alice_logo"
	AliceLogoIcon      string = "https://yastatic.net/s3/home/apisearch/alice_icon.png"
	IOTDefaultTitle    string = "Алиса: Умный дом"
	IOTAppDefaultTitle string = "Дом с Алисой"
)

const (
	ScheduledScenarioFailedPushID         = "iot.timer_failed"
	ScheduledScenarioFailedThrottlePolicy = "iot-timer-failed"

	NewDevicesPushID         = "iot.new_device"
	NewDevicesThrottlePolicy = "iot-new-device"

	CreateScenarioPushID         = "iot.scenario_create"
	CreateScenarioThrottlePolicy = "iot-scenario-create"

	InvokeScenarioPushID         = "iot.scenario_invoked"
	InvokeScenarioThrottlePolicy = "iot-scenario-invoked"

	DevicesRangeCheckPushID         = "iot.devices_range_check"
	DevicesRangeCheckThrottlePolicy = "iot-devices-range-check"

	SkillsForcedUnlinked               = "iot.skills_force_unlink" // https://st.yandex-team.ru/PUSHLAUNCH-7625
	SkillsForcedUnlinkedThrottlePolicy = "iot-skills-force-unlink"
)
