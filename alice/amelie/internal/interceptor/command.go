package interceptor

import (
	"context"

	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/amelie/pkg/telegram/interceptor"
	"a.yandex-team.ru/library/go/core/log"
)

type TextCommand interface {
	Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error
	IsRelevant(ctx context.Context, msg *telegram.Message) bool
}

type CallbackCommand interface {
	Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error
	IsRelevant(ctx context.Context, cb *telegram.Callback) bool
}

type CommandInterceptor struct {
	interceptor.CommandInterceptor
}

func NewCommandInterceptor(logger log.Logger) *CommandInterceptor {
	return &CommandInterceptor{
		CommandInterceptor: *interceptor.NewCommandInterceptor(logger),
	}
}

func (i *CommandInterceptor) AddTextCommand(cmd TextCommand) {
	i.Add(func(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}) error {
		if eventType != telegram.TextEvent {
			return interceptor.ErrCommandIrrelevant
		}
		msg := telegram.AsMessage(event)
		if !cmd.IsRelevant(ctx, msg) {
			return interceptor.ErrCommandIrrelevant
		}
		return cmd.Handle(ctx, bot, msg)
	})
}

func (i *CommandInterceptor) AddCallbackCommand(cmd CallbackCommand) {
	i.Add(func(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}) error {
		if eventType != telegram.CallbackEvent {
			return interceptor.ErrCommandIrrelevant
		}
		cb := telegram.AsCallback(event)
		if !cmd.IsRelevant(ctx, cb) {
			return interceptor.ErrCommandIrrelevant
		}
		return cmd.Handle(ctx, bot, cb)
	})
}
