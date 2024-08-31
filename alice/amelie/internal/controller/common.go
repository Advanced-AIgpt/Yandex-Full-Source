package controller

import (
	"context"
	"crypto/md5"
	"fmt"
	"regexp"
	"strings"

	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

const (
	onboardingCommandText     = "/onboarding"
	shortcutsCommand          = "/shortcuts"
	shortcutsCommandAlias     = "/sh"
	shortcutsCommandView      = "/*sh*ortcuts"
	editShortcutsCommand      = "/editshortcuts"
	editShortcutsCommandAlias = "/esh"
	editShortcutsCommandView  = "/*e*dit*sh*ortcuts"
	meCommand                 = "/me"
	startCommand              = "/start"
	supportCommand            = "/support"
	supportCommandAlias       = "/su"
	supportCommandView        = "/*su*pport"
	infoCommand               = "/info"
	infoCommandAlias          = "/i"
	infoCommandView           = "/*i*nfo"
)

const (
	shortcutsAddCallback            = "shortcuts.add"
	shortcutsDeleteCallback         = "shortcuts.delete"
	shortcutsGoBackAfterAddCallback = "shortcuts.add.go_back"
	meSelectCallback                = "me.select"
)

const (
	sayInputState     = "say.input"
	shortcutsAddState = "shortcuts.add"
)

const (
	shortcutsGroupName  = "Быстрые команды"
	multimediaGroupName = "Мультимедиа"
	iotGroupName        = "Умный дом"
	accountGroupName    = "Аккаунт"
)

var (
	onboardingCommandRegexp    = newCommandRegexp(onboardingCommandText)
	shortcutsCommandRegexp     = newCommandRegexp(shortcutsCommand, shortcutsCommandAlias)
	editShortcutsCommandRegexp = newCommandRegexp(editShortcutsCommand, editShortcutsCommandAlias)
	meCommandRegexp            = newCommandRegexp(meCommand)
	startCommandRegexp         = newCommandRegexp(startCommand)
	supportCommandRegexp       = newCommandRegexp(supportCommand, supportCommandAlias)
	infoCommandRegexp          = newCommandRegexp(infoCommand, infoCommandAlias)
)

type (
	sessionManager interface {
		GetSession(ctx context.Context) *model.Session
	}
	stateManager interface {
		SetState(ctx context.Context, state string)
	}
	authManager interface {
		ShowLoginInfo(ctx context.Context, bot telegram.Bot)
		GetOAuthURL(bot telegram.Bot) string
		AddAccount(ctx context.Context, code string) error
	}
)

func newCommandRegexp(aliases ...string) *regexp.Regexp {
	cmd := strings.Join(aliases, "|")
	cmd = strings.ReplaceAll(cmd, "/", "")
	return regexp.MustCompile(fmt.Sprintf(`^/(%s)( .*)?$`, cmd))
}

func newCallbackWithArg(cb, arg string) string {
	return fmt.Sprintf("%s.%s", cb, arg)
}

func isCallbackWithArg(cb, value string) bool {
	return strings.HasPrefix(value, cb+".")
}

func parseCallbackWithArg(cb, value string) string {
	ss := len(cb) + 1
	if ss >= len(value) {
		return ""
	}
	return value[ss:]
}

func getHash(s string) string {
	return fmt.Sprintf("%x", md5.Sum([]byte(s)))
}
