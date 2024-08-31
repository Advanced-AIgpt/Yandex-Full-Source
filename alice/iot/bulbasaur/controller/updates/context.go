package updates

import (
	"context"
)

type ctxKeySignal int

const (
	eventInfoKey ctxKeySignal = 0
)

func withEventSignal(ctx context.Context, event event) context.Context {
	return context.WithValue(ctx, eventInfoKey, eventInfo{id: event.id(), source: event.source()})
}
