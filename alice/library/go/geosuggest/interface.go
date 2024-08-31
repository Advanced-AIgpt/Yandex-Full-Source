package geosuggest

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type IClient interface {
	GetGeosuggestFromAddress(ctx context.Context, address string) (GeosuggestFromAddressResponse, error)
}

type Mock struct {
	addressToResponses map[string]GeosuggestFromAddressResponse
}

func NewMock() *Mock {
	return &Mock{
		addressToResponses: make(map[string]GeosuggestFromAddressResponse),
	}
}

func (m *Mock) AddResponseToAddress(address string, response GeosuggestFromAddressResponse) {
	m.addressToResponses[address] = response
}

func (m *Mock) GetGeosuggestFromAddress(ctx context.Context, address string) (GeosuggestFromAddressResponse, error) {
	if res, exist := m.addressToResponses[address]; exist {
		return res, nil
	}
	return GeosuggestFromAddressResponse{}, xerrors.New("no result for that address")
}
