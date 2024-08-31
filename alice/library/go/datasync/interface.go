package datasync

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type IClient interface {
	GetAddressesForUser(ctx context.Context, userTicket string) (PersonalityAddressesResponse, error)
}

type Mock struct {
	addressesForUserTicket map[string]PersonalityAddressesResponse
}

func NewMock() *Mock {
	return &Mock{
		addressesForUserTicket: make(map[string]PersonalityAddressesResponse),
	}
}

func (m *Mock) AddAddressesForUser(userTicket string, response PersonalityAddressesResponse) {
	m.addressesForUserTicket[userTicket] = response
}

func (m *Mock) GetAddressesForUser(ctx context.Context, userTicket string) (PersonalityAddressesResponse, error) {
	if response, exist := m.addressesForUserTicket[userTicket]; exist {
		return response, nil
	}
	return PersonalityAddressesResponse{}, xerrors.Errorf("failed to get addresses for user ticket %s: no mock address for him", userTicket)
}
