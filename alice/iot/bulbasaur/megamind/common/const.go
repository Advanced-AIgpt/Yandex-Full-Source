package common

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

// Capability types supported by granet processor
var SupportedCapabilityTypes = model.CapabilityTypes{
	model.OnOffCapabilityType, model.ColorSettingCapabilityType, model.ModeCapabilityType, model.RangeCapabilityType, model.ToggleCapabilityType,
	model.CustomButtonCapabilityType, model.QuasarServerActionCapabilityType, model.QuasarCapabilityType,
}

type RelativityType string

const (
	Increase RelativityType = "increase"
	Decrease RelativityType = "decrease"
	Invert   RelativityType = "invert"
)

var RelativeFlagNonSupportingSkills = map[string]bool{
	model.TUYA:                             true,
	model.QUASAR:                           true,
	model.VIRTUAL:                          true,
	model.XiaomiSkill:                      true,
	model.PhilipsSkill:                     true,
	model.LGSkill:                          true,
	model.SamsungSkill:                     true,
	model.LegrandSkill:                     true,
	model.RubetekSkill:                     true,
	model.PerenioSkill:                     true,
	model.ElariSkill:                       true,
	model.DigmaSkill:                       true,
	"357e40f9-640d-462e-8c39-8ecdade9b611": true,
	"3d1b38d7-ab99-44fe-a799-0daa65202358": true,
	"14b3d60c-36d7-42e3-adaa-3bf7bbd9dab8": true,
	"c178cd89-5bdb-44f2-b3ae-9bb328a36f7b": true,
	"35e2897a-c583-495a-9e33-f5d6f0f4cb49": true,
	"f1dc4741-402d-40a9-86bc-480356b68f75": true,
	"cd65d53a-0106-4708-bc6d-2042cb1d46c3": true,
	"f90771df-9756-4c81-9495-50825e7eeff8": true,
	"ffdfeaae-536f-42e6-bb80-a9ef73f39240": true,
	"311bf19d-2873-4c52-9a69-d20bd9410cf2": true,
	"fe03b0ca-5a70-4210-96d5-a4f1c10d96a8": true,
	"be937798-ec53-457b-aa7e-e928c05768b0": true,
	"580f0b9c-3420-489d-b7c9-4bec87057d06": true,
	"24b419c6-135f-4d28-b7cb-8a8da2fc5f1f": true,
	"b91b16fc-3e95-4e42-9654-a4f685e5bc44": true,
	"4172ed0a-a4d3-4d57-ac42-93d9a5f28668": true,
	"966138ad-35e5-4118-b47c-7a9f5bf17674": true,
	"c927bb15-5ecb-472a-8895-c3740602d36a": true,
	"0a36403d-0b70-4305-85d8-7da221d8f91c": true,
	"a7fc411f-0fe2-461e-915b-7ff11969c551": true,
	"2d7a8c3a-4326-49f4-b0ab-de5c81916614": true,
	"fc4fd3aa-a19b-4b55-9ea9-7e2289e9b67f": true,
	"4b7c9be0-3a7a-4591-80dc-f1cd37b49a9d": true,
	"a061fc56-af82-4d58-be45-5ab534881292": true,
	"85eb5745-659c-4730-98d7-3a13dd6a6fb8": true,
	"541a365a-100e-45d6-84f2-6c8770e31bab": true,
	"5f7f874e-9978-4329-b521-a276c4f1bec6": true,
	"91ddede6-c5e4-4662-9e9c-bf60e184f907": true,
	"023a3727-ade4-494d-a82e-6880ed2348b2": true,
	"13521c6a-bde1-4d62-9d67-10e0562c7543": true,
	"99ef8285-8e88-47d6-ae85-179f0e9c5b35": true,
	"e6a3a900-96c9-42ea-9472-4211d69b6011": true,
}

// Possible targets of query requests
const (
	CapabilityTarget = "capability"
	PropertyTarget   = "property"
	StateTarget      = "state"
)
