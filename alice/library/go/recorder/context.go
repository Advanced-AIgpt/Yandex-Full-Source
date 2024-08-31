package recorder

import "context"

type ctxKeyRecorder int

const reqIDRecorderKey ctxKeyRecorder = 0

func WithDebugInfoRecorder(ctx context.Context, recorder *DebugInfoRecorder) context.Context {
	return context.WithValue(ctx, reqIDRecorderKey, recorder)
}

func GetDebugInfoRecorder(ctx context.Context) *DebugInfoRecorder {
	if ctx == nil {
		return nil
	}
	if logRecorder, ok := ctx.Value(reqIDRecorderKey).(*DebugInfoRecorder); ok {
		return logRecorder
	}
	return nil
}
