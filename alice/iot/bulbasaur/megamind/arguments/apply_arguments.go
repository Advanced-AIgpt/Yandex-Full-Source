package arguments

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
)

type ActionIntentApplyArguments struct {
	ExtractedActionIntent  ExtractedActionIntent `json:"extracted_action_intent"`
	SilentResponseRequired bool                  `json:"silent_response_required"`
}

func (aa *ActionIntentApplyArguments) ToProto(ctx context.Context) *protos.ActionIntentApplyArguments {
	return &protos.ActionIntentApplyArguments{
		ExtractedActionIntent:  aa.ExtractedActionIntent.ToProto(ctx),
		SilentResponseRequired: aa.SilentResponseRequired,
	}
}

func (aa *ActionIntentApplyArguments) FromProto(p *protos.ActionIntentApplyArguments) {
	extractedActionIntent := ExtractedActionIntent{}
	extractedActionIntent.FromProto(p.GetExtractedActionIntent())

	arguments := ActionIntentApplyArguments{
		ExtractedActionIntent:  extractedActionIntent,
		SilentResponseRequired: p.GetSilentResponseRequired(),
	}

	*aa = arguments
}
