package sup

import (
	"fmt"
	"net/url"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/sup"
)

type AppLinksGenerator struct {
	BulbasaurURL string
}

func (g *AppLinksGenerator) BuildDevicePageLink(deviceID string) string {
	deviceURL := fmt.Sprintf("https://yandex.ru/quasar/iot/device/%s", deviceID)
	if g.BulbasaurURL != "" {
		deviceURL = appendSrcrwr(deviceURL, g.BulbasaurURL)
	}
	return deviceURL
}

func (g *AppLinksGenerator) BuildDeviceListPageLink() string {
	devicesURL := "https://yandex.ru/quasar/iot"
	if g.BulbasaurURL != "" {
		devicesURL = appendSrcrwr(devicesURL, g.BulbasaurURL)
	}
	return devicesURL
}

func (g *AppLinksGenerator) BuildScenarioPageLink(scenarioID string) string {
	scenarioURL := fmt.Sprintf("https://yandex.ru/quasar/iot/scenario/%s", scenarioID)
	if g.BulbasaurURL != "" {
		scenarioURL = appendSrcrwr(scenarioURL, g.BulbasaurURL)
	}
	return scenarioURL
}

func (g *AppLinksGenerator) BuildScenarioLaunchPageLink(launchID string) string {
	scenarioLaunchURL := fmt.Sprintf("https://yandex.ru/quasar/external/show-scenario-launch?launchId=%s", launchID)
	if g.BulbasaurURL != "" {
		scenarioLaunchURL = appendSrcrwr(scenarioLaunchURL, g.BulbasaurURL)
	}
	return scenarioLaunchURL
}

func (g *AppLinksGenerator) BuildScenarioListPageLink() string {
	scenariosURL := "https://yandex.ru/quasar/external/show-scenarios-list"
	if g.BulbasaurURL != "" {
		scenariosURL = appendSrcrwr(scenariosURL, g.BulbasaurURL)
	}
	return scenariosURL
}

func (g *AppLinksGenerator) BuildCreateScenarioPageLink() string {
	scenariosURL := "https://yandex.ru/quasar/external/scenario-create"
	if g.BulbasaurURL != "" {
		scenariosURL = appendSrcrwr(scenariosURL, g.BulbasaurURL)
	}
	return scenariosURL
}

func (g *AppLinksGenerator) BuildSkillIntegrationLink(skillID string) string {
	scenariosURL := fmt.Sprintf("https://yandex.ru/quasar/external/account-linking?skillId=%s", skillID)
	if g.BulbasaurURL != "" {
		scenariosURL = appendSrcrwr(scenariosURL, g.BulbasaurURL)
	}
	return scenariosURL
}

func (g *AppLinksGenerator) BuildAddDevicePageLink(deviceType model.DeviceType, skillID string) string {
	result := "https://yandex.ru/quasar/external/add-device"
	if skillID == model.TUYA {
		switch deviceType {
		case model.LightDeviceType:
			result = "https://yandex.ru/quasar/external/device-discovery?deviceType=devices.types.light"
		case model.SocketDeviceType:
			result = "https://yandex.ru/quasar/external/device-discovery?deviceType=devices.types.socket"
		case model.HubDeviceType:
			result = "https://yandex.ru/quasar/external/device-discovery?deviceType=devices.types.hub"
		}
	}
	if g.BulbasaurURL != "" {
		result = appendSrcrwr(result, g.BulbasaurURL)
	}
	return result
}

func appendSrcrwr(link, iotHost string) string {
	parsedURL, err := url.Parse(link)
	if err != nil {
		panic(err.Error())
	}
	query, err := url.ParseQuery(parsedURL.RawQuery)
	if err != nil {
		panic(err.Error())
	}

	query.Add("srcrwr", fmt.Sprintf("IOT_HOST:%s", iotHost))
	parsedURL.RawQuery = query.Encode()

	return parsedURL.String()
}

func SearchAppLink(link string) string {
	return sup.SearchAppLinkWrapper(sup.QuasarLinkWrapper(link))
}

func IotAppAndroidLink(link string) string {
	// IOT-1427: iotApp use different frontend package
	result := strings.Replace(link, "https://yandex.ru/quasar", "https://yandex.ru/iot", 1)
	return sup.IoTAndroidLinkWrapper(result)
}

func IotAppIOSLink(link string) string {
	// IOT-1427: iotApp use different frontend package
	result := strings.Replace(link, "https://yandex.ru/quasar", "https://yandex.ru/iot", 1)
	return sup.IoTIOSLinkWrapper(result)
}
