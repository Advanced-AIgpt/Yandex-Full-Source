package mobile

import (
	"regexp"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var russianNumericRegex = regexp.MustCompile(`[^а-яёА-ЯЁ0-9\s]+`)

var KnownIRCategories = map[string]IRCategory{
	string(tuya.TvIrCategoryID):        {ID: string(tuya.TvIrCategoryID), Name: tuya.TvIrCategoryName, Type: model.TvDeviceDeviceType},
	string(tuya.AcIrCategoryID):        {ID: string(tuya.AcIrCategoryID), Name: tuya.AcIrCategoryName, Type: model.AcDeviceType},
	string(tuya.SetTopBoxIrCategoryID): {ID: string(tuya.SetTopBoxIrCategoryID), Name: tuya.SetTopBoxIrCategoryName, Type: model.ReceiverDeviceType},
	string(tuya.BoxIrCategoryID):       {ID: string(tuya.BoxIrCategoryID), Name: tuya.BoxIrCategoryName, Type: model.TvBoxDeviceType},
}

var CustomIRCategory = IRCategory{ID: string(tuya.CustomIrCategoryID), Name: tuya.CustomIrCategoryName, Type: model.OtherDeviceType}

type FirmwareUpgradeStatus string

const (
	FirmwareActual            FirmwareUpgradeStatus = "ACTUAL"
	FirmwareUpgradeIsReady    FirmwareUpgradeStatus = "UPGRADE_READY"
	FirmwareUpgradeInProgress FirmwareUpgradeStatus = "UPGRADE_IN_PROGRESS"
	FirmwareUpgradeComplete   FirmwareUpgradeStatus = "UPGRADE_IS_COMPLETE"
	FirmwareUpgradeException  FirmwareUpgradeStatus = "UPGRADE_EXCEPTION"
	FirmwareUnknown           FirmwareUpgradeStatus = "UNKNOWN"
)

var KnownFirmwareUpgradeStatuses = map[tuya.FirmwareUpgradeStatus]FirmwareUpgradeStatus{
	tuya.FirmwareNoNeedUpgradeStatus:    FirmwareActual,
	tuya.FirmwareHardwareIsReadyStatus:  FirmwareUpgradeIsReady,
	tuya.FirmwareUpgradingStatus:        FirmwareUpgradeInProgress,
	tuya.FirmwareUpgradeCompleteStatus:  FirmwareUpgradeComplete,
	tuya.FirmwareUpgradeExceptionStatus: FirmwareUpgradeException,
}

const (
	IRCustomButtonExistingNameValidator string = "ir_custom_button_existing_name_validator"
)

var SuggestionsForIRCustomControls = []string{"Вентилятор", "Проектор", "Обогреватель", "Телевизор", "Приставка", "Кондиционер", "Люстра"}
var SuggestionsForIRCustomButtons = []string{"Включи", "Выключи", "Сделай ярче", "Смени режим", "Смени вход", "Включи на максимум", "Ночной режим", "Сделай теплее"}
