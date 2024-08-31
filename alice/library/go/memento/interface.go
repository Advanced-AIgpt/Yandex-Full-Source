package memento

import (
	"context"

	memento "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IClient interface {
	GetUserObjects(ctx context.Context, userTicket string, request *memento.TReqGetUserObjects) (*memento.TRespGetUserObjects, error)
	UpdateUserObjects(ctx context.Context, userTicket string, request *memento.TReqChangeUserObjects) (*memento.TRespChangeUserObjects, error)
}

type Mock struct {
	getUserObjects    map[string]*memento.TRespGetUserObjects
	updateUserObjects map[string]*memento.TRespChangeUserObjects
}

func NewMock() *Mock {
	return &Mock{
		getUserObjects:    make(map[string]*memento.TRespGetUserObjects),
		updateUserObjects: make(map[string]*memento.TRespChangeUserObjects),
	}
}

func (m *Mock) GetUserObjects(ctx context.Context, userTicket string, request *memento.TReqGetUserObjects) (*memento.TRespGetUserObjects, error) {
	if res, exist := m.getUserObjects[userTicket]; exist {
		return res, nil
	}
	return nil, xerrors.New("no result for that user ticket")
}

func (m *Mock) UpdateUserObjects(ctx context.Context, userTicket string, request *memento.TReqChangeUserObjects) (*memento.TRespChangeUserObjects, error) {
	if res, exist := m.updateUserObjects[userTicket]; exist {
		return res, nil
	}
	return nil, xerrors.New("no result for that user ticket")
}
