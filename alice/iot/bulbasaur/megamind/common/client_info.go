package common

import (
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/client/protos"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type ClientInfo struct {
	libmegamind.ClientInfo

	isTandem bool
}

func (ci ClientInfo) IsEmpty() bool {
	return ci == ClientInfo{}
}

func NewClientInfo(p *protos.TClientInfoProto, dataSources map[int32]*scenarios.TDataSource) ClientInfo {
	var clientInfo ClientInfo
	clientInfo.ClientInfo = libmegamind.NewClientInfo(p)
	clientInfo.extendFromDataSources(dataSources)

	return clientInfo
}

func (ci ClientInfo) LocalTime(t time.Time) time.Time {
	return t.In(ci.GetLocation(model.DefaultTimezone))
}

func (ci ClientInfo) IsTandem() bool {
	return ci.isTandem
}

func (ci *ClientInfo) extendFromDataSources(dataSources map[int32]*scenarios.TDataSource) {
	if tandemDataSource, found := dataSources[int32(common.EDataSourceType_TANDEM_ENVIRONMENT_STATE)]; found {
		ci.isTandem = false
		tandemEnvironmentState := tandemDataSource.GetTandemEnvironmentState()
		for _, tandemGroup := range tandemEnvironmentState.GetGroups() {
			for _, tandemDevice := range tandemGroup.GetDevices() {
				if tandemDevice.GetId() == ci.DeviceID {
					ci.isTandem = true
					return
				}
			}
		}
	}
	if environmentStateDataSource, found := dataSources[int32(common.EDataSourceType_ENVIRONMENT_STATE)]; found {
		ci.isTandem = false
		environmentState := environmentStateDataSource.GetEnvironmentState()
		for _, group := range environmentState.GetGroups() {
			if group.Type != common.TEnvironmentGroupInfo_tandem {
				continue
			}
			for _, tandemDevice := range group.GetDevices() {
				if tandemDevice.GetId() == ci.DeviceID {
					ci.isTandem = true
					return
				}
			}
		}
	}
}

// GetIotLocation returns householdID and room of client device, if applicable
func (ci ClientInfo) GetIotLocation(info model.UserInfo) (string, *model.Room, bool) {
	if !ci.IsSmartSpeaker() {
		return "", nil, false
	}
	device, ok := info.Devices.GetDeviceByQuasarExtID(ci.DeviceID)
	if !ok {
		return "", nil, false
	}
	return device.HouseholdID, device.Room, true
}

func (ci ClientInfo) GetSupportedVideoStreamProtocols() []model.VideoStreamProtocol {
	switch {
	case ci.IsYandexModule2(), ci.IsTvDevice(), ci.IsYandexStationMaxSpeaker(), ci.IsYandexStationSpeaker(), ci.IsIotApp(), ci.IsTandem():
		return []model.VideoStreamProtocol{model.HLSStreamingProtocol, model.ProgressiveMP4StreamingProtocol}
	}

	return nil
}
