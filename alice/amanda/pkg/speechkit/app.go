package speechkit

type App struct {
	AppID              string `json:"app_id,omitempty"`
	AppVersion         string `json:"app_version,omitempty"`
	ClientTime         string `json:"client_time,omitempty"`
	Language           string `json:"lang,omitempty"`
	OSVersion          string `json:"os_version,omitempty"`
	Platform           string `json:"platform,omitempty"`
	Timestamp          string `json:"timestamp,omitempty"`
	Timezone           string `json:"timezone,omitempty"`
	UUID               string `json:"uuid,omitempty"`
	UserAgent          string `json:"user_agent,omitempty"`
	DeviceID           string `json:"device_id,omitempty"`
	DeviceModel        string `json:"device_model,omitempty"`
	DeviceManufacturer string `json:"device_manufacturer,omitempty"`
	DeviceColor        string `json:"device_color,omitempty"`
}
