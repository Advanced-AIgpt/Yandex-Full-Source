package megamind

import (
	"strings"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

// deprecated
func GetStartBroadcastDirective(timeout int32) *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_StartBroadcastDirective{
			StartBroadcastDirective: &scenarios.TStartBroadcastDirective{
				Name:      StartBroadcastDirectiveName,
				TimeoutMs: timeout,
			},
		},
	}
}

// deprecated
func GetStopBroadcastDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_StopBroadcastDirective{
			StopBroadcastDirective: &scenarios.TStopBroadcastDirective{
				Name: StopBroadcastDirectiveName,
			},
		},
	}
}

func GetOpenURIDirective(link string, clientInfoType ClientInfoType) *scenarios.TDirective {
	// IOT-1427: iotApp use different frontend package
	iotAppLink := strings.Replace(link, "https://yandex.ru/quasar", "https://yandex.ru/iot", 1)
	var uri string
	switch clientInfoType {
	case IotAppIOSClientInfoType:
		uri = sup.IoTIOSLinkWrapper(iotAppLink)
	case IotAppAndroidClientInfoType:
		uri = sup.IoTAndroidLinkWrapper(iotAppLink)
	default:
		uri = sup.SearchAppLinkWrapper(sup.QuasarLinkWrapper(link))
	}
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_OpenUriDirective{
			OpenUriDirective: &scenarios.TOpenUriDirective{
				Name: OpenURIDirectiveName,
				Uri:  uri,
			},
		},
	}
}

func GetIoTDiscoveryStartDirective(timeout int32, deviceType bmodel.DeviceType, ssid string) *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_IoTDiscoveryStartDirective{
			IoTDiscoveryStartDirective: &scenarios.TIoTDiscoveryStartDirective{
				Name:       IoTDiscoveryStartDirectiveName,
				TimeoutMs:  timeout,
				DeviceType: string(deviceType),
				SSID:       ssid,
			},
		},
	}
}

func GetIoTDiscoveryStopDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_IoTDiscoveryStopDirective{
			IoTDiscoveryStopDirective: &scenarios.TIoTDiscoveryStopDirective{
				Name: IoTDiscoveryStopDirectiveName,
			},
		},
	}
}

func GetIoTDiscoveryCredentialsDirective(ssid string, password string, cipher string, token string) *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_IoTDiscoveryCredentialsDirective{
			IoTDiscoveryCredentialsDirective: &scenarios.TIoTDiscoveryCredentialsDirective{
				Name:     IoTDiscoveryCredentialsDirectiveName,
				SSID:     ssid,
				Password: password,
				Token:    token,
				Cipher:   cipher,
			},
		},
	}
}
