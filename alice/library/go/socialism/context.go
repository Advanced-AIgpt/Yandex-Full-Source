package socialism

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = iota
	skillInfoKey

	getUserApplicationTokenInfoSignal int = iota
	checkUserAppTokenExistsSignal
	deleteUserTokenSignal
)

func withGetUserApplicationTokenInfoSignal(ctx context.Context, skillInfo SkillInfo) context.Context {
	ctx = context.WithValue(ctx, signalKey, getUserApplicationTokenInfoSignal)
	ctx = context.WithValue(ctx, skillInfoKey, skillInfo)
	return ctx
}

func withCheckUserAppTokenExistsSignal(ctx context.Context, skillInfo SkillInfo) context.Context {
	ctx = context.WithValue(ctx, signalKey, checkUserAppTokenExistsSignal)
	ctx = context.WithValue(ctx, skillInfoKey, skillInfo)
	return ctx
}

func withDeleteUserTokenSignal(ctx context.Context, skillInfo SkillInfo) context.Context {
	ctx = context.WithValue(ctx, signalKey, deleteUserTokenSignal)
	ctx = context.WithValue(ctx, skillInfoKey, skillInfo)
	return ctx
}
