package tuya

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	btuya "a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
)

type FirmwareChangelogID string

const (
	Lamp3To5AndScenesFirmwareUpgradeID FirmwareChangelogID = "lamp_3_to_5_and_scenes"
)

type FirmwareChangelogs map[TuyaDeviceProductID]versionChangelog

type versionChangelog struct {
	version   int
	changelog FirmwareChangelogID
}

func (fc FirmwareChangelogs) GetAvailableChangelog(ctx context.Context, customData interface{}, fwVersion *string) *FirmwareChangelogID {
	parsedCustomData, err := btuya.ParseCustomData(customData)
	if err != nil {
		return nil
	}
	if parsedCustomData.ProductID == nil {
		return nil
	}
	if fwVersion == nil {
		return nil
	}
	intVersion, err := GetLastNumberOfFirmwareVersion(*fwVersion)
	if err != nil {
		return nil
	}
	if fc == nil {
		return nil
	}
	productID := TuyaDeviceProductID(*parsedCustomData.ProductID)
	switch productID {
	// IOT-1746: E27 lemon 2 requires its own flag
	case LampE27Lemon2YandexProductID:
		if !experiments.ShowAvailableChangelogE27Lemon2.IsEnabled(ctx) {
			return nil
		}
	default:
		if !experiments.ShowAvailableChangelog.IsEnabled(ctx) {
			return nil
		}
	}
	if versionChangelog, exist := fc[TuyaDeviceProductID(*parsedCustomData.ProductID)]; exist {
		if intVersion < int64(versionChangelog.version) {
			return &versionChangelog.changelog
		}
	}
	return nil
}

var KnownFirmwareChangelogs = FirmwareChangelogs{
	LampE14MPYandexProductID: {
		version:   18,
		changelog: Lamp3To5AndScenesFirmwareUpgradeID,
	},
	LampGU10MPYandexProductID: {
		version:   7,
		changelog: Lamp3To5AndScenesFirmwareUpgradeID,
	},
	LampE27Lemon2YandexProductID: {
		version:   20,
		changelog: Lamp3To5AndScenesFirmwareUpgradeID,
	},
	LampE27Lemon3YandexProductID: {
		version:   8,
		changelog: Lamp3To5AndScenesFirmwareUpgradeID,
	},
}
