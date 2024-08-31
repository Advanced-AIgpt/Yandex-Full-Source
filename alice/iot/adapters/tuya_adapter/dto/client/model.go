package client

import (
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

type GetDevicesUnderPairingTokenResponse struct {
	Status         string                   `json:"status"`
	RequestID      string                   `json:"request_id"`
	SuccessDevices DevicesUnderPairingToken `json:"success_devices"`
	ErrorDevices   DevicesUnderPairingToken `json:"error_devices"`
}

type DeviceUnderPairingToken struct {
	ID        string `json:"id"`
	IP        string `json:"ip"`
	Name      string `json:"name"`
	ProductID string `json:"productId"`
	UUID      string `json:"uuid"`
}

func (dupt *DeviceUnderPairingToken) FromDeviceUnderPairingToken(device tuya.DeviceUnderPairingToken) {
	dupt.ID = device.ID
	dupt.IP = device.IP
	dupt.Name = device.Name
	dupt.ProductID = device.ProductID
	dupt.UUID = device.UUID
}

type DevicesUnderPairingToken []DeviceUnderPairingToken

func (devices DevicesUnderPairingToken) GetIDs() []string {
	deviceIDs := make([]string, 0, len(devices))
	for _, device := range devices {
		deviceIDs = append(deviceIDs, device.ID)
	}
	return deviceIDs
}

func (gduptr *GetDevicesUnderPairingTokenResponse) FromSuccessAndErrorDevices(successDevices []tuya.DeviceUnderPairingToken, errorDevices []tuya.DeviceUnderPairingToken) {
	gduptr.SuccessDevices = make([]DeviceUnderPairingToken, 0, len(successDevices))
	for _, device := range successDevices {
		var dupt DeviceUnderPairingToken
		dupt.FromDeviceUnderPairingToken(device)
		gduptr.SuccessDevices = append(gduptr.SuccessDevices, dupt)
	}
	gduptr.ErrorDevices = make([]DeviceUnderPairingToken, 0, len(errorDevices))
	for _, device := range errorDevices {
		var dupt DeviceUnderPairingToken
		dupt.FromDeviceUnderPairingToken(device)
		gduptr.ErrorDevices = append(gduptr.ErrorDevices, dupt)
	}
}

type GetDevicesDiscoveryInfoRequest struct {
	DevicesID []string `json:"devices_id"`
}

type GetDevicesDiscoveryInfoResponse struct {
	Status      string                   `json:"status"`
	RequestID   string                   `json:"request_id"`
	DevicesInfo []adapter.DeviceInfoView `json:"devices_info"`
	UserID      string                   `json:"user_id"`
}

func (r GetDevicesDiscoveryInfoResponse) ToAdapterDiscoveryPayload() adapter.DiscoveryPayload {
	return adapter.DiscoveryPayload{
		UserID:  r.UserID,
		Devices: r.DevicesInfo,
	}
}

type GetTokenRequest struct {
	SSID           string                  `json:"ssid"`
	Password       string                  `json:"password"`
	ConnectionType tuya.WiFiConnectionType `json:"connection_type"`
}

type GetTokenResponse struct {
	Status    string    `json:"status"`
	RequestID string    `json:"request_id"`
	TokenInfo TokenInfo `json:"token_info"`
}

type TokenInfo struct {
	Region string `json:"region"`
	Token  string `json:"token"`
	Secret string `json:"secret"`
	Cipher string `json:"cipher"`
}

func (ti TokenInfo) GetPairingToken() string {
	return ti.Region + ti.Token + ti.Secret
}
