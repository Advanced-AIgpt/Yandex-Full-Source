package middleware

import (
	"net/http"

	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func DebugInfoRecorder(factory *recorder.DebugInfoRecorderFactory, dialoger dialogs.Dialoger) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()
			if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); skillID != "" {
				ctx = requestsource.WithRequestSource(ctx, "devconsole")
				if user, err := userctx.GetUser(ctx); err == nil {
					ctxlog.Infof(ctx, factory.Logger, "Creating recorder for user %d, skillId %s", user.ID, skillID)
					authData, err := dialoger.AuthorizeSkillOwner(ctx, user.ID, skillID, user.Ticket)
					if err != nil {
						ctxlog.Infof(ctx, factory.Logger, "Dialog auth failed, skip creating recorder, reason: %v", err)
					} else if authData.Success {
						ctx = dialogs.WithDialogAuthData(ctx, authData)
						ctx = recorder.WithDebugInfoRecorder(ctx, factory.CreateRecorder())
						ctxlog.Info(ctx, factory.Logger, "Dialog auth successful, recorder created")
					} else {
						ctxlog.Info(ctx, factory.Logger, "Dialog auth unsuccessful, skip creating recorder")
					}
				}
			}
			*r = *r.WithContext(ctx)
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
