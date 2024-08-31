package iotapi

import (
	"fmt"
	"math"
	"net/http"
	"strings"
)

type Status int64

func (s Status) IsError() bool {
	return s < 0
}

func (s Status) XiaomiError() Error {
	if !s.IsError() {
		return Error{}
	}
	// error looks like -70xxxyzzz
	rawStatus := int(math.Abs(float64(s)))   // becomes 70xxxyzzz
	errorCode := rawStatus % 1000            // zzz part
	errorLocation := (rawStatus / 1000) % 10 // y part
	httpCode := (rawStatus / 10000) % 1000   // xxx part
	return Error{
		HTTPCode:  httpCode,
		location:  errorLocation,
		errorCode: errorCode,
	}
}

func (s Status) Error() error {
	if !s.IsError() {
		return nil
	}
	return s.XiaomiError()
}

type Error struct {
	HTTPCode  int
	location  int
	errorCode int
}

func (e Error) Location() string {
	switch e.location {
	case 0:
		return "client"
	case 1:
		return "open platform"
	case 2:
		return "device cloud"
	case 3:
		return "device"
	case 4:
		return "miot-spec"
	default:
		return "unknown"
	}
}

type ErrorCode string

const (
	DeviceDoesNotExistErrorCode             ErrorCode = "device does not exist"
	ServiceDoesNotExistErrorCode            ErrorCode = "service does not exist"
	PropertyDoesNotExistErrorCode           ErrorCode = "property does not exist"
	EventDoesNotExistErrorCode              ErrorCode = "event does not exist"
	ActionDoesNotExistErrorCode             ErrorCode = "action does not exist"
	DeviceDescriptionNotFoundErrorCode      ErrorCode = "device description not found"
	DeviceCloudNotFoundErrorCode            ErrorCode = "device cloud not found"
	InvalidIdentifierErrorCode              ErrorCode = "invalid id (pid/sid/aid/eid/...)"
	SceneDoesNotExistErrorCode              ErrorCode = "scene does not exist"
	DeviceOfflineErrorCode                  ErrorCode = "device offline"
	PropertyIsNotReadableErrorCode          ErrorCode = "property is not readable"
	PropertyIsNotWritableErrorCode          ErrorCode = "property is not writable"
	PropertyIsNotSubscribableErrorCode      ErrorCode = "property is not subscribable"
	PropertyValueErrorErrorCode             ErrorCode = "property value error"
	ActionValueErrorErrorCode               ErrorCode = "action value error"
	ActionExecutionErrorErrorCode           ErrorCode = "action execution error"
	ActionParametersNumberMismatchErrorCode ErrorCode = "number of action parameters does not match"
	ActionParameterErrorErrorCode           ErrorCode = "action parameter error"
	OperationTimeoutErrorCode               ErrorCode = "device operation timed out"
	NotSupportedInCurrentStateErrorCode     ErrorCode = "device cannot perform this operation in its current state"
	NotSupportedForInfraredDeviceErrorCode  ErrorCode = "infrared device does not support this operation"
	TokenExpiredErrorCode                   ErrorCode = "token does not exist or is expired"
	TokenIllegalErrorCode                   ErrorCode = "token illegal"
	AuthorizationExpiredErrorCode           ErrorCode = "authorization expired"
	UnauthorizedVoiceDeviceErrorCode        ErrorCode = "unauthorized voice device"
	DeviceNotBoundErrorCode                 ErrorCode = "device is not bound"
	FeatureNotOnlineErrorCode               ErrorCode = "feature not online"
)

func (e Error) ErrorCode() ErrorCode {
	switch e.errorCode {
	case 1:
		return DeviceDoesNotExistErrorCode
	case 2:
		return ServiceDoesNotExistErrorCode
	case 3:
		return PropertyDoesNotExistErrorCode
	case 4:
		return EventDoesNotExistErrorCode
	case 5:
		return ActionDoesNotExistErrorCode
	case 6:
		return DeviceDescriptionNotFoundErrorCode
	case 7:
		return DeviceCloudNotFoundErrorCode
	case 8:
		return InvalidIdentifierErrorCode
	case 9:
		return SceneDoesNotExistErrorCode
	case 11:
		return DeviceOfflineErrorCode
	case 13:
		return PropertyIsNotReadableErrorCode
	case 23:
		return PropertyIsNotWritableErrorCode
	case 33:
		return PropertyIsNotSubscribableErrorCode
	case 43:
		return PropertyValueErrorErrorCode
	case 34:
		return ActionValueErrorErrorCode
	case 15:
		return ActionExecutionErrorErrorCode
	case 25:
		return ActionParametersNumberMismatchErrorCode
	case 35:
		return ActionParameterErrorErrorCode
	case 36:
		return OperationTimeoutErrorCode
	case 100:
		return NotSupportedInCurrentStateErrorCode
	case 101:
		return NotSupportedForInfraredDeviceErrorCode
	case 901:
		return TokenExpiredErrorCode
	case 902:
		return TokenIllegalErrorCode
	case 903:
		return AuthorizationExpiredErrorCode
	case 904:
		return UnauthorizedVoiceDeviceErrorCode
	case 905:
		return DeviceNotBoundErrorCode
	case 999:
		return FeatureNotOnlineErrorCode
	default:
		return "unknown"
	}
}

func (e Error) Error() string {
	return fmt.Sprintf(
		"xiaomi error: http code %d:%s, location %d:%s, error code %d:%s",
		e.HTTPCode, http.StatusText(e.HTTPCode),
		e.location, e.Location(),
		e.errorCode, e.ErrorCode(),
	)
}

type Device struct {
	DID                 string
	Type                string
	Name                string
	IsShared            bool `json:"is_shared"`
	Category            string
	CloudID             int    `json:"cloud_id"`
	LastUpdateTimestamp uint64 `json:"last_update_timestamp"`
	RID                 string `json:"rid"`
	CommonMark          bool   `json:"common_mark"`
	ReadDID             string `json:"real_did"`
	Mac                 string
}

type Devices []Device

func (d Devices) Exists(deviceID string) bool {
	for _, device := range d {
		if device.DID == deviceID {
			return true
		}
	}
	return false
}

type DeviceInfo struct {
	ID           string
	Name         string
	IsShared     bool   `json:"is_shared"`
	CloudID      int    `json:"cloud_id"`
	RID          string `json:"rid"`
	Mac          string
	Online       bool   `json:"online"`
	CurrentFwVer string `json:"current_fw_ver"`
	LatestFwVer  string `json:"latest_fw_ver"`
	SerialNumber string `json:"serialNumber"`
}

type Home struct {
	ID    uint64
	Name  string
	Rooms []Room
}

type Room struct {
	ID   string
	Name string
}

type Property struct {
	Pid         string      `json:"pid"`
	Status      Status      `json:"status,omitempty"`
	Description string      `json:"description,omitempty"`
	Value       interface{} `json:"value"`
	IsSplit     bool        `json:"-"`
}

func (p *Property) GetDeviceExternalID() string { // FIXME: this can break
	if p.IsSplit {
		return strings.Join(strings.Split(p.Pid, ".")[:2], ".")
	}
	return strings.Split(p.Pid, ".")[0]
}

type Event struct {
	Eid     string `json:"eid"`
	Status  Status `json:"status,omitempty"`
	IsSplit bool   `json:"-"`
}

func (e *Event) IsStatusSuccess() bool {
	return e.Status == 0
}

func (e *Event) GetDeviceExternalID() string { // FIXME: this can break
	if e.IsSplit {
		return strings.Join(strings.Split(e.Eid, ".")[:2], ".")
	}
	return strings.Split(e.Eid, ".")[0]
}

type Action struct {
	Aid    string        `json:"aid"`
	Oid    string        `json:"aoid,omitempty"`
	Out    []interface{} `json:"out,omitempty"`
	In     []interface{} `json:"int,omitempty"`
	Status Status        `json:"status,omitempty"`
}

func (action *Action) GetDeviceExternalID() string { // FIXME: this can break
	return strings.Split(action.Aid, ".")[0] // FIXME: isSplit actions?
}

type Region string

func (r Region) String() string {
	return string(r)
}

const (
	ChinaRegion     Region = "china"
	RussiaRegion    Region = "russia"
	EuropeRegion    Region = "europe"
	SingaporeRegion Region = "singapore"
	USWestRegion    Region = "us_west"
)

func getRegionBaseURL(region Region) string {
	switch region {
	case ChinaRegion:
		return "api.home.mi.com"
	case RussiaRegion:
		return "ru.api.home.mi.com"
	case EuropeRegion:
		return "de.api.home.mi.com"
	case SingaporeRegion:
		return "sg.api.home.mi.com"
	case USWestRegion:
		return "us.api.home.mi.com"
	default:
		panic(fmt.Sprintf("failed to get region base url: unknown region %s", region))
	}
}

type Topic string

const (
	PropertiesChangedTopic Topic = "properties-changed"
	EventOccurredTopic     Topic = "event-occured"
	UserEventTopic         Topic = "user-event"
)

type SubscriptionType string

const (
	HTTPSubscriptionType SubscriptionType = "http"
)

// properties-changed topic
type PropertiesChangedCustomData struct {
	SubscriptionKey string `json:"subscription_key"`
	UserID          string `json:"user_id"`
	DeviceID        string `json:"device_id"`
	Type            string `json:"type"`
	Region          Region `json:"region"`
	IsSplit         bool   `json:"is_split"`
}

type EventOccurredCustomData struct {
	SubscriptionKey string `json:"subscription_key"`
	UserID          string `json:"user_id"`
	DeviceID        string `json:"device_id"`
	Type            string `json:"type"`
	Region          Region `json:"region"`
	IsSplit         bool   `json:"is_split"`
}

type PropertiesChangedCallback struct {
	PropertyStates []Property                  `json:"properties"`
	CustomData     PropertiesChangedCustomData `json:"custom-data"`
}

type SubscribeToPropertyChangesRequest struct {
	Topic       Topic                       `json:"topic"`
	PropertyIDS []string                    `json:"properties"`
	CustomData  PropertiesChangedCustomData `json:"custom-data"`
	Type        SubscriptionType            `json:"type"`
	PushID      string                      `json:"push-id"`
	ReceiverURL string                      `json:"receiver-url"`
}

type SubscribeToPropertiesChangedResult struct {
	Properties []Property `json:"properties"`
}

type EventOccurredCallback struct {
	Events     []Event                 `json:"events"`
	CustomData EventOccurredCustomData `json:"custom-data"`
}

type SubscribeToDeviceEventsRequest struct {
	Topic       Topic                   `json:"topic"`
	EventIDs    []string                `json:"events"`
	CustomData  EventOccurredCustomData `json:"custom-data"`
	Type        SubscriptionType        `json:"type"`
	PushID      string                  `json:"push-id"`
	ReceiverURL string                  `json:"receiver-url"`
}

type SubscribeToDeviceEventsResult struct {
	Events []Event `json:"events"`
}

// user-event topic
type UserEventCustomData struct {
	SubscriptionKey string `json:"subscription_key"`
	UserID          string `json:"user_id"`
	Region          Region `json:"region"`
}

type UserEventOperation string

const (
	AddDeviceOperation    UserEventOperation = "add-device"
	RemoveDeviceOperation UserEventOperation = "remove-device"
)

type UserEvent struct {
	Operation  UserEventOperation `json:"operation"`
	DeviceID   string             `json:"did"`
	DeviceName string             `json:"name"`
	DeviceType string             `json:"type"`
}

type UserEventCallback struct {
	Event      UserEvent           `json:"event"`
	CustomData UserEventCustomData `json:"custom-data"`
}

type SubscribeToUserEventsRequest struct {
	Topic       Topic               `json:"topic"`
	CustomData  UserEventCustomData `json:"custom-data"`
	Type        SubscriptionType    `json:"type"`
	PushID      string              `json:"push-id"`
	ReceiverURL string              `json:"receiver-url"`
}
