package interceptor

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

type StateInterceptor struct {
	sessionInterceptor *SessionInterceptor
	stateCallbacks     []StateCallback
	logger             log.Logger
}

type StateCallback interface {
	IsRelevant(ctx context.Context, state string) bool
	Handle(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}, state string) (fallThrough bool)
}

func (i *StateInterceptor) RegisterCallback(cb StateCallback) {
	i.stateCallbacks = append(i.stateCallbacks, cb)
}

func (i *StateInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	if !i.sessionInterceptor.GetSession(ctx).State.Empty() {
		state := string(i.sessionInterceptor.GetSession(ctx).State)
		i.ResetState(ctx)
		found := false
		if i.stateCallbacks != nil {
			for _, cb := range i.stateCallbacks {
				if cb.IsRelevant(ctx, state) {
					found = true
					setrace.InfoLogEvent(ctx, i.logger, fmt.Sprintf("%T matched for state '%s'", cb, state))
					if cb.Handle(ctx, bot, eventType, event, state) {
						setrace.InfoLogEvent(ctx, i.logger, "Callback processed and asked to fallthrough")
						break
					}
					setrace.InfoLogEvent(ctx, i.logger, "Callback processed")
					return nil
				}
			}
		}
		if !found {
			setrace.ErrorLogEvent(ctx, i.logger, fmt.Sprintf("No callback found for state '%s'", state))
		}
	}
	return next(ctx, bot, eventType, event)
}

func (i *StateInterceptor) HasState(ctx context.Context) bool {
	return !i.sessionInterceptor.GetSession(ctx).State.Empty()
}

func (i *StateInterceptor) ResetState(ctx context.Context) {
	setrace.InfoLogEvent(ctx, i.logger, "Reset state")
	i.sessionInterceptor.GetSession(ctx).State = ""
}

func (i *StateInterceptor) SetState(ctx context.Context, state string) {
	setrace.InfoLogEvent(ctx, i.logger, "Set state: "+state)
	i.sessionInterceptor.GetSession(ctx).State = model.State(state)
}

func NewStateInterceptor(sessionInterceptor *SessionInterceptor, logger log.Logger) *StateInterceptor {
	return &StateInterceptor{
		sessionInterceptor: sessionInterceptor,
		logger:             logger,
	}
}
