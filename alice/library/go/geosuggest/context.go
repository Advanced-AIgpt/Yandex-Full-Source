package geosuggest

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	getGeosuggestFromAddressSignal int = iota
)

func withGetGeosuggestFromAddressSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getGeosuggestFromAddressSignal)
}
