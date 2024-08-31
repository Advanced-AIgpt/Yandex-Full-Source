package libbass

import (
	"context"
)

type IClient interface {
	SendPush(ctx context.Context, payload PushPayload) error
}

type Mock struct {
	SendPushFunc func(ctx context.Context, payload PushPayload) error
}

func (m Mock) SendPush(ctx context.Context, payload PushPayload) error {
	if m.SendPushFunc != nil {
		return m.SendPushFunc(ctx, payload)
	}
	return nil
}
