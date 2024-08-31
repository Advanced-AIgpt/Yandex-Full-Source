package controller

import (
	"context"
	"fmt"
	"net/url"
	"regexp"
	"strings"

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/pkg/bass"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

var (
	youTubeCommandRegexp = regexp.MustCompile(`^/y(outube)?( +(?P<arg>[^ ]*))?$`)
	youTubeLinkRegexp    = regexp.MustCompile(`^((?:https?:)?//)?((?:www|m)\.)?((?:youtube\.com|youtu.be))(/(?:[\w\-]+\?v=|embed/|v/)?)([\w\-]+)(\S+)? *`)
)

type youTubeController struct {
	bassClient         bass.Client
	sessionInterceptor *interceptor.SessionInterceptor
}

func (y *youTubeController) registerCommandHelpers(add func(info commandHelper)) {
	add(commandHelper{
		command:          "/youtube",
		commandView:      "/*y*outube",
		shortDescription: "запустить видео с youtube",
		longDescription:  "запустить видео с youtube",
		groupName:        multimediaGroupName,
	})
}

func (y *youTubeController) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(&youTubeCommand{y})
}

type youTubeCommand struct {
	*youTubeController
}

func (y *youTubeCommand) PlayVideo(ctx context.Context, bot telegram.Bot, link string) error {
	parsedURL, err := url.Parse(link)
	if err != nil {
		_, _ = bot.Reply(ctx, "Что-то не так со ссылкой")
		return fmt.Errorf("PlayVideo error: invalid link: link=%s: %w", link, err)
	}
	var timestamp string
	var videoID string
	switch parsedURL.Host {
	case "www.youtube.com", "youtube.com", "m.youtube.com":
		videoID = parsedURL.Query().Get("v")
		timestamp = parsedURL.Query().Get("t")
	case "youtu.be", "www.youtu.be":
		videoID = parsedURL.Path[1:]
	default:
		_, _ = bot.Reply(ctx, "Не получилось распознать видео")
		return fmt.Errorf("PlayVideo error: unknown host: link=%s", link)
	}
	if videoID == "" {
		_, _ = bot.Reply(ctx, "Не получилось распознать видео")
		return fmt.Errorf("PlayVideo error: unknown videoID: link=%s", link)
	}
	if timestamp == "" {
		timestamp = "0"
	}
	acc, err := y.sessionInterceptor.GetSession(ctx).User.GetActiveAccount()
	if err != nil {
		_, _ = bot.Reply(ctx, "Нужно выбрать активный аккаунт")
		return err
	}
	if acc.DeviceID == "" {
		_, _ = bot.Reply(ctx, "Нужно выбрать активное устройство")
		return nil
	}
	return y.bassClient.PlayYouTubeVideo(ctx, acc.ID, acc.DeviceID, videoID, timestamp)
}

func (y *youTubeCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	link := msg.Text
	if strings.HasPrefix(link, "/") {
		split := strings.SplitN(link, " ", 2)
		link = ""
		if len(split) == 2 {
			link = split[1]
		}
	}
	link = strings.Trim(link, " ")
	if link == "" {
		_, _ = bot.Reply(ctx, "Какое видео нужно показать?")
		return nil
	}
	return y.PlayVideo(ctx, bot, link)
}

func (y *youTubeCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return youTubeCommandRegexp.MatchString(msg.Text) || youTubeLinkRegexp.MatchString(msg.Text)
}
