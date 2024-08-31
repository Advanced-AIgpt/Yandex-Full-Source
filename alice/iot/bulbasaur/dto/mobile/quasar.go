package mobile

import (
	"context"
	"encoding/json"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type QuasarUpdateConfigRequest struct {
	Config  json.RawMessage `json:"config"`
	Version string          `json:"version"`
}

type QuasarUpdateConfigResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	Version   string `json:"version"`
}

type QuasarConfigurationResponse struct {
	Status    string                  `json:"status"`
	RequestID string                  `json:"request_id"`
	Devices   []DeviceConfigureV2View `json:"devices"`
}

func (v *QuasarConfigurationResponse) From(ctx context.Context, devices model.Devices, deviceInfos quasarconfig.DeviceInfos, households model.Households, stereopairs model.Stereopairs, voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs) {
	v.Devices = make([]DeviceConfigureV2View, 0, len(devices))
	householdsMap := households.ToMap()
	filteredDevices := devices.QuasarDevices()
	filteredDevices = filteredDevices.FilterByStereopairFollowers(stereopairs)
	for _, device := range filteredDevices {
		household, exist := householdsMap[device.HouseholdID]
		if !exist {
			continue
		}
		var view DeviceConfigureV2View
		view.FromDevice(ctx, device, filteredDevices, household, deviceInfos, stereopairs, voiceprintDeviceConfigs)
		v.Devices = append(v.Devices, view)
	}
	sort.Sort(DeviceConfigureV2ViewSorting(v.Devices))
}

type SpeakerNewsTopicsResponse struct {
	Status    string                 `json:"status"`
	RequestID string                 `json:"request_id"`
	Topics    []SpeakerNewsTopicView `json:"topics"`
	Providers []string               `json:"providers"`
}

func (r *SpeakerNewsTopicsResponse) FillTopicsAndProviders() error {
	r.Topics = make([]SpeakerNewsTopicView, 0, len(model.KnownSpeakerNewsTopics))
	r.Providers = make([]string, 0) // empty for now: structure will come later
	for _, topic := range model.KnownSpeakerNewsTopics {
		var topicView SpeakerNewsTopicView
		if err := topicView.FromSpeakerNewsTopic(model.SpeakerNewsTopic(topic)); err != nil {
			return xerrors.Errorf("failed to fill topics: %w", err)
		}
		r.Topics = append(r.Topics, topicView)
	}
	sort.Sort(SpeakerNewsTopicViewSorting(r.Topics))
	return nil
}

type SpeakerNewsTopicView struct {
	ID   model.SpeakerNewsTopic `json:"id"`
	Name string                 `json:"name"`
}

func (v *SpeakerNewsTopicView) FromSpeakerNewsTopic(topic model.SpeakerNewsTopic) error {
	v.ID = topic
	name, exist := speakerNewsTopicsNameMap[topic]
	if !exist {
		return xerrors.Errorf("unknown speaker news topic: %s", topic)
	}
	v.Name = name
	return nil
}

type SpeakerSoundsResponse struct {
	Status    string             `json:"status"`
	RequestID string             `json:"request_id"`
	Sounds    []SpeakerSoundView `json:"sounds"`
}

func (r *SpeakerSoundsResponse) FillSounds(categoryID model.SpeakerSoundCategoryID) {
	r.Sounds = make([]SpeakerSoundView, 0, len(model.KnownSpeakerSounds))
	for _, speakerSound := range model.KnownSpeakerSounds {
		if speakerSound.CategoryID == categoryID {
			var view SpeakerSoundView
			view.FromSpeakerSound(speakerSound)
			r.Sounds = append(r.Sounds, view)
		}
	}
	sort.Sort(SpeakerSoundViewSorting(r.Sounds))
}

type SpeakerSoundView struct {
	ID       model.SpeakerSoundID `json:"id"`
	Name     string               `json:"name"`
	AudioURL string               `json:"audio_url"`
}

func (v *SpeakerSoundView) FromSpeakerSound(speakerSound model.SpeakerSound) {
	v.ID = speakerSound.ID
	v.Name = speakerSound.Name
	v.AudioURL = speakerSound.AudioURL()
}

type SpeakerSoundCategoriesResponse struct {
	Status     string                     `json:"status"`
	RequestID  string                     `json:"request_id"`
	Categories []SpeakerSoundCategoryView `json:"categories"`
}

func (r *SpeakerSoundCategoriesResponse) FillCategories() {
	r.Categories = make([]SpeakerSoundCategoryView, 0, len(model.KnownSpeakerSoundCategories))
	for _, category := range model.KnownSpeakerSoundCategories {
		var view SpeakerSoundCategoryView
		view.FromSpeakerSoundCategory(category)
		r.Categories = append(r.Categories, view)
	}
	sort.Sort(SpeakerSoundCategoryViewSorting(r.Categories))
}

type TargetSpeakerType string

var (
	StereopairSpeakerType TargetSpeakerType = "stereopair"
	RegularSpeakerType    TargetSpeakerType = "speaker"
)

type TargetSpeaker struct {
	ID   string            `json:"id"`
	Type TargetSpeakerType `json:"type"`
}

type SpeakerDevicesDiscoveryRequest struct {
	TargetSpeaker TargetSpeaker `json:"target_speaker"`
	DiscoveryType string        `json:"discovery_type,omitempty"`
	Attempt       int           `json:"attempt"`
}

type SpeakerDevicesDiscoveryResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	Code      string `json:"code,omitempty"`
	Timeout   int    `json:"timeout,omitempty"`
}

type SpeakerSoundCategoryView struct {
	ID   model.SpeakerSoundCategoryID `json:"id"`
	Name string                       `json:"name"`
}

func (v *SpeakerSoundCategoryView) FromSpeakerSoundCategory(speakerSoundCategory model.SpeakerSoundCategory) {
	v.ID = speakerSoundCategory.ID
	v.Name = speakerSoundCategory.Name
}

type SpeakerCapabilitiesResponse struct {
	Status       string                `json:"status"`
	RequestID    string                `json:"request_id"`
	Capabilities []CapabilityStateView `json:"capabilities"`
}

func (r *SpeakerCapabilitiesResponse) FillCapabilities(ctx context.Context) {
	quasarCapabilities := model.GenerateQuasarCapabilities(ctx, model.YandexStation2DeviceType)
	r.Capabilities = make([]CapabilityStateView, 0, len(quasarCapabilities))
	for _, capability := range quasarCapabilities {
		if experiments.DeduplicateQuasarCapabilities.IsEnabled(ctx) && capability.Instance() == string(model.PhraseActionCapabilityInstance) {
			continue
		}
		var view CapabilityStateView
		view.FromCapability(capability)
		r.Capabilities = append(r.Capabilities, view)
	}
	sort.Sort(CapabilityStateViewSorting(r.Capabilities))
}

type DeviceQuasarConfigureView struct {
	QuasarInfo          *QuasarInfo     `json:"quasar_info,omitempty"`
	QuasarConfig        json.RawMessage `json:"quasar_config,omitempty"`
	QuasarConfigVersion string          `json:"quasar_config_version,omitempty"`

	// StereopairInfo
	// Deprecated: use field Stereopair
	StereopairInfo            StereopairInfoItemViews `json:"stereopair_info,omitempty"`
	Stereopair                *StereopairView         `json:"stereopair,omitempty"`
	StereopairCreateAvailable bool                    `json:"stereopair_create_available,omitempty"`

	// Tandem
	Tandem TandemDeviceConfigureView `json:"tandem"`

	// Voiceprint
	Voiceprint *VoiceprintView `json:"voiceprint,omitempty"`
}

func (v *DeviceQuasarConfigureView) From(
	ctx context.Context,
	device model.Device,
	userDevices model.Devices,
	deviceInfos quasarconfig.DeviceInfos,
	stereopairs model.Stereopairs,
	voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs,
) {
	var quasarInfo QuasarInfo
	quasarInfo.FromCustomData(device.CustomData, device.Type)
	v.QuasarInfo = &quasarInfo

	if quasarDeviceInfo := deviceInfos.GetByDeviceID(device.ID); quasarDeviceInfo != nil {
		configBytes, _ := json.Marshal(quasarDeviceInfo.Config.Config)
		v.QuasarConfig = configBytes
		v.QuasarConfigVersion = quasarDeviceInfo.Config.Version
	}

	if stereopair, exist := stereopairs.GetByDeviceID(device.ID); exist {
		v.Stereopair = &StereopairView{}
		v.Stereopair.From(ctx, stereopair)
		v.StereopairInfo = v.Stereopair.Devices
	}

	v.Voiceprint = NewVoiceprintView(device, voiceprintDeviceConfigs)
	v.StereopairCreateAvailable = model.CanCreateStereopairWithDevice(device, userDevices, stereopairs)
	v.Tandem.From(ctx, device, userDevices, deviceInfos, stereopairs)
}
