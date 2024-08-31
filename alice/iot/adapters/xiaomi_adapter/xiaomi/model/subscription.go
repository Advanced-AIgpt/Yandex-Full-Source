package model

var (
	UserEventSubscriptionTaskName    = "UserEventSubscriptionTask"
	DeviceStatusSubscriptionTaskName = "DeviceStatusSubscriptionTask"
	PropertySubscriptionTaskName     = "PropertySubscriptionTask"
	EventSubscriptionTaskName        = "EventSubscriptionTask"
)

type DeviceStatusSubscriptionTaskPayload struct {
	DeviceID string
}

type PropertySubscriptionTaskPayload struct {
	DeviceID   string
	PropertyID string
	IsSplit    bool
	XiaomiType string
	Region     Region
}

type EventSubscriptionTaskPayload struct {
	DeviceID   string
	EventID    string
	IsSplit    bool
	XiaomiType string
	Region     Region
}
