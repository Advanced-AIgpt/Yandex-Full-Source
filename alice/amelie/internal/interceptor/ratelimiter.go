package interceptor

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/amelie/internal/config"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

type RateLimiterInterceptor struct {
	sessionInterceptor *SessionInterceptor
	logger             log.Logger
	cfg                config.RateLimiter
}

func (r *RateLimiterInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	if len(r.sessionInterceptor.GetSession(ctx).Rate.RPSHistory) == 0 || r.cfg.MaxRPS == 0 {
		setrace.InfoLogEvent(ctx, r.logger, "rate limiter is skipped")
		return next(ctx, bot, eventType, event)
	}
	rpsDelay := time.Second / time.Duration(r.cfg.MaxRPS)
	for i, flashback := range r.sessionInterceptor.GetSession(ctx).Rate.RPSHistory {
		if !flashback.Actual {
			continue
		}
		if i == 0 {
			break // ok
		}
		prev := r.sessionInterceptor.GetSession(ctx).Rate.RPSHistory[i-1]
		if flashback.Time.Sub(prev.Time) >= rpsDelay {
			break
		}
		setrace.InfoLogEvent(ctx, r.logger, "RPS limit exceeded")
		return nil
	}
	return next(ctx, bot, eventType, event)
}

func NewRateLimiterInterceptor(logger log.Logger, sessionInterceptor *SessionInterceptor, cfg config.RateLimiter) *RateLimiterInterceptor {
	return &RateLimiterInterceptor{
		sessionInterceptor: sessionInterceptor,
		logger:             logger,
		cfg:                cfg,
	}
}
