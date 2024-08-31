package frames

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type RepeatAfterMeFrame struct {
	Text  string `json:"text"`
	Voice string `json:"voice"`
}

func (f RepeatAfterMeFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_RepeatAfterMeSemanticFrame{
			RepeatAfterMeSemanticFrame: &common.TRepeatAfterMeSemanticFrame{
				Text:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: f.Text}},
				Voice: &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: f.Voice}},
			},
		},
	}
}

func NewRepeatAfterMeFrame(text string, voice string) RepeatAfterMeFrame {
	return RepeatAfterMeFrame{Text: text, Voice: voice}
}
