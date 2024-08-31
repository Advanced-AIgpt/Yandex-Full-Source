package settings

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/memento"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	mementoproto "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger log.Logger
	Client memento.IClient
}

func (c *Controller) UpdateUserSettings(ctx context.Context, user model.User, settings UserSettings) error {
	configs, err := c.extractConfigsFromSettings(ctx, settings)
	if err != nil {
		return xerrors.Errorf("failed to extract configs from settings: %w", err)
	}
	request := NewChangeUserObjectsRequest().WithConfigs(configs...).Build()
	if _, err := c.Client.UpdateUserObjects(ctx, user.Ticket, request); err != nil {
		return xerrors.Errorf("failed to send memento update objects request: %w", err)
	}
	return nil
}

func (c *Controller) GetUserSettings(ctx context.Context, user model.User) (UserSettings, error) {
	request := NewGetUserObjectsRequest().WithScenario(IoTScenarioIntent).WithConfigKeys(KnownMementoKeys...).Build()
	resp, err := c.Client.GetUserObjects(ctx, user.Ticket, request)
	if err != nil {
		return UserSettings{}, xerrors.Errorf("failed to send memento request during getting user config: %w", err)
	}
	extractedConfigs := make([]IUserConfig, 0, len(resp.UserConfigs))
	for _, configPair := range resp.UserConfigs {
		config := makeConfigByKey(configPair.Key)
		if config == nil {
			continue
		}
		err := config.From(configPair)
		if err != nil {
			return UserSettings{}, xerrors.Errorf("failed to get config %s from memento response: %w", configPair.Key.String(), err)
		}
		extractedConfigs = append(extractedConfigs, config)
	}
	return c.extractSettingsFromConfigs(ctx, extractedConfigs)
}

func (c *Controller) extractConfigsFromSettings(ctx context.Context, settings UserSettings) ([]*mementoproto.TConfigKeyAnyPair, error) {
	configs := makeConfigs()
	result := make([]*mementoproto.TConfigKeyAnyPair, 0, len(configs))
	for _, config := range configs {
		ok, err := config.ExtractFrom(settings)
		if err != nil {
			return nil, xerrors.Errorf("failed to build config with key %s: %w", config.Key().String(), err)
		}
		if !ok {
			continue
		}
		build, err := config.Build()
		if err != nil {
			return nil, xerrors.Errorf("failed to build config with key %s: %w", config.Key().String(), err)
		}
		result = append(result, build)
	}
	return result, nil
}

func (c *Controller) extractSettingsFromConfigs(ctx context.Context, configs []IUserConfig) (UserSettings, error) {
	var result UserSettings
	for _, config := range configs {
		settings, err := config.ExtractTo(result)
		if err != nil {
			return UserSettings{}, xerrors.Errorf("failed to extract data from config %s to user settings: %w", config.Key().String(), err)
		}
		result = settings
	}
	return result, nil
}

func (c *Controller) ExtractUserSettingsFromRequest(ctx context.Context, baseRequest *scenarios.TScenarioBaseRequest) (UserSettings, error) {
	configs := make([]IUserConfig, 0)
	if responseOptions := baseRequest.GetMemento().GetUserConfigs().GetIoTResponseOptions(); responseOptions != nil {
		configs = append(configs, &IoTResponseOptionsConfig{TIoTResponseOptions: responseOptions})
	}
	if whisperConfig := baseRequest.GetMemento().GetUserConfigs().GetTtsWhisperConfig(); whisperConfig != nil {
		configs = append(configs, &TTSWhisperConfig{TTtsWhisperConfig: whisperConfig})
	}
	if musicConfig := baseRequest.GetMemento().GetUserConfigs().GetMusicConfig(); musicConfig != nil {
		configs = append(configs, &MusicConfig{TUserConfig: musicConfig})
	}
	if orderStatusConfig := baseRequest.GetMemento().GetUserConfigs().GetOrderStatusConfig(); orderStatusConfig != nil {
		configs = append(configs, &OrderStatusConfig{TOrderStatusUserConfig: orderStatusConfig})
	}
	return c.extractSettingsFromConfigs(ctx, configs)
}

func (c *Controller) VoiceprintDeviceConfigs(ctx context.Context, user model.User, devices model.Devices) (VoiceprintDeviceConfigs, error) {
	quasarExternalIDMap := devices.ToQuasarExternalIDMap()
	mementoDeviceKeys := make([]*mementoproto.TDeviceKeys, 0, len(quasarExternalIDMap))
	for quasarExternalID := range quasarExternalIDMap {
		mementoDeviceKeys = append(mementoDeviceKeys, &mementoproto.TDeviceKeys{
			DeviceId: quasarExternalID,
			Keys: []mementoproto.EDeviceConfigKey{
				mementoproto.EDeviceConfigKey_DCK_PERSONALIZATION_DATA,
			},
		})
	}
	request := NewGetUserObjectsRequest().WithDevicesKeys(mementoDeviceKeys...).Build()
	resp, err := c.Client.GetUserObjects(ctx, user.Ticket, request)
	if err != nil {
		return nil, xerrors.Errorf("failed to send memento request during getting voiceprint device config: %w", err)
	}
	results := make(VoiceprintDeviceConfigs, 0, len(quasarExternalIDMap))
	for _, deviceConfig := range resp.DevicesConfigs {
		device, exist := quasarExternalIDMap[deviceConfig.DeviceId]
		if !exist {
			continue
		}
		for _, configPair := range deviceConfig.DeviceConfigs {
			if configPair.Key != mementoproto.EDeviceConfigKey_DCK_PERSONALIZATION_DATA {
				continue
			}
			var config VoiceprintDeviceConfig
			if err := config.From(configPair, device); err != nil {
				return nil, xerrors.Errorf("failed to get voiceprint device config from memento config pair: %w", err)
			}
			results = append(results, config)
		}
	}

	return results, nil
}

func makeConfigs() []IUserConfig {
	return []IUserConfig{
		NewIoTResponseOptionsConfig(),
		NewTTSWhisperConfig(),
		NewMusicConfig(),
		NewOrderStatusConfig(),
	}
}

func makeConfigByKey(configKey mementoproto.EConfigKey) IUserConfig {
	switch configKey {
	case mementoproto.EConfigKey_CK_IOT_RESPONSE_OPTIONS:
		return NewIoTResponseOptionsConfig()
	case mementoproto.EConfigKey_CK_TTS_WHISPER:
		return NewTTSWhisperConfig()
	case mementoproto.EConfigKey_CK_MUSIC:
		return NewMusicConfig()
	case mementoproto.EConfigKey_CK_ORDER_STATUS:
		return NewOrderStatusConfig()
	default:
		return nil
	}
}
