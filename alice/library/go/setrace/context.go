package setrace

import (
	"context"
	"net/http"
	"os"

	"go.uber.org/atomic"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
)

type ctxKey int

const (
	mainActivationIDKey ctxKey = iota
	eventIndexKey
	mainRequestTimestampKey
	mainRequestIDKey
	childActivationIDKey
	childDescriptionKey
	pidKey
)

func GetMainRequestID(ctx context.Context) (string, bool) {
	v, b := ctx.Value(mainRequestIDKey).(string)
	return v, b
}

func GetEventIndex(ctx context.Context) (*atomic.Uint64, bool) {
	v, b := ctx.Value(eventIndexKey).(*atomic.Uint64)
	return v, b
}

func GetMainActivationID(ctx context.Context) (string, bool) {
	v, b := ctx.Value(mainActivationIDKey).(string)
	return v, b
}

func GetMainRequestTimestamp(ctx context.Context) (int64, bool) {
	v, b := ctx.Value(mainRequestTimestampKey).(int64)
	return v, b
}

func GetChildActivationID(ctx context.Context) (string, bool) {
	v, b := ctx.Value(childActivationIDKey).(string)
	return v, b
}

func GetChildDescription(ctx context.Context) (string, bool) {
	v, b := ctx.Value(childDescriptionKey).(string)
	return v, b
}

func GetPid(ctx context.Context) (uint32, bool) {
	v, b := ctx.Value(pidKey).(uint32)
	return v, b
}

// requestTimestamp in microseconds
func MainContext(ctx context.Context, requestID string, activationID string, requestTimestamp int64, pid uint32) context.Context {
	ctx = context.WithValue(ctx, mainRequestIDKey, requestID)
	ctx = context.WithValue(ctx, mainActivationIDKey, activationID)
	ctx = context.WithValue(ctx, eventIndexKey, &atomic.Uint64{})
	ctx = context.WithValue(ctx, mainRequestTimestampKey, requestTimestamp)
	ctx = context.WithValue(ctx, pidKey, pid)
	return ctx
}

func ChildContext(ctx context.Context, childActivationID, childDescription string) context.Context {
	ctx = context.WithValue(ctx, childActivationIDKey, childActivationID)
	ctx = context.WithValue(ctx, childDescriptionKey, childDescription)
	return ctx
}

func Middleware(logger log.Logger) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()
			timestamp, requestID, activationID, err := requestid.ParseRTLogTokenFromRequest(r)
			if err == nil {
				ctx = MainContext(ctx, requestID, activationID, timestamp, uint32(os.Getpid()))
				ActivationStarted(ctx, logger)
				defer ActivationFinished(ctx, logger)
			} else {
				logger.Warnf("unable to parse rtlog token from request: %v", err)
			}
			*r = *r.WithContext(ctx)
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
