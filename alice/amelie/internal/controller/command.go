package controller

import (
	"context"
	"fmt"
	"strings"

	tb "gopkg.in/tucnak/telebot.v2" // todo: remove dep

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

var (
	//helpCommandRegexp = regexp.MustCompile(`^/command *(?P<arg>[^ ]+)?$`)
	helpCommandRegexp = newCommandRegexp("/help", "/h")
)

const (
	sendMessageInfo = `Любые прочие сообщения будут отправлены на активную колонку, имитируя голосовой запрос.

@Жирным шрифтом@ выделено сокращение для вызова команды, например вместо /@y@outube можно написать /y и /esh вместо /@e@dit@sh@ortcuts.`
)

type userError struct {
	msg    string
	button *tb.InlineButton
}

func (u *userError) Error() string {
	return u.msg
}

func (u *userError) reply(ctx context.Context, bot telegram.Bot) error {
	if u.button == nil {
		_, _ = bot.Reply(ctx, u.msg)
	} else {
		_, _ = bot.Reply(ctx, u.msg, &tb.ReplyMarkup{
			InlineKeyboard: [][]tb.InlineButton{
				{
					*u.button,
				},
			},
		})
	}
	return nil
}

type commandHelper struct {
	command          string
	commandView      string
	shortDescription string
	longDescription  string
	groupName        string
}

type helpCommand struct {
	commands []commandHelper
}

func (cmd *helpCommand) registerCommandHelpers(add func(info commandHelper)) {}

func (cmd *helpCommand) addHelper(helper commandHelper) {
	if len(helper.shortDescription) < 3 {
		panic(fmt.Sprintf("commands shortDescription len must be greater than 3: %#v", helper))
	}
	short := getCommandView(helper.commandView)
	for _, command := range cmd.commands {
		if short == "" {
			break
		}
		if short == getCommandView(command.commandView) {
			panic(fmt.Sprintf("duplicated command views: %s and %s", helper.commandView, command.commandView))
		}
	}
	cmd.commands = append(cmd.commands, helper)
}

func getCommandView(view string) string {
	if !strings.Contains(view, "*") {
		return ""
	}
	return view[strings.Index(view, "*"):strings.LastIndex(view, "*")]
}

func (cmd *helpCommand) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(cmd)
}

func (cmd *helpCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return helpCommandRegexp.MatchString(msg.Text)
}

func (cmd *helpCommand) formatCommands() string {
	var otherLines []string
	var commandLines []string
	groups := map[string][]commandHelper{}
	for _, command := range cmd.commands {
		groups[command.groupName] = append(groups[command.groupName], command)
	}
	makeLine := func(helper commandHelper) string {
		name := helper.command
		if len(helper.commandView) > 0 {
			name = helper.commandView
		}
		return fmt.Sprintf("%s %s %s",
			name, telegram.EscapeMD("-"), telegram.EscapeMD(helper.longDescription))
	}
	// TODO: fix order
	for group, helpers := range groups {
		if group == "" {
			for _, helper := range helpers {
				otherLines = append(otherLines, makeLine(helper))
			}
			continue
		}
		if len(commandLines) > 0 {
			commandLines = append(commandLines, "")
		}
		commandLines = append(commandLines, fmt.Sprintf("*%s*", telegram.EscapeMD(group)))
		for _, helper := range helpers {
			commandLines = append(commandLines, makeLine(helper))
		}
	}
	if len(commandLines) > 0 {
		commandLines = append(commandLines, "")
	}
	return strings.Join(append(commandLines, otherLines...), "\n")
}

func (cmd *helpCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	//var commandLines []string
	//for _, helper := range cmd.commands {
	//	commandLines = append(commandLines, fmt.Sprintf("%s - %s", helper.command, helper.longDescription))
	//}
	_, _ = bot.Reply(ctx,
		fmt.Sprintf("%s\n\n%s", cmd.formatCommands(),
			strings.ReplaceAll(telegram.EscapeMD(sendMessageInfo), "@", "*")), tb.ModeMarkdownV2)
	return nil
}

func (cmd *helpCommand) GetBotCommands() (res []telegram.Command) {
	if cmd.commands == nil {
		return
	}
	for _, helper := range cmd.commands {
		res = append(res, telegram.Command{
			Text:        helper.command,
			Description: helper.shortDescription,
		})
	}
	return
}
