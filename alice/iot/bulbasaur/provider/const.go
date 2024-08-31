package provider

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

type HandlerConfig struct {
	Method string
	Route  string
}

type Handler string

const UserAgentValue string = "Yandex LLC"

const (
	AvailabilityHandler  Handler = "availability"
	UserUnlinkHandler    Handler = "user_unlink"
	DevicesHandler       Handler = "devices"
	DevicesQueryHandler  Handler = "devices_query"
	DevicesActionHandler Handler = "devices_action"
)

var HandlersToConfig = map[Handler]HandlerConfig{
	AvailabilityHandler:  {Method: http.MethodHead, Route: "v1.0"},
	UserUnlinkHandler:    {Method: http.MethodPost, Route: "v1.0/user/unlink"},
	DevicesHandler:       {Method: http.MethodGet, Route: "v1.0/user/devices"},
	DevicesQueryHandler:  {Method: http.MethodPost, Route: "v1.0/user/devices/query"},
	DevicesActionHandler: {Method: http.MethodPost, Route: "v1.0/user/devices/action"},
}

type KnownHandlerErrorCodes map[Handler]map[adapter.ErrorCode]struct{}

func (khe KnownHandlerErrorCodes) IsKnownErrorCode(handler Handler, errorCode adapter.ErrorCode) bool {
	if handlerErrorCodes, isKnownHandler := khe[handler]; isKnownHandler {
		if _, isKnownErrorCode := handlerErrorCodes[errorCode]; isKnownErrorCode {
			return true
		}
	}
	return false
}

var SupportedHandlerErrorCodes = KnownHandlerErrorCodes{
	AvailabilityHandler: {},
	UserUnlinkHandler:   {},
	DevicesHandler:      {},
	DevicesQueryHandler: {
		adapter.DeviceUnreachable: struct{}{},
		adapter.DeviceBusy:        struct{}{},
		adapter.DeviceNotFound:    struct{}{},
		adapter.InternalError:     struct{}{},
	},
	DevicesActionHandler: {
		adapter.DeviceUnreachable:         struct{}{},
		adapter.DeviceBusy:                struct{}{},
		adapter.DeviceNotFound:            struct{}{},
		adapter.InvalidAction:             struct{}{},
		adapter.InvalidValue:              struct{}{},
		adapter.InternalError:             struct{}{},
		adapter.NotSupportedInCurrentMode: struct{}{},
		adapter.DoorOpen:                  struct{}{},
		adapter.LidOpen:                   struct{}{},
		adapter.RemoteControlDisabled:     struct{}{},
		adapter.NotEnoughWater:            struct{}{},
		adapter.LowChargeLevel:            struct{}{},
		adapter.ContainerFull:             struct{}{},
		adapter.ContainerEmpty:            struct{}{},
		adapter.DripTrayFull:              struct{}{},
		adapter.DeviceStuck:               struct{}{},
		adapter.DeviceOff:                 struct{}{},
		adapter.FirmwareOutOfDate:         struct{}{},
		adapter.NotEnoughDetergent:        struct{}{},
		adapter.AccountLinkingError:       struct{}{},
		adapter.HumanInvolvementNeeded:    struct{}{},
	},
}

type ServiceKey string

const (
	QuasarServiceKey ServiceKey = "QUASAR_HOST"
)

type HostURL string

const (
	QuasarProductionURL HostURL = "quasar.yandex.ru"
	QuasarTestingURL    HostURL = "testing.quasar.yandex.ru"

	RemoteCarStableURL        HostURL = "auto-remote-access-server.maps.yandex.net"
	RemoteCarPrestableURL     HostURL = "auto-remote-access-server.prestable.maps.yandex.net"
	RemoteCarFakePrestableURL HostURL = "auto-remote-access-server.fakeprestable.maps.yandex.net"
)

type HostTVMAlias string

const (
	// aliases from misc/tvm.sh
	QuasarProductionTVMAlias HostTVMAlias = "quasar-backend"
	QuasarTestingTVMAlias    HostTVMAlias = "quasar-backend-test"

	RemoteCarStableTVMAlias        HostTVMAlias = "remotecar"
	RemoteCarPrestableTVMAlias     HostTVMAlias = "remotecar-prestable"
	RemoteCarFakePrestableTVMAlias HostTVMAlias = "remotecar-fake-prestable"

	CloudFunctionsTVMAlias HostTVMAlias = "cloud"
)

var KnownQuasarProviderHosts map[HostURL]HostTVMAlias
var KnownRemoteCarProviderHosts map[HostURL]HostTVMAlias

var KnownErrorCodesToSolomonCommandStatus map[adapter.ErrorCode]string

func init() {
	KnownQuasarProviderHosts = map[HostURL]HostTVMAlias{
		QuasarProductionURL: QuasarProductionTVMAlias,
		QuasarTestingURL:    QuasarTestingTVMAlias,
	}

	KnownRemoteCarProviderHosts = map[HostURL]HostTVMAlias{
		RemoteCarStableURL:        RemoteCarStableTVMAlias,
		RemoteCarPrestableURL:     RemoteCarPrestableTVMAlias,
		RemoteCarFakePrestableURL: RemoteCarFakePrestableTVMAlias,
	}

	KnownErrorCodesToSolomonCommandStatus = map[adapter.ErrorCode]string{
		adapter.DeviceUnreachable:         "device_unreachable",
		adapter.DeviceBusy:                "device_busy",
		adapter.DeviceNotFound:            "device_not_found",
		adapter.InternalError:             "internal_error",
		adapter.InvalidAction:             "invalid_action",
		adapter.InvalidValue:              "invalid_value",
		adapter.NotSupportedInCurrentMode: "not_supported_in_current_mode",
		adapter.DoorOpen:                  "door_open",
		adapter.LidOpen:                   "lid_open",
		adapter.RemoteControlDisabled:     "remote_control_disabled",
		adapter.NotEnoughWater:            "not_enough_water",
		adapter.LowChargeLevel:            "low_charge_level",
		adapter.ContainerFull:             "container_full",
		adapter.ContainerEmpty:            "container_empty",
		adapter.DripTrayFull:              "drip_tray_full",
		adapter.DeviceStuck:               "device_stuck",
		adapter.DeviceOff:                 "device_off",
		adapter.FirmwareOutOfDate:         "firmware_out_of_date",
		adapter.NotEnoughDetergent:        "not_enough_detergent",
		adapter.AccountLinkingError:       "account_linking_error",
		adapter.HumanInvolvementNeeded:    "human_involvement_needed",
	}
}

const QuasarProviderActionDelayMs = 500
