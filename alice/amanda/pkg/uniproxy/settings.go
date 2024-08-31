package uniproxy

import (
	"a.yandex-team.ru/alice/amanda/pkg/speechkit"
)

type Settings struct {
	UniproxyURL string
	AuthToken   string
	UUID        string
	Language    string
	Voice       string
	VINSURL     string
	ASRTopic    string

	App               speechkit.App
	Header            speechkit.Header
	Location          speechkit.Location
	Experiments       map[string]interface{}
	DeviceState       speechkit.DeviceState
	AdditionalOptions speechkit.AdditionalOptions
	VoiceSession      *bool
	ResetSession      bool
	LAASRegion        speechkit.LAASRegion

	SkipTTS bool
}
