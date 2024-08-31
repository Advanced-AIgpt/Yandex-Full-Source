package middleware

import (
	"context"
	"net/http"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ctxKey int

const (
	skillIDKey ctxKey = iota
)

func SkillID(skillID string) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()
			*r = *r.WithContext(ctxlog.WithFields(ctx, log.String("skill_id", skillID)))
			*r = *r.WithContext(context.WithValue(ctx, skillIDKey, skillID))
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}

func GetSkillID(ctx context.Context) string {
	if ctx == nil {
		return ""
	}
	if skillID, ok := ctx.Value(skillIDKey).(string); ok {
		return skillID
	}
	return "" // better find a bug than implicitly ignore error
}
