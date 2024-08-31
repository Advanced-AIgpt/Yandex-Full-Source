package model

type ErrorCode string

const (
	PollingTimeout               ErrorCode = "POLLING_TIMEOUT"
	DeviceStateNotFound          ErrorCode = "DEVICE_STATE_NOT_FOUND"
	ConnectingDeviceTypeNotFound ErrorCode = "CONNECTING_DEVICE_TYPE_NOT_FOUND"
)

type IVulpixError interface {
	error
	ErrorCode() ErrorCode
}

type ErrPollingTimeout struct{}

func (ept ErrPollingTimeout) Error() string {
	return string(PollingTimeout)
}

func (ept ErrPollingTimeout) ErrorCode() ErrorCode {
	return PollingTimeout
}

type ErrDeviceStateNotFound struct{}

func (edsnf ErrDeviceStateNotFound) Error() string {
	return string(DeviceStateNotFound)
}

func (edsnf ErrDeviceStateNotFound) ErrorCode() ErrorCode {
	return DeviceStateNotFound
}

type ErrConnectingDeviceTypeNotFound struct{}

func (e ErrConnectingDeviceTypeNotFound) Error() string {
	return string(ConnectingDeviceTypeNotFound)
}

func (e ErrConnectingDeviceTypeNotFound) ErrorCode() ErrorCode {
	return ConnectingDeviceTypeNotFound
}
