package endpoints

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
)

type EndpointLocation struct {
	HouseholdID string
	RoomName    string
}

// todo: apply arguments don't belong here

func (s EndpointLocation) ToProto() *protos.FinishDiscoveryApplyArguments_ParentEndpointLocation {
	return &protos.FinishDiscoveryApplyArguments_ParentEndpointLocation{
		HouseholdID: s.HouseholdID,
		RoomName:    s.RoomName,
	}
}

func (s *EndpointLocation) FromProto(p *protos.FinishDiscoveryApplyArguments_ParentEndpointLocation) {
	s.HouseholdID = p.GetHouseholdID()
	s.RoomName = p.GetRoomName()
}
