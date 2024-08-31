package callback

import (
	"encoding/json"
	"fmt"
	"regexp"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
)

type DiscoveryRequest struct {
	Timestamp timestamp.PastTimestamp `json:"ts"`
	Payload   *DiscoveryPayload       `json:"payload" valid:"notNil"`
}

type IDiscoveryFilter interface {
	// returns bool, string - whether device should be filtered or not. If it should, then also returns msg why
	FilterOut(device model.Device) (bool, string)
}

type DiscoveryDeviceTypeFilter struct {
	DeviceTypes []string `json:"device_types,omitempty"`
}

func (df DiscoveryDeviceTypeFilter) FilterOut(device model.Device) (bool, string) {
	if !tools.Contains(string(device.Type), df.DeviceTypes) {
		return true, fmt.Sprintf("device type %s is not presented in allowed for this discovery device types", device.Type)
	}
	return false, ""
}

type DiscoveryDefaultFilter struct{}

func (df DiscoveryDefaultFilter) FilterOut(device model.Device) (bool, string) {
	return false, ""
}

type DiscoveryDeviceIDFilter struct {
	DeviceIDs []string `json:"device_ids"`
}

func (df DiscoveryDeviceIDFilter) FilterOut(device model.Device) (bool, string) {
	if !tools.Contains(device.ExternalID, df.DeviceIDs) {
		return true, fmt.Sprintf("device %s is not presented in allowed for this discovery devices id", device.ExternalID)
	}
	return false, ""
}

type DiscoveryPayload struct {
	Filter         IDiscoveryFilter    `json:"filter,omitempty"`
	FilterType     DiscoveryFilterType `json:"filter_type"`
	ExternalUserID string              `json:"user_id"`
}

func (dp *DiscoveryPayload) UnmarshalJSON(b []byte) error {
	var commonFields struct {
		FilterType     DiscoveryFilterType `json:"filter_type"`
		ExternalUserID string              `json:"user_id"`
	}

	if err := json.Unmarshal(b, &commonFields); err != nil {
		return err
	}
	dp.FilterType = commonFields.FilterType
	dp.ExternalUserID = commonFields.ExternalUserID

	switch dp.FilterType {
	case DiscoveryDeviceTypeFilterType:
		var specificFields struct {
			Filter *DiscoveryDeviceTypeFilter `json:"filter"`
		}
		if err := json.Unmarshal(b, &specificFields); err != nil {
			return err
		}
		if specificFields.Filter != nil {
			dp.Filter = specificFields.Filter
		}
	case DiscoveryDeviceIDFilterType:
		var specificFields struct {
			Filter *DiscoveryDeviceIDFilter `json:"filter"`
		}
		if err := json.Unmarshal(b, &specificFields); err != nil {
			return err
		}
		if specificFields.Filter != nil {
			dp.Filter = specificFields.Filter
		}
	default:
		dp.Filter = DiscoveryDefaultFilter{}
		dp.FilterType = DiscoveryDefaultFilterType
	}
	return nil
}

var dpRenaming = []struct {
	re      *regexp.Regexp
	replace string
}{
	{
		regexp.MustCompile(`^\.Timestamp$`),
		"ts",
	},
	{
		regexp.MustCompile(`^\.Payload$`),
		"payload",
	},
	{
		regexp.MustCompile(`^Payload\.FilterType$`),
		"payload.filter_type",
	},
	{
		regexp.MustCompile(`^Payload\.UserID$`),
		"payload.user_id",
	},
}

func (dr *DiscoveryRequest) MapFieldNameToJSONAddress(fieldName string) string {
	for _, rule := range dpRenaming {
		if res := rule.re.ReplaceAllString(fieldName, rule.replace); res != fieldName {
			return res
		}
	}
	return ""
}
