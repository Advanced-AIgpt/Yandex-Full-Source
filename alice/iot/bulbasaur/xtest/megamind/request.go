package xtestmegamind

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/library/client/protos"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type ScenarioRunRequest struct {
	*scenarios.TScenarioRunRequest
}

func (r *ScenarioRunRequest) WithClientInfo(clientInfo common.ClientInfo) *ScenarioRunRequest {
	clientInfoProto := protos.TClientInfoProto{
		AppId:              &clientInfo.AppID,
		AppVersion:         &clientInfo.AppVersion,
		OsVersion:          &clientInfo.OsVersion,
		Platform:           &clientInfo.Platform,
		Uuid:               &clientInfo.UUID,
		DeviceId:           &clientInfo.DeviceID,
		Lang:               &clientInfo.Lang,
		ClientTime:         &clientInfo.ClientTime,
		Timezone:           &clientInfo.Timezone,
		Epoch:              &clientInfo.Epoch,
		DeviceModel:        &clientInfo.DeviceModel,
		DeviceManufacturer: &clientInfo.DeviceManufacturer,
	}

	r.BaseRequest.ClientInfo = &clientInfoProto
	return r
}

func NewScenarioRunRequest() *ScenarioRunRequest {
	return &ScenarioRunRequest{
		TScenarioRunRequest: &scenarios.TScenarioRunRequest{
			BaseRequest: &scenarios.TScenarioBaseRequest{
				RequestId:          "",
				ServerTimeMs:       0,
				RandomSeed:         0,
				ClientInfo:         nil,
				Location:           nil,
				Interfaces:         nil,
				DeviceState:        nil,
				State:              nil,
				Experiments:        nil,
				Options:            nil,
				IsNewSession:       false,
				UserPreferences:    nil,
				IsSessionReset:     false,
				UserLanguage:       0,
				UserClassification: nil,
				Memento:            nil,
				RequestSource:      0,
				IsStackOwner:       false,
				Origin:             nil,
				NluFeatures:        nil,
			},
			Input: &scenarios.TInput{},
		},
	}
}
