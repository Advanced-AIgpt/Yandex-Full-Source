package uuid

import (
	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

func RegistrySkill(c common.Controller) {
	c.AddCommand(common.NewCommand("uuid"), showUUID)
}

func showUUID(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(common.FormatFieldValueMD("UUID", ctx.GetSettings().GetApplicationDetails().UUID))
}
