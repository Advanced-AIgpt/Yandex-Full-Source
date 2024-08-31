package model

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TandemDeviceConfig struct {
	Partner TandemDeviceConfigPartner `json:"partner"`
	Group   TandemGroup               `json:"group"`
	Role    TandemRole                `json:"role"`
}

func (tdc *TandemDeviceConfig) Clone() *TandemDeviceConfig {
	if tdc == nil {
		return nil
	}
	return &TandemDeviceConfig{
		Partner: tdc.Partner.Clone(),
		Group:   tdc.Group.Clone(),
		Role:    tdc.Role,
	}
}

func (tdc TandemDeviceConfig) ToUserInfoProto() *common.TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig {
	return &common.TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig{
		Partner: tdc.Partner.ToUserInfoProto(),
	}
}

func (tdc *TandemDeviceConfig) fromUserInfoProto(p *common.TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig) {
	tandemDeviceConfig := TandemDeviceConfig{}
	tandemDeviceConfig.Partner.fromUserInfoProto(p.GetPartner())
	*tdc = tandemDeviceConfig
}

type TandemDeviceConfigPartner struct {
	ID string `json:"id"`
}

func (tdcp TandemDeviceConfigPartner) Clone() TandemDeviceConfigPartner {
	return TandemDeviceConfigPartner{ID: tdcp.ID}
}

func (tdcp TandemDeviceConfigPartner) ToUserInfoProto() *common.TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner {
	return &common.TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner{
		Id: tdcp.ID,
	}
}

func (tdcp *TandemDeviceConfigPartner) fromUserInfoProto(p *common.TIoTUserInfo_TDevice_TDeviceConfig_TTandemDeviceConfig_TTandemDeviceConfigPartner) {
	partner := TandemDeviceConfigPartner{
		ID: p.GetId(),
	}
	*tdcp = partner
}

type TandemGroup struct {
	ID uint64 `json:"id"`
}

func (tg TandemGroup) Clone() TandemGroup {
	return TandemGroup{
		ID: tg.ID,
	}
}

func CanCreateTandem(currentDevice Device, candidate Device, stereopairs Stereopairs) error {
	switch {
	case currentDevice.ID == candidate.ID:
		return xerrors.Errorf("device %s and device %s could not be tandemized: it is the same device", currentDevice.ID, candidate.ID)
	case !currentDevice.IsTandemCompatibleWith(candidate):
		return xerrors.Errorf("device %s and device %s could not be tandemized: incompatible types", currentDevice.ID, candidate.ID)
	case currentDevice.HouseholdID != candidate.HouseholdID:
		return xerrors.Errorf("device %s and device %s could not be tandemized: different households", currentDevice.ID, candidate.ID)
	case candidate.InternalConfig.Tandem != nil && candidate.InternalConfig.Tandem.Partner.ID != currentDevice.ID:
		return xerrors.Errorf("device %s and device %s could not be tandemized: device %s already in tandem with other device", currentDevice.ID, candidate.ID, candidate.ID)
	case stereopairs.GetDeviceRole(candidate.ID) == FollowerRole:
		return xerrors.Errorf("device %s and device %s could not be tandemized: device %s is stereopair follower", currentDevice.ID, candidate.ID, candidate.ID)
	case stereopairs.GetDeviceRole(currentDevice.ID) == FollowerRole:
		return xerrors.Errorf("device %s and device %s could not be tandemized: device %s is stereopair follower", currentDevice.ID, candidate.ID, currentDevice.ID)
	default:
		return nil
	}
}

type TandemRole string

const (
	LeaderTandemRole   TandemRole = "leader"
	FollowerTandemRole TandemRole = "follower"
)
