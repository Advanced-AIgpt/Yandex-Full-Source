package internal

import (
	"a.yandex-team.ru/alice/amanda/pkg/speechkit"
)

type ASRResult struct {
	ResponseCode   string       `json:"responseCode,omitempty"`
	Recognition    []Hypothesis `json:"recognition,omitempty"`
	EndOfUtterance bool         `json:"endOfUtt"`
}

type Word struct {
	Confidence float64 `json:"confidence"`
	Value      string  `json:"value"`
}

type Hypothesis struct {
	Confidence float64 `json:"confidence"`
	Words      []Word  `json:"words"`
}
type AdvancedASROptions struct {
	UtteranceSilence int `json:"utterance_silence"`
}

type SpeechKitRequest struct {
	speechkit.Request

	Topic              string              `json:"topic,omitempty"`
	Format             string              `json:"format,omitempty"`
	AdvancedASROptions *AdvancedASROptions `json:"advancedASROptions,omitempty"`
}
