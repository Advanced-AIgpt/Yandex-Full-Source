package controller

import (
	"context"

	tb "gopkg.in/tucnak/telebot.v2" // TODO: remove

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

var onboardingActions = []string{
	"Включи мою музыку",
	"Включи утреннее шоу",
	"Какая погода?",
}

func makeOnboardingReplyKeyboard() (keyboard [][]tb.ReplyButton) {
	for _, action := range onboardingActions {
		keyboard = append(keyboard, []tb.ReplyButton{{
			Text: action,
		}})
	}
	return
}

type onboardingController struct {
}

func (o *onboardingController) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(&onboardingCommand{o})
}

func (o *onboardingController) registerCommandHelpers(add func(info commandHelper)) {
	add(commandHelper{
		command:          onboardingCommandText,
		shortDescription: "быстрые команды",
		longDescription:  "быстрые команды",
	})
}

type onboardingCommand struct {
	*onboardingController
}

func (o *onboardingCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	_, _ = bot.Reply(ctx, "Вот, что я могу", &tb.ReplyMarkup{
		ReplyKeyboard:       makeOnboardingReplyKeyboard(),
		ResizeReplyKeyboard: true,
		OneTimeKeyboard:     true,
	})
	return nil
}

func (o *onboardingCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return onboardingCommandRegexp.MatchString(msg.Text)
}
