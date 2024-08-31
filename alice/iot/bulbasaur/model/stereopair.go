package model

import (
	"fmt"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type Stereopair struct {
	ID      string
	Name    string
	Created timestamp.PastTimestamp
	Config  StereopairConfig
	Devices Devices
}

func (stereopair *Stereopair) New(config StereopairConfig, devices DevicesMapByID, stereopairs Stereopairs, now timestamp.PastTimestamp) error {
	if _, err := config.Validate(nil); err != nil {
		return xerrors.Errorf("failed to validate stereopair config: %w", err)
	}

	leaderConfig, ok := config.Devices.GetDeviceByRole(LeaderRole)
	if !ok {
		return xerrors.New("failed to find leader config")
	}
	followerConfig, ok := config.Devices.GetDeviceByRole(FollowerRole)
	if !ok {
		return xerrors.New("failed to find follower config")
	}

	leaderDevice, ok := devices[leaderConfig.ID]
	if !ok {
		return xerrors.Errorf("failed to find leader device: %q", leaderConfig.ID)
	}

	followerDevice, ok := devices[followerConfig.ID]
	if !ok {
		return xerrors.Errorf("failed to find follower device: %q", followerDevice.ID)
	}
	pairingDevices := make(Devices, 0, len(config.Devices))
	for _, cfg := range config.Devices {
		device, ok := devices[cfg.ID]
		if !ok {
			return xerrors.Errorf("failed to find device of stereopair config in full devices map while creating stereopair, bad device id: %q", cfg.ID)
		}
		pairingDevices = append(pairingDevices, device)
	}

	// check devices compatibility
	for i := range pairingDevices {
		for j := i + 1; j < len(pairingDevices); j++ {
			if err := CanCreateStereopair(leaderDevice, followerDevice, stereopairs); err != nil {
				return xerrors.Errorf("failed to pair devices: %w", err)
			}
		}
	}

	id, err := uuid.NewV4()
	if err != nil {
		return xerrors.Errorf("failed to generate stereopair id: %w", err)
	}

	stereopair.ID = id.String()
	stereopair.Name = "Стереопара"
	stereopair.Created = now
	stereopair.Config = config
	stereopair.Devices = pairingDevices
	return nil
}

func (stereopair *Stereopair) GetLeaderDevice() Device {
	device, _ := stereopair.Devices.GetDeviceByID(stereopair.Config.GetLeaderID())
	return device
}

func (stereopair Stereopair) AssertName() error {
	return validateDeviceName(stereopair.Name, true)
}

func (stereopair Stereopair) DevicesIDs() []string {
	result := make([]string, 0)
	for _, device := range stereopair.Config.Devices {
		result = append(result, device.ID)
	}
	return result
}

func (stereopair Stereopair) ToUserInfoProto() *common.TIoTUserInfo_TStereopair {
	p := &common.TIoTUserInfo_TStereopair{
		Id:   stereopair.ID,
		Name: stereopair.Name,
	}

	for _, device := range stereopair.Config.Devices {
		protoDevice := &common.TIoTUserInfo_TStereopair_TStereopairDevice{
			Id:      device.ID,
			Role:    device.Role.ToUserInfoProto(),
			Channel: device.Channel.ToUserInfoProto(),
		}
		p.Devices = append(p.Devices, protoDevice)
	}
	return p
}

func MakeStereopairFromUserInfoProto(p *common.TIoTUserInfo_TStereopair) (sp Stereopair) {
	sp.ID = p.GetId()
	sp.Name = p.GetName()
	protoDevices := p.GetDevices()
	sp.Config.Devices = make(StereopairDeviceConfigs, 0, len(protoDevices))
	for _, protoDevice := range protoDevices {
		sp.Config.Devices = append(sp.Config.Devices, StereopairDeviceConfig{
			ID:      protoDevice.GetId(),
			Channel: MakeStereopairDeviceChannelFromUserInfoProto(protoDevice.GetChannel()),
			Role:    MakeStereopairDeviceRoleFromUserInfoProto(protoDevice.GetRole()),
		})
	}
	return sp
}

type StereopairDeviceChannel string

const (
	LeftChannel  StereopairDeviceChannel = "left"
	RightChannel StereopairDeviceChannel = "right"
)

func (c StereopairDeviceChannel) ToUserInfoProto() common.TIoTUserInfo_TStereopair_EStereopairChannel {
	switch c {
	case LeftChannel:
		return common.TIoTUserInfo_TStereopair_LeftStereopairChannel
	case RightChannel:
		return common.TIoTUserInfo_TStereopair_RightStereopairChannel
	default:
		panic(fmt.Sprintf("unknown stereopair channel: %q", c))
	}
}

func MakeStereopairDeviceChannelFromUserInfoProto(p common.TIoTUserInfo_TStereopair_EStereopairChannel) StereopairDeviceChannel {
	switch p {
	case common.TIoTUserInfo_TStereopair_LeftStereopairChannel:
		return LeftChannel
	case common.TIoTUserInfo_TStereopair_RightStereopairChannel:
		return RightChannel
	default:
		panic(fmt.Sprintf("unknown stereopair channel: %q", p.String()))
	}
}

type StereopairDeviceRole string

const (
	LeaderRole   StereopairDeviceRole = "leader"
	FollowerRole StereopairDeviceRole = "follower"
	NoStereopair StereopairDeviceRole = ""
)

func (r StereopairDeviceRole) ToUserInfoProto() common.TIoTUserInfo_TStereopair_EStereopairRole {
	switch r {
	case NoStereopair:
		panic("failed to call StereopairDeviceRole.ToUserInfoProto: must be call only for devices in stereopair.")
	case LeaderRole:
		return common.TIoTUserInfo_TStereopair_LeaderStereopairRole
	case FollowerRole:
		return common.TIoTUserInfo_TStereopair_FollowerStereopairRole
	default:
		panic(fmt.Sprintf("unknown stereopair role: %q", r))
	}
}

func MakeStereopairDeviceRoleFromUserInfoProto(p common.TIoTUserInfo_TStereopair_EStereopairRole) StereopairDeviceRole {
	switch p {
	case common.TIoTUserInfo_TStereopair_UnknownStereopairRole:
		panic("failed to call MakeStereopairDeviceRoleFromUserInfoProto: must be call only for devices in stereopair.")
	case common.TIoTUserInfo_TStereopair_LeaderStereopairRole:
		return LeaderRole
	case common.TIoTUserInfo_TStereopair_FollowerStereopairRole:
		return FollowerRole
	default:
		panic(fmt.Sprintf("unknown stereopair role: %q", p))
	}
}

type StereopairDeviceConfig struct {
	ID      string                  `json:"id"`
	Channel StereopairDeviceChannel `json:"channel"`
	Role    StereopairDeviceRole    `json:"role"`
}

func (d *StereopairDeviceConfig) Validate(_ *valid.ValidationCtx) (bool, error) {
	var errors valid.Errors
	if d.Channel != LeftChannel && d.Channel != RightChannel {
		errors = append(errors, xerrors.Errorf("bad channel: %q", d.Channel))
	}
	if d.Role != LeaderRole && d.Role != FollowerRole {
		errors = append(errors, xerrors.Errorf("bad role: %q", d.Role))
	}
	if len(errors) == 0 {
		return false, nil
	}
	return false, errors
}

type StereopairDeviceConfigs []StereopairDeviceConfig

func (s StereopairDeviceConfigs) Clone() StereopairDeviceConfigs {
	if s == nil {
		return nil
	}
	res := make(StereopairDeviceConfigs, len(s))
	copy(res, s)
	return res
}

func (s StereopairDeviceConfigs) DeviceIDs() []string {
	res := make([]string, 0, len(s))
	for _, cfg := range s {
		res = append(res, cfg.ID)
	}
	return res
}

func (s StereopairDeviceConfigs) GetDeviceByRole(role StereopairDeviceRole) (StereopairDeviceConfig, bool) {
	for _, cfg := range s {
		if cfg.Role == role {
			return cfg, true
		}
	}
	return StereopairDeviceConfig{}, false
}

func (s StereopairDeviceConfigs) GetConfigByID(id string) (StereopairDeviceConfig, bool) {
	for _, cfg := range s {
		if cfg.ID == id {
			return cfg, true
		}
	}
	return StereopairDeviceConfig{}, false
}

func (s StereopairDeviceConfigs) SetConfig(config StereopairDeviceConfig) error {
	for i := range s {
		if s[i].ID == config.ID {
			s[i] = config
			return nil
		}
	}
	return xerrors.Errorf("device not found in stereopair config: %q", config.ID)
}

type StereopairConfig struct {
	Devices StereopairDeviceConfigs `json:"devices"`
}

func (s StereopairConfig) Clone() StereopairConfig {
	return StereopairConfig{
		Devices: s.Devices.Clone(),
	}
}

func NewStereopairConfig() StereopairConfig {
	return StereopairConfig{Devices: make(StereopairDeviceConfigs, 0, 2)}
}

func (s *StereopairConfig) GetLeaderID() (id string) {
	for _, device := range s.Devices {
		if device.Role == LeaderRole {
			return device.ID
		}
	}
	return ""
}

func (s *StereopairConfig) GetFollowerIDs() []string {
	res := make([]string, 0, len(s.Devices)-1)
	for _, device := range s.Devices {
		if device.Role == FollowerRole {
			res = append(res, device.ID)
		}
	}
	return res
}

func (s *StereopairConfig) GetByChannel(ch StereopairDeviceChannel) StereopairDeviceConfig {
	for _, device := range s.Devices {
		if device.Channel == ch {
			return device
		}
	}
	return StereopairDeviceConfig{}
}

func (s *StereopairConfig) Validate(ctx *valid.ValidationCtx) (bool, error) {
	var errors valid.Errors

	if len(s.Devices) != 2 {
		errors = append(errors, xerrors.Errorf("must be exactly two devices"))
	}

	leaderCount := 0
	leftChannelCount := 0
	for _, device := range s.Devices {
		if device.Role == LeaderRole {
			leaderCount++
		}
		if device.Channel == LeftChannel {
			leftChannelCount++
		}
	}
	if leaderCount != 1 {
		errors = append(errors, xerrors.Errorf("must be exactly one leader and one follower"))
	}
	if leftChannelCount != 1 {
		errors = append(errors, xerrors.Errorf("must be exactly one left and one right channel"))
	}

	if len(errors) == 0 {
		return false, nil
	}

	return false, errors
}

type Stereopairs []Stereopair

func (sps Stereopairs) IDs() []string {
	result := make([]string, 0, len(sps))
	for _, sp := range sps {
		result = append(result, sp.ID)
	}
	return result
}

func (sps Stereopairs) GetDeviceRole(id string) StereopairDeviceRole {
	sp, exists := sps.GetByDeviceID(id)
	if !exists {
		return NoStereopair
	}

	device, _ := sp.Config.Devices.GetConfigByID(id)
	return device.Role
}

func (sps Stereopairs) GetByID(id string) (Stereopair, bool) {
	for i := range sps {
		if sps[i].ID == id {
			return sps[i], true
		}
	}
	return Stereopair{}, false
}

func (sps Stereopairs) GetByDeviceID(deviceID string) (Stereopair, bool) {
	for i := range sps {
		if _, exist := sps[i].Config.Devices.GetConfigByID(deviceID); exist {
			return sps[i], true
		}
	}
	return Stereopair{}, false
}

func (sps Stereopairs) Len() int {
	return len(sps)
}

func (sps Stereopairs) Less(i, j int) bool {
	sI := sps[i]
	sJ := sps[j]

	switch {
	case sI.Name != sJ.Name:
		return sI.Name < sJ.Name
	default:
		return sI.ID < sJ.ID
	}
}

func (sps Stereopairs) Swap(i, j int) {
	sps[i], sps[j] = sps[j], sps[i]
}

func CanCreateStereopair(leader, follower Device, userStereopairs Stereopairs) error {
	leaderStereopair, leaderInStereopair := userStereopairs.GetByDeviceID(leader.ID)
	followerStereopair, followerInStereopair := userStereopairs.GetByDeviceID(follower.ID)

	switch {
	case leader.ID == follower.ID:
		return xerrors.Errorf("can't pair device with same device: %q", leader.ID)
	case leaderInStereopair:
		return xerrors.Errorf("leader already in stereopair: streopairID %q, leader device id: %q", leaderStereopair.ID, leader.ID)
	case followerInStereopair:
		return xerrors.Errorf("follower already in stereopair: streopairID %q, follower device id: %q", followerStereopair.ID, follower.ID)
	case !leader.IsStereopairAvailable():
		return xerrors.Errorf("leader device not available for pairing: %q", leader.ID)
	case !follower.IsStereopairAvailable():
		return xerrors.Errorf("follower device not available for pairing: %q", follower.ID)
	case !StereopairAvailablePairs[leader.Type].Contains(follower.Type):
		return xerrors.Errorf("can't pair devices with incompatible types: %q (%q) and %q (%q)", leader.ID, leader.Type, follower.ID, follower.Type)
	case leader.HouseholdID != follower.HouseholdID:
		return xerrors.Errorf("can't pair devices from different households: %q (%q) and %q (%q)", leader.ID, leader.HouseholdID, follower.ID, follower.HouseholdID)
	}

	return nil
}

func CanCreateStereopairWithDevice(device Device, userDevices Devices, stereopairs Stereopairs) bool {
	for _, otherDevice := range userDevices {
		if CanCreateStereopair(device, otherDevice, stereopairs) == nil || CanCreateStereopair(otherDevice, device, stereopairs) == nil {
			return true
		}
	}
	return false
}
