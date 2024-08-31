package mobile

import (
	"context"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type StereopairCreateRequest struct {
	Devices []StereopairCreateRequestDeviceInfo `json:"devices"`
}

func (r *StereopairCreateRequest) ToStereopairConfig() model.StereopairConfig {
	res := model.NewStereopairConfig()
	for _, device := range r.Devices {
		res.Devices = append(res.Devices, device.ToStereopairDeviceConfig())
	}
	return res
}

type StereopairCreateRequestDeviceInfo struct {
	ID      string                        `json:"id"`
	Role    model.StereopairDeviceRole    `json:"role"`
	Channel model.StereopairDeviceChannel `json:"channel"`
}

func (di StereopairCreateRequestDeviceInfo) ToStereopairDeviceConfig() model.StereopairDeviceConfig {
	return model.StereopairDeviceConfig{
		ID:      di.ID,
		Channel: di.Channel,
		Role:    di.Role,
	}
}

type StereopairListPossibleView struct {
	Status    string                                     `json:"status"`
	RequestID string                                     `json:"request_id"`
	Devices   []StereopairListPossibleResponseDeviceInfo `json:"devices"`
}

func (l *StereopairListPossibleView) From(ctx context.Context, devices model.Devices, stereopairs model.Stereopairs, quasarDeviceInfos quasarconfig.DeviceInfos) {
	var possible model.Devices
	for _, device := range devices {
		if role := stereopairs.GetDeviceRole(device.ID); role == model.NoStereopair && device.IsStereopairAvailable() {
			possible = append(possible, device.Clone())
		}
	}

	quasarDeviceInfoMap := quasarDeviceInfos.ToMap()
	combinations := make([]StereopairListPossibleResponseDeviceInfo, 0, len(possible))
	for _, leader := range possible {
		var leaderItem StereopairListPossibleResponseDeviceInfo

		stereopair, isStereopair := stereopairs.GetByDeviceID(leader.ID)

		for _, follower := range possible {
			if model.CanCreateStereopair(leader, follower, stereopairs) == nil {
				leaderItem.Followers = append(leaderItem.Followers, StereopairListPossibleResponseDeviceFollowerInfo{
					ID:      follower.ID,
					CanPair: true,
				})
			}
		}

		if quasarDeviceInfo, exist := quasarDeviceInfoMap[leader.ID]; exist && quasarDeviceInfo.Tandem != nil {
			var tandem TandemShortInfoView
			if isStereopair {
				tandem.Partner.FromStereopair(ctx, stereopair)
			} else {
				tandem.Partner.FromDevice(quasarDeviceInfo.Tandem.Partner)
			}

			leaderItem.Tandem = &tandem
		}

		if len(leaderItem.Followers) > 0 {
			if isStereopair {
				leaderItem.FromStereopair(ctx, stereopair)
			} else {
				leaderItem.FromDevice(ctx, leader)
			}
			combinations = append(combinations, leaderItem)
		}
	}

	sort.Sort(StereopairListPossibleResponseDeviceInfosByName(combinations))
	l.Devices = combinations
}

type StereopairListPossibleResponseDeviceInfo struct {
	ItemInfoView
	Followers []StereopairListPossibleResponseDeviceFollowerInfo `json:"followers"`
	Tandem    *TandemShortInfoView                               `json:"tandem,omitempty"`
}

type StereopairListPossibleResponseDeviceFollowerInfo struct {
	ID      string `json:"id"`
	CanPair bool   `json:"can_pair"`
}

type StereopairSetChannelsRequest struct {
	Devices []DeviceChannel `json:"devices"`
}

type DeviceChannel struct {
	ID      string                        `json:"id"`
	Channel model.StereopairDeviceChannel `json:"channel"`
}
