package mobile

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type TandemAvailablePairsForDeviceResponse struct {
	Status    string                      `json:"status"`
	RequestID string                      `json:"request_id"`
	Devices   []TandemAvailableDeviceView `json:"devices"`
}

func (r *TandemAvailablePairsForDeviceResponse) From(ctx context.Context, currentDevice model.Device, devices model.Devices, stereopairs model.Stereopairs, deviceInfos quasarconfig.DeviceInfos) {
	// FIXME: use only model information about the tandem
	r.Devices = make([]TandemAvailableDeviceView, 0, len(devices))
	for _, device := range devices {
		if quasarconfig.CanCreateTandem(currentDevice, device, stereopairs, deviceInfos) != nil {
			continue
		}
		isCurrentDevicePartner := deviceInfos.TandemPartnerID(currentDevice.ID) == device.ID
		var view TandemAvailableDeviceView
		switch stereopairs.GetDeviceRole(device.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(device.ID)
			view.FromStereopair(ctx, stereopair, isCurrentDevicePartner)
		case model.FollowerRole:
			// skip
			continue
		default:
			view.FromDevice(device, isCurrentDevicePartner)
		}
		r.Devices = append(r.Devices, view)
	}
}

type TandemAvailableDeviceView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	Type       model.DeviceType `json:"type"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
	QuasarInfo *QuasarInfo      `json:"quasar_info,omitempty"`
	RenderInfo *RenderInfoView  `json:"render_info,omitempty"`
	IsSelected bool             `json:"is_selected,omitempty"`
}

func (v *TandemAvailableDeviceView) FromDevice(device model.Device, isSelected bool) {
	v.ID = device.ID
	v.Name = device.Name
	v.ItemType = DeviceItemInfoViewType
	v.Type = device.Type
	v.IsSelected = isSelected
	v.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		v.QuasarInfo = &quasarInfo
	}
}

func (v *TandemAvailableDeviceView) FromStereopair(ctx context.Context, stereopair model.Stereopair, isSelected bool) {
	v.FromDevice(stereopair.GetLeaderDevice(), isSelected)
	v.ItemType = StereopairItemInfoViewType
	v.Name = stereopair.Name
	v.Stereopair = &StereopairView{}
	v.Stereopair.From(ctx, stereopair)
}

type TandemShortInfoView struct {
	Partner TandemPartnerShortInfoView `json:"partner"`
}

type TandemPartnerShortInfoView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	ItemType   ItemInfoViewType `json:"item_type"`
	Stereopair *StereopairView  `json:"stereopair,omitempty"`
}

func (v *TandemPartnerShortInfoView) FromDevice(device model.Device) {
	v.ID = device.ID
	v.Name = device.Name
	v.ItemType = DeviceItemInfoViewType
}

func (v *TandemPartnerShortInfoView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	v.FromDevice(stereopair.GetLeaderDevice())
	v.ItemType = StereopairItemInfoViewType
	v.Name = stereopair.Name
	v.Stereopair = &StereopairView{}
	v.Stereopair.From(ctx, stereopair)
}

type TandemPartnerInfoView struct {
	TandemPartnerShortInfoView
	RoomName string `json:"room_name,omitempty"`
}

func (v *TandemPartnerInfoView) FromDevice(device model.Device) {
	v.TandemPartnerShortInfoView.FromDevice(device)
	if device.Room != nil {
		v.RoomName = device.Room.Name
	}
}

func (v *TandemPartnerInfoView) FromStereopair(ctx context.Context, stereopair model.Stereopair) {
	v.TandemPartnerShortInfoView.FromStereopair(ctx, stereopair)
	if leaderDeviceRoom := stereopair.GetLeaderDevice().Room; leaderDeviceRoom != nil {
		v.RoomName = leaderDeviceRoom.Name
	}
}

type TandemCreateRequest struct {
	Speaker struct {
		ID string `json:"id"`
	} `json:"speaker"`
	Display struct {
		ID string `json:"id"`
	} `json:"display"`
}

type TandemDeviceConfigureView struct {
	Candidates []TandemDeviceCandidateConfigureView `json:"candidates"`
	Partner    *TandemPartnerInfoView               `json:"partner,omitempty"`
}

func (v *TandemDeviceConfigureView) From(ctx context.Context, device model.Device, userDevices model.Devices, deviceInfos quasarconfig.DeviceInfos, stereopairs model.Stereopairs) {
	v.Candidates = make([]TandemDeviceCandidateConfigureView, 0, len(deviceInfos))
	for _, candidate := range userDevices {
		if err := quasarconfig.CanCreateTandem(device, candidate, stereopairs, deviceInfos); err == nil {
			var view TandemDeviceCandidateConfigureView
			view.FromDevice(candidate)
			v.Candidates = append(v.Candidates, view)
		}
	}

	if tandemInfo := deviceInfos.TandemInfo(device.ID); tandemInfo != nil {
		var partnerView TandemPartnerInfoView
		stereopair, isStereopair := stereopairs.GetByDeviceID(tandemInfo.Partner.ID)
		if isStereopair {
			partnerView.FromStereopair(ctx, stereopair)
		} else {
			partnerView.FromDevice(tandemInfo.Partner)
		}
		v.Partner = &partnerView
	}
}

type TandemDeviceCandidateConfigureView struct {
	ID string `json:"id"`
}

func (v *TandemDeviceCandidateConfigureView) FromDevice(device model.Device) {
	v.ID = device.ID
}
