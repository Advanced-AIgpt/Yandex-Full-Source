package libbegemot

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	wizardSignal int = iota
)

func withWizardSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, wizardSignal)
}
