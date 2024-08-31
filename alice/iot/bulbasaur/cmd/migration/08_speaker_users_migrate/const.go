package main

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type platformID int

const (
	_                       platformID = iota
	YandexStationPlatformID            //1
	YandexModulePlatformID
	DexpPlatformID
	IrbisPlatformID
	LGPlatformID
	ElariPlatformID
	YandexMiniPlatformID
	JetPlatformID
	PresigioPlatformID
	DigmaPlatformID
	JblPortablePlatformID
	JblMusicPlatformID
	DexpTVPlatformID
	YandexStation2PlatformID
	YandexMini2PlatformID
	YandexTVRealtek
	YandexTVRealtek2
	YandexTVHisilicon
	YandexTVMediatek
	YandexTVModule //20
)

var platform = map[platformID]string{ // dunno what to do if platform is unknown -> no need to make read with default
	YandexStationPlatformID:  "yandexstation",
	YandexModulePlatformID:   "yandexmodule",
	DexpPlatformID:           "lightcomm",
	IrbisPlatformID:          "linkplay_a98",
	LGPlatformID:             "wk7y",
	ElariPlatformID:          "elari_a98",
	YandexMiniPlatformID:     "yandexmini",
	YandexMini2PlatformID:    "yandexmini_2",
	JetPlatformID:            "jet_smart_music",
	PresigioPlatformID:       "prestigio_smart_mate",
	DigmaPlatformID:          "digma_di_home",
	JblPortablePlatformID:    "jbl_link_portable",
	JblMusicPlatformID:       "jbl_link_music",
	DexpTVPlatformID:         "dexp_tv",
	YandexStation2PlatformID: "yandexstation_2",
	YandexTVRealtek:          "yandex_tv_realtek",
	YandexTVRealtek2:         "yandex_tv_realtek2",
	YandexTVHisilicon:        "yandex_tv_hisilicon",
	YandexTVMediatek:         "yandex_tv_mediatek",
	YandexTVModule:           "yandex_tv_module",
}

type platformToName map[platformID]string

func (ptn platformToName) GetName(platformID platformID) string {
	if name, isKnown := ptn[platformID]; isKnown {
		return name
	}
	panic(fmt.Sprintf("unknown platformID: %v", platformID))
}

var defaultNames = platformToName{
	YandexStationPlatformID:  "Яндекс Станция",
	YandexStation2PlatformID: "Яндекс Станция 2",
	YandexModulePlatformID:   "Яндекс Модуль",
	DexpPlatformID:           "Колонка Dexp",
	IrbisPlatformID:          "Колонка Irbis",
	LGPlatformID:             "Колонка LG",
	ElariPlatformID:          "Колонка Elari",
	YandexMiniPlatformID:     "Яндекс Мини",
	YandexMini2PlatformID:    "Яндекс Мини 2",
	JetPlatformID:            "Колонка Jet Smart Music",
	PresigioPlatformID:       "Колонка Prestigio Smartmate",
	DigmaPlatformID:          "Колонка Digma Di Home",
	JblPortablePlatformID:    "JBL Link Portable",
	JblMusicPlatformID:       "JBL Link Music",
	DexpTVPlatformID:         "Dexp TV",
	YandexTVRealtek:          "Realtek TV",
	YandexTVRealtek2:         "Realtek2 TV",
	YandexTVHisilicon:        "Hisilicon TV",
	YandexTVMediatek:         "Mediatek TV",
	YandexTVModule:           "Module TV",
}

type platformToDeviceType map[platformID]model.DeviceType

func (ptdt platformToDeviceType) GetDeviceType(platformID platformID) model.DeviceType {
	if deviceType, isKnown := ptdt[platformID]; isKnown {
		return deviceType
	}
	panic(fmt.Sprintf("unknown platformID: %v", platformID))
}

var speakerDeviceTypes = platformToDeviceType{
	YandexStationPlatformID:  model.YandexStationDeviceType,
	YandexModulePlatformID:   model.YandexModuleDeviceType,
	DexpPlatformID:           model.DexpSmartBoxDeviceType, //lightcomm
	IrbisPlatformID:          model.IrbisADeviceType,       //linkplay
	LGPlatformID:             model.LGXBoomDeviceType,      //wk7y
	ElariPlatformID:          model.ElariSmartBeatDeviceType,
	YandexMiniPlatformID:     model.YandexStationMiniDeviceType,
	YandexMini2PlatformID:    model.YandexStationMini2DeviceType,
	JetPlatformID:            model.JetSmartMusicDeviceType,
	PresigioPlatformID:       model.PrestigioSmartMateDeviceType,
	JblPortablePlatformID:    model.JBLLinkPortableDeviceType,
	JblMusicPlatformID:       model.JBLLinkMusicDeviceType,
	DexpTVPlatformID:         model.TvDeviceDeviceType,
	YandexStation2PlatformID: model.YandexStation2DeviceType,
	YandexTVRealtek:          model.TvDeviceDeviceType,
	YandexTVRealtek2:         model.TvDeviceDeviceType,
	YandexTVHisilicon:        model.TvDeviceDeviceType,
	YandexTVMediatek:         model.TvDeviceDeviceType,
	YandexTVModule:           model.YandexModule2DeviceType,
}
