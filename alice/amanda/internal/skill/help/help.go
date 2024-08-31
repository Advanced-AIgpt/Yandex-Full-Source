package help

import (
	"bytes"
	"fmt"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/account"
	appskill "a.yandex-team.ru/alice/amanda/internal/skill/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
	"a.yandex-team.ru/alice/amanda/internal/skill/debug"
	"a.yandex-team.ru/alice/amanda/internal/skill/device"
	"a.yandex-team.ru/alice/amanda/internal/skill/experiments"
	"a.yandex-team.ru/alice/amanda/internal/skill/features"
	"a.yandex-team.ru/alice/amanda/internal/skill/location"
	"a.yandex-team.ru/alice/amanda/internal/skill/params"
	"a.yandex-team.ru/alice/amanda/internal/skill/queryparams"
)

// NDA link: https://nda.ya.ru/t/Xh9-1Dju3WSK7A

const _supportChatLink = "https://t.me/joinchat/BBPaeRGYwx3cNiNYXnK0SQ"

var commandsGroup = newGroup().
	addBlock(newBlock("Аккаунт").
		addCommand("acc", "посмотреть информацию или сменить аккаунт").
		addCommand("addacc", "добавить аккаунт").
		addCommand("delacc", "удалить аккаунт"),
	).
	addBlock(newBlock("Приложение").
		addCommand("app", "посмотреть параметры приложения").
		addCommand("apppresets", "выбрать пресет").
		addCommand("setappid", "установить AppID").
		addCommand("setappversion", "установить AppVersion").
		addCommand("setosversion", "установить OSVersion").
		addCommand("setplatform", "установить Platform").
		addCommand("setdeviceid", "установить DeviceID").
		addCommand("setdevicemodel", "установить DeviceModel").
		addCommand("setdevicemanufacturer", "установить Device Manufacturer").
		addCommand("setlanguage", "установить Language").
		addCommand("setuseragent", "установить UserAgent"),
	).
	addBlock(newBlock("Управление устройством").
		addCommand("device", "настроить параметры устройства").
		addCommand("setspotter", "установить споттер").
		addCommand("setfiltrationlevel", "установить режим фильтрации контента Алисы").
		addCommand("devices", "настроить подключенные устройства").
		addCommand("adddevice", "подключить устройство").
		addCommand("deldevice", "удалить устройство"),
	).
	addBlock(newBlock("Параметры").
		addCommand("params", "посмотреть параметры").
		addCommand("setmmurl", "установить MegamindURL").
		addCommand("setuniproxyurl", "установить UniproxyURL").
		addCommand("setvoice", "выбрать спикера").
		addCommand("voicesession", "настроить VoiceSession").
		addCommand("tts", "настроить TTS"),
	).
	addBlock(newBlock("Параметры запроса").
		addCommand("listqueryparams", "посмотреть параметры запроса").
		addCommand("addqueryparam", "добавить параметр запроса").
		addCommand("delqueryparam", "удалить параметр запроса").
		addCommand("switchqueryparam", "включить/выключить параметр запроса"),
	).
	addBlock(newBlock("Местоположение").
		addCommand("location", "посмотреть информацию о местоположении").
		addCommand("setlocation", "установить координаты").
		addCommand("setregionid", "установить регион"),
	).
	addBlock(newBlock("Эксперименты").
		addCommand("exp", "показать эксперименты").
		addCommand("setexp", "установить эксперимент").
		addCommand("setexpnum", "установить числовой эксперимент").
		addCommand("delexp", "удалить эксперимент").
		addCommand("switchexp", "включить/выключить эксперимент"),
	).
	addBlock(newBlock("Supported Features").
		addCommand("features", "посмотреть supported features").
		addCommand("addfeatures", "добавить supported features").
		addCommand("switchfeatures", "включить/выключить supported features").
		addCommand("delfeatures", "удалить supported features"),
	).
	addBlock(newBlock("Отладка").
		addCommand("setrace", "посмотреть информацию о последних запросах").
		addCommand("directives", "настроить обработку директив").
		addCommand("renderdivcard", "отрендерить div'ную карточку"),
	).
	addBlock(newBlock("Прочее").
		addCommand("imagerecognition", "распознать картинку").
		addCommand("resetsession", "управление сбросом сессии").
		addCommand("resetbot", "сбросить настройки бота").
		addCommand("me", "получить информацию обо всех параметрах").
		addCommand("uuid", "получить текущий UUID").
		addCommand("switchhints", "включить/выключить подсказки команд"),
	).
	addBlock(newBlock("").
		addCommand("cancel", "отменить текущую команду"),
	)

func InitBotCommands(bot *telebot.Bot) error {
	var res []telebot.Command
	for _, block := range commandsGroup.blocks {
		for _, cmd := range block.commands {
			res = append(res, telebot.Command{
				Text:        cmd.command,
				Description: cmd.description,
			})
		}
	}
	return bot.SetCommands(res)
}

func RegistrySkill(c common.Controller) {
	reCMD := common.NewCommand
	c.AddCommand(reCMD("start"), start)
	c.AddCommand(reCMD("help"), help)
	c.AddCommand(reCMD("botfathercommands"), func(ctx app.Context, msg *telebot.Message) {
		_, _ = ctx.Send(commandsGroup.toBotCommands())
	})
	c.AddCommand(reCMD("imagerecognition"), func(ctx app.Context, msg *telebot.Message) {
		_, _ = ctx.SendMD("Отправьте мне фотогографию, а я погляжу и скажу, кто или что на ней")
	})
	c.AddCommand(reCMD("renderdivcard"), func(ctx app.Context, msg *telebot.Message) {
		_, _ = ctx.SendMD(&telebot.Document{
			File:     telebot.FromReader(bytes.NewReader([]byte(_divCardExampleJSON))),
			Caption:  "Отправьте мне JSON файл с body div карточкой и получите ее рендер",
			MIME:     "application/json",
			FileName: "DIVCardExample.json",
		})
	})
	c.AddCommand(reCMD("me"), me)
	c.AddCommand(reCMD("switchhints"), switchHints)
}

func switchHints(ctx app.Context, msg *telebot.Message) {
	action := "выключены"
	if ctx.GetSettings().GetSystemDetails().HideHinds {
		action = "включены"
	}
	ctx.GetSettings().GetSystemDetails().HideHinds = !ctx.GetSettings().GetSystemDetails().HideHinds
	_, _ = ctx.Send(fmt.Sprintf("Подсказки были успешно %s", action))
}

func me(ctx app.Context, msg *telebot.Message) {
	text := strings.Join([]string{
		common.FormatFieldValueMD("Пользователь", account.GetUsername(ctx)),
		appskill.GetInfo(ctx),
		params.GetInfo(ctx),
		queryparams.GetInfo(ctx),
		experiments.GetInfo(ctx),
		features.GetInfo(ctx),
		location.GetInfo(ctx),
		device.GetInfo(ctx),
		common.FormatFieldValueMD("Подсказки", GetHintsInfo(ctx)),
		debug.GetDirectivesInfo(ctx),
	}, "\n")
	_, _ = ctx.SendMD(text)
}

func GetHintsInfo(ctx app.Context) string {
	if ctx.GetSettings().GetSystemDetails().HideHinds {
		return "выключены"
	}
	return "включены"
}

func help(ctx app.Context, msg *telebot.Message) {
	text := strings.Join(
		[]string{
			"[Чат поддержки](" + _supportChatLink + ")",
			"",
			commandsGroup.toHelpTextMD(),
			"",
			"В большинстве команд начинающихся с `/set` поддержан вариант `/setkey value`",
			`В некоторых командах можно использовать "none" для сброса значения`,
		},
		"\n",
	)
	_, _ = ctx.SendMD(text)
}

func start(ctx app.Context, msg *telebot.Message) {
	text := strings.Join(
		[]string{
			"Я могу помочь Вам протестировать Алису, просто отправьте мне сообщение",
			"Для получения информации по доступным командам используйте /help",
		},
		"\n",
	)
	_, _ = ctx.SendMD(text)
}

type helpCommand struct {
	command     string
	description string
}

func (c helpBlock) addCommand(command, description string) helpBlock {
	c.commands = append(c.commands, helpCommand{
		command:     command,
		description: description,
	})
	return c
}

type helpBlock struct {
	title    string
	commands []helpCommand
}

type helpGroup struct {
	blocks []helpBlock
}

func newGroup() helpGroup {
	return helpGroup{}
}

func newBlock(title string) helpBlock {
	return helpBlock{
		title: title,
	}
}

func (group helpGroup) addBlock(block helpBlock) helpGroup {
	group.blocks = append(group.blocks, block)
	return group
}

func (group helpGroup) toHelpTextMD() string {
	var lines []string
	for _, block := range group.blocks {
		if len(lines) > 0 {
			lines = append(lines, "")
		}
		if block.title != "" {
			lines = append(lines, fmt.Sprintf("*%s*", common.EscapeMD(block.title)))
		}
		for _, cmd := range block.commands {
			lines = append(lines, common.EscapeMD(fmt.Sprintf("/%s - %s", cmd.command, cmd.description)))
		}
	}
	return strings.Join(lines, "\n")
}

func (group helpGroup) toBotCommands() string {
	var lines []string
	for _, block := range group.blocks {
		for _, cmd := range block.commands {
			lines = append(lines, fmt.Sprintf("%s - %s", cmd.command, cmd.description))
		}
	}
	return strings.Join(lines, "\n")
}
