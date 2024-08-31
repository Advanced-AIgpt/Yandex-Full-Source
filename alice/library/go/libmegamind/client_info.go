package libmegamind

import (
	"strings"
	"time"

	"a.yandex-team.ru/alice/library/client/protos"
)

// pure go port of https://a.yandex-team.ru/arc/trunk/arcadia/alice/library/client/client_info.cpp
type ClientInfo struct {
	AppID              string
	AppVersion         string
	OsVersion          string
	Platform           string
	UUID               string
	DeviceID           string
	Lang               string
	ClientTime         string
	Timezone           string
	Epoch              string
	DeviceModel        string
	DeviceManufacturer string
}

func NewClientInfo(p *protos.TClientInfoProto) ClientInfo {
	ci := ClientInfo{}
	if p.AppId != nil {
		ci.AppID = *p.AppId
	}
	if p.AppVersion != nil {
		ci.AppVersion = *p.AppVersion
	}
	if p.OsVersion != nil {
		ci.OsVersion = *p.OsVersion
	}
	if p.Platform != nil {
		ci.Platform = *p.Platform
	}
	if p.Uuid != nil {
		ci.UUID = *p.Uuid
	}
	if p.DeviceId != nil {
		ci.DeviceID = *p.DeviceId
	}
	if p.Lang != nil {
		ci.Lang = *p.Lang
	}
	if p.ClientTime != nil {
		ci.ClientTime = *p.ClientTime
	}
	if p.Timezone != nil {
		ci.Timezone = *p.Timezone
	}
	if p.Epoch != nil {
		ci.Epoch = *p.Epoch
	}
	if p.DeviceModel != nil {
		ci.DeviceModel = *p.DeviceModel
	}
	if p.DeviceManufacturer != nil {
		ci.DeviceManufacturer = *p.DeviceManufacturer
	}
	return ci
}

func (c ClientInfo) IsYaMessenger() bool {
	return c.AppID == "ru.yandex.messenger"
}

func (c ClientInfo) IsWindows() bool {
	return c.Platform == "windows"
}

func (c ClientInfo) IsMac() bool {
	return c.Platform == "MacIntel" //FIXME: can be AppleSilicon
}

func (c ClientInfo) IsYaBrowser() bool {
	return c.IsYaBrowserTest() || c.IsYaBrowserProd()
}

func (c ClientInfo) IsYaBrowserTest() bool {
	return c.IsYaBrowserTestDesktop() || c.IsYaBrowserTestMobile() || c.IsYaBrowserIpadTest()
}

func (c ClientInfo) IsYaBrowserProd() bool {
	return c.IsYaBrowserProdDesktop() || c.IsYaBrowserProdMobile() || c.IsYaBrowserIpadProd()
}

func (c ClientInfo) IsYaBrowserProdDesktop() bool {
	return c.AppID == "yabro"
}

func (c ClientInfo) IsYaBrowserProdMobile() bool {
	return c.AppID == "ru.yandex.mobile.search" ||
		c.AppID == "com.yandex.browser"
}

func (c ClientInfo) IsYaBrowserIpadProd() bool {
	return c.AppID == "ru.yandex.mobile.search.ipad"
}

func (c ClientInfo) IsYaBrowserTestDesktop() bool {
	return c.AppID == "yabro.beta" ||
		c.AppID == "yabro.broteam" ||
		c.AppID == "yabro.canary" ||
		c.AppID == "yabro.dev"
}

func (c ClientInfo) IsYaBrowserTestMobile() bool {
	return c.AppID == "ru.yandex.mobile.search.inhouse" ||
		c.AppID == "ru.yandex.mobile.search.dev" ||
		c.AppID == "ru.yandex.mobile.search.test" ||
		// Android
		c.AppID == "com.yandex.browser.beta" ||
		c.AppID == "com.yandex.browser.alpha" ||
		c.AppID == "com.yandex.browser.inhouse" || // TODO: remove (ask gump@)
		c.AppID == "com.yandex.browser.dev" || // TODO: remove (ask gump@)
		c.AppID == "com.yandex.browser.canary" ||
		c.AppID == "com.yandex.browser.broteam"
}

func (c ClientInfo) IsYaBrowserIpadTest() bool {
	return c.AppID == "ru.yandex.mobile.search.ipad.inhouse" ||
		c.AppID == "ru.yandex.mobile.search.ipad.dev" ||
		c.AppID == "ru.yandex.mobile.search.ipad.test"
}

func (c ClientInfo) IsSampleApp() bool {
	return c.AppID == "com.yandex.dialog_assistant.sample" ||
		c.AppID == "ru.yandex.mobile.search.dialog_assistant_sample" ||
		c.IsAliceKit()
}

func (c ClientInfo) IsAliceKit() bool {
	return strings.HasPrefix(c.AppID, "ru.yandex.mobile.alice") ||
		strings.HasPrefix(c.AppID, "com.yandex.alicekit")
}

func (c ClientInfo) IsTestClient() bool {
	return c.AppID == "uniproxy.monitoring" ||
		c.AppID == "uniproxy.test" ||
		c.AppID == "com.yandex.search.shooting" ||
		c.AppID == "test" ||
		strings.HasPrefix(c.AppID, "com.yandex.vins")
}

func (c ClientInfo) IsWeatherPluginTest() bool {
	return strings.HasPrefix(c.AppID, "ru.yandex.weatherplugin.")
}

func (c ClientInfo) IsStandalone() bool {
	return c.AppID == "com.yandex.alice" ||
		c.AppID == "ru.yandex.mobile.aliceapp" ||
		c.AppID == "ru.yandex.mobile.aliceapp.dev" ||
		c.AppID == "ru.yandex.mobile.aliceapp.inhouse"
}

func (c ClientInfo) IsIotApp() bool {
	return c.AppID == "com.yandex.iot" ||
		c.AppID == "com.yandex.iot.dev" ||
		c.AppID == "com.yandex.iot.test" ||
		c.AppID == "com.yandex.iot.inhouse"
}

func (c ClientInfo) IsIotAppIOS() bool {
	return c.IsIotApp() && c.IsIOS()
}

func (c ClientInfo) IsIotAppAndroid() bool {
	return c.IsIotApp() && c.IsAndroid()
}

func (c ClientInfo) IsWeatherPluginProd() bool {
	return c.AppID == "ru.yandex.weatherplugin"
}

func (c ClientInfo) IsWeatherPlugin() bool {
	return c.IsWeatherPluginProd() || c.IsWeatherPluginTest()
}

func (c ClientInfo) IsSearchAppTest() bool {
	return c.AppID == "ru.yandex.mobile.inhouse" ||
		c.AppID == "ru.yandex.mobile.dev" ||
		strings.HasPrefix(c.AppID, "ru.yandex.searchplugin.") ||
		c.IsWeatherPluginTest()
}
func (c ClientInfo) IsSearchAppProd() bool {
	return c.AppID == "ru.yandex.mobile" ||
		c.AppID == "ru.yandex.searchplugin" ||
		c.IsWeatherPluginProd()
}

func (c ClientInfo) IsSearchApp() bool {
	return c.IsSearchAppProd() ||
		c.IsSearchAppTest() ||
		c.IsWeatherPlugin() || // absorbed client: DIALOG-5251
		c.IsTestClient() || // assume that all test clients are "Search app"
		c.IsSampleApp()
}

func (c ClientInfo) IsElariWatch() bool {
	return strings.HasPrefix(c.AppID, "ru.yandex.iosdk.elariwatch")
}

func (c ClientInfo) IsQuasar() bool {
	return strings.HasPrefix(c.AppID, "ru.yandex.quasar")
}

// actually, it is not only yandex mini speaker - that predicate matches every speaker not powered by Android
// such as JBL, LG, etc
func (c ClientInfo) IsMiniSpeaker() bool {
	return strings.HasPrefix(c.AppID, "aliced")
}

func (c ClientInfo) IsYandexMiniSpeaker() bool {
	return strings.HasPrefix(c.DeviceModel, "yandexmini")
}

func (c ClientInfo) IsYandexMicroSpeaker() bool {
	return strings.HasPrefix(c.DeviceModel, "yandexmicro")
}

func (c ClientInfo) IsYandexMidiSpeaker() bool {
	return strings.HasPrefix(c.DeviceModel, "yandexmidi")
}

func (c ClientInfo) IsYandexStationSpeaker() bool {
	return strings.HasPrefix(c.DeviceModel, "Station")
}

func (c ClientInfo) IsYandexStationMaxSpeaker() bool {
	return strings.HasPrefix(c.DeviceModel, "Station_2")
}

func (c ClientInfo) IsJBLSpeaker() bool {
	return strings.HasPrefix(c.DeviceModel, "jbl")
}

func (c ClientInfo) IsCentaur() bool {
	return strings.HasPrefix(c.AppID, "ru.yandex.centaur")
}

func (c ClientInfo) IsSmartSpeaker() bool {
	return c.IsQuasar() || c.IsMiniSpeaker() || c.IsCentaur()
}

func (c ClientInfo) IsIOS() bool {
	return c.Platform == "ios" ||
		c.Platform == "ipad" ||
		c.Platform == "iphone"
}

func (c ClientInfo) IsAndroid() bool {
	return c.Platform == "android"
}

func (c ClientInfo) IsTvDevice() bool {
	return strings.HasPrefix(c.AppID, "com.yandex.tv.alice")
}

func (c ClientInfo) IsYandexModule2() bool {
	return strings.HasPrefix(c.DeviceModel, "yandexmodule_2")
}

func (c ClientInfo) GetLocation(defaultTimezone string) *time.Location {
	timezone := c.Timezone
	if timezone == "" {
		timezone = defaultTimezone
	}
	location, err := time.LoadLocation(timezone)
	if err != nil {
		location, err = time.LoadLocation(defaultTimezone)
		if err != nil {
			return time.UTC
		}
	}
	return location
}

func (c ClientInfo) IsSelfDrivingCar() bool {
	return strings.HasPrefix(c.AppID, "ru.yandex.sdg")
}
