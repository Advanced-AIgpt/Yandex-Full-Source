package interceptor

import (
	"context"
	"errors"

	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	ErrCommandIrrelevant = errors.New("COMMAND_IRRELEVANT")
)

type Command func(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{}) error
type CommandInterceptor struct {
	commands []Command
	logger   log.Logger
}

func (c *CommandInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	for _, command := range c.commands {
		if err := command(ctx, bot, eventType, event); err != nil {
			if errors.Is(err, ErrCommandIrrelevant) {
				continue
			}
			setrace.InfoLogEvent(ctx, c.logger, "Command processing failed")
			return err
		}
		setrace.InfoLogEvent(ctx, c.logger, "Command is processed")
		return nil
	}
	return next(ctx, bot, eventType, event)
}

func (c *CommandInterceptor) Add(command Command) {
	c.commands = append(c.commands, command)
}

func NewCommandInterceptor(logger log.Logger) *CommandInterceptor {
	return &CommandInterceptor{
		logger: logger,
	}
}
