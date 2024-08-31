package discovery

import (
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
)

type State struct {
	SessionID string
}

func (srs State) toProto() *protos.YandexIODiscoveryState {
	return &protos.YandexIODiscoveryState{
		SessionID: srs.SessionID,
	}
}

func (srs *State) fromProto(p *protos.YandexIODiscoveryState) {
	srs.SessionID = p.GetSessionID()
}

func (srs *State) FromProtoState(requestState *anypb.Any) error {
	var protoState protos.YandexIODiscoveryState
	if err := requestState.UnmarshalTo(&protoState); err != nil {
		return err
	}
	srs.fromProto(&protoState)
	return nil
}
