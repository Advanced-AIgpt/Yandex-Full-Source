package common

import (
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type SpecifyRequestState struct {
	SemanticFrames []*common.TSemanticFrame
}

func (srs SpecifyRequestState) ToProto() *protos.SpecifyRequestState {
	return &protos.SpecifyRequestState{
		SemanticFrames: srs.SemanticFrames,
	}
}

func (srs *SpecifyRequestState) FromProto(p *protos.SpecifyRequestState) {
	*srs = SpecifyRequestState{
		SemanticFrames: p.GetSemanticFrames(),
	}
}

func (srs *SpecifyRequestState) FromBaseRequestState(state *anypb.Any) error {
	var specifyRequestStateProto protos.SpecifyRequestState
	if err := state.UnmarshalTo(&specifyRequestStateProto); err != nil {
		return err
	}
	srs.FromProto(&specifyRequestStateProto)
	return nil
}
