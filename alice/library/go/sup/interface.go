package sup

import (
	"context"
	"strconv"
)

type IClient interface {
	SendPush(ctx context.Context, request PushRequest) (PushResponse, error)
}

type IReceiver interface {
	isSupReceiver()
	MarshalJSON() ([]byte, error)
}

type ClientMock struct {
	usersPushCount  map[uint64]int
	userPushes      map[uint64][]PushRequest
	PushIDGenerator int
}

func (cm *ClientMock) SendPush(ctx context.Context, request PushRequest) (PushResponse, error) {
	receivers := make(map[uint64]struct{})
	for _, receiver := range request.Receivers {
		if uidReceiver, ok := receiver.(UIDReceiver); ok {
			uid, err := strconv.ParseUint(uidReceiver.UID, 10, 64)
			if err != nil {
				return PushResponse{}, err
			}
			cm.usersPushCount[uid]++
			cm.userPushes[uid] = append(cm.userPushes[uid], request)
			receivers[uid] = struct{}{}
		}
	}
	response := PushResponse{
		ID:        strconv.Itoa(cm.PushIDGenerator),
		Receivers: uint64(len(receivers)),
	}
	cm.PushIDGenerator++
	return response, nil
}

func (cm *ClientMock) PushCount(uid uint64) int {
	if res, exist := cm.usersPushCount[uid]; exist {
		return res
	}
	return 0
}

func (cm *ClientMock) Pushes(uid uint64) []PushRequest {
	if res, exist := cm.userPushes[uid]; exist {
		return res
	}
	return []PushRequest{}
}

func NewClientMock() *ClientMock {
	return &ClientMock{
		usersPushCount: make(map[uint64]int),
		userPushes:     make(map[uint64][]PushRequest),
	}
}
