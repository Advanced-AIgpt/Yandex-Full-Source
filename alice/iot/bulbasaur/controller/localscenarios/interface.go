package localscenarios

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type Controller interface {
	// SyncLocalScenarios sends SyncIotScenarios TSF to ZigbeeSpeakers from userDevices to notify them about scenario changes
	SyncLocalScenarios(ctx context.Context, userID uint64, scenarios model.Scenarios, userDevices model.Devices) error
	// AddLocalScenariosToSpeaker sends AddScenariosSpeechkitDirective skDirective with local scenarios' data to endpointID
	AddLocalScenariosToSpeaker(ctx context.Context, userID uint64, endpointID string, scenarios model.Scenarios, userDevices model.Devices) error
	// RemoveLocalScenarios sends RemoveScenariosSpeechkitDirective skDirective to all zigbee speakers with scenarioIDs to remove
	RemoveLocalScenarios(ctx context.Context, userID uint64, scenarioIDs []string, userDevices model.Devices) error
	// RemoveLocalScenariosFromSpeaker sends RemoveScenariosSpeechkitDirective skDirective to endpointID with scenarioIDs to remove
	RemoveLocalScenariosFromSpeaker(ctx context.Context, userID uint64, endpointID string, scenarioIDs []string) error
}
