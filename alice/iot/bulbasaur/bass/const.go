package bass

const (
	// Vulpix Frames
	// deprecated
	BroadcastSuccessSemanticFrame SemanticFrameType = "iot_broadcast_success"
	BroadcastFailureSemanticFrame SemanticFrameType = "iot_broadcast_failure"
	// v2
	DiscoverySuccessSemanticFrame SemanticFrameType = "iot_discovery_success"
)

const (
	IoTScenarioSpeakerActionSemanticFrame SemanticFrameType = "iot_scenario_speaker_action"
	YandexIOActionSemanticFrame           SemanticFrameType = "yandex_io_action"
	ScenarioStepActionsSemanticFrame      SemanticFrameType = "scenario_step_actions"
)

const IOTClientID string = "ru.yandex.quasar.iot"
