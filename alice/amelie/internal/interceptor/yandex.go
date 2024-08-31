package interceptor

import (
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/alice/amelie/pkg/staff"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const (
	verificationPeriod = 5 * time.Minute
)

type YandexInterceptor struct {
	staff              staff.Client
	sessionInterceptor *SessionInterceptor
	logger             log.Logger
	serviceLogger      log.Logger
}

func (i *YandexInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	i.intercept(ctx)
	return next(ctx, bot, eventType, event)
}

func NewYandexInterceptor(client staff.Client, sessionInterceptor *SessionInterceptor, logger log.Logger, serviceLogger log.Logger) *YandexInterceptor {
	return &YandexInterceptor{
		staff:              client,
		sessionInterceptor: sessionInterceptor,
		logger:             logger,
		serviceLogger:      serviceLogger,
	}
}

func (i *YandexInterceptor) isInternal(ctx context.Context, username string) (bool, error) {
	user, err := i.staff.GetUserByTelegramUsername(ctx, username)
	if err != nil {
		return false, fmt.Errorf("unable to obtain information about user: %w", err)
	}
	return user.Official != nil && !user.Official.IsDismissed, nil
}

func (i *YandexInterceptor) intercept(ctx context.Context) {
	session := i.sessionInterceptor.GetSession(ctx)
	now := time.Now()
	if now.Sub(session.User.Yandex.VerificationTime) > verificationPeriod {
		isInternal, err := i.isInternal(ctx, session.User.Username)
		if err != nil {
			msg := fmt.Sprintf("Yandex verification error: %v", err)
			ctxlog.Errorf(ctx, i.serviceLogger, msg)
			setrace.ErrorLogEvent(ctx, i.serviceLogger, msg)
			return
		}
		session.User.Yandex.IsInternal = isInternal
		if isInternal {
			setrace.InfoLogEvent(ctx, i.logger, fmt.Sprintf("User %s is marked as internal", session.User.Username))
		}
		session.User.Yandex.VerificationTime = now
	}
}
