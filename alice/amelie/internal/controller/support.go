package controller

import (
	"context"

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

type supportController struct {
}

func (s *supportController) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(&supportCommandHandler{})
}

func (s *supportController) registerCommandHelpers(add func(info commandHelper)) {
	add(commandHelper{
		command:          supportCommand,
		commandView:      supportCommandView,
		shortDescription: "информация о поддержке",
		longDescription:  "информация о поддержке",
	})
}

type supportCommandHandler struct {
}

func (h *supportCommandHandler) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	_, _ = bot.Reply(ctx, "Если у Вас появились проблемы при использовании бота, Вы можете обратиться за помощью в дружелюбный чат по устройствам с Алисой и Умному дому: https://t.me/station_yandex")
	return nil
}

func (h *supportCommandHandler) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return supportCommandRegexp.MatchString(msg.Text)
}
