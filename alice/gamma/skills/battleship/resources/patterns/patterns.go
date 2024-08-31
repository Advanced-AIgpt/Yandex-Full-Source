package patterns

import (
	"encoding/json"
	"fmt"
	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/library/go/core/resource"
)

const (
	StartIntent     = "start"
	RestartIntent   = "restart"
	RulesIntent     = "rules"
	WaitIntent      = "wait"
	SurrenderIntent = "surrender"
	EndIntent       = "end"

	YesIntent = "yes"
	NoIntent  = "no"

	KillIntent    = "kill"
	InjuredIntent = "injured"
	AwayIntent    = "away"
	WinIntent     = "win"

	ShootIntent    = "shoot"
	FioShootIntent = "fioShoot"
)

const (
	stopGame = "(* (давай (хватит|закончим)|хватит играть|надоело|(больше не (хочу|хочется))|" +
		"прекращ*|прекрат*|стоп|останов*|заканчивай|заканчиваю|[давай] (закончим|закончи|закончить) [игр*]|устал* игр*|" +
		"(не (хочу|хочется) [больше] играть)|надоел|надоела|мне надоело играть|" +
		"((не нравится) * игра)|(выйти|выход) из * игры)*|" +
		"(конец|хватит|[я] устал*|выход|выйти|сдаюсь))"
)

var GlobalCommands = []sdk.Pattern{
	{
		Name: StartIntent,
		Pattern: "[алис*] * [поигра*|давай|хоч*] [игра*] * морск* бо* *|" +
			"* ((поиграю|поиграем|поиграй|поиграть)|(играю|играем|играй|играть)|(сыграю|сыграем|сыграй|сыграть)|играм) в морск* бо* * |" +
			"* (включи|активируй) морск* бо* * |",
	},
	{
		Name:    RestartIntent,
		Pattern: "* [поигра*|давай|хоч*] [игра*] (сначал*|занов*|еще раз|по нов*) *",
	},
	{
		Name: RulesIntent,
		Pattern: "* (как * играть|[забы*] правил*|не понял*) *|" +
			"* (правила|как играть|что делать|как дальше) * |" +
			" (и че|и что|и как)",
	},
	{
		Name:    WaitIntent,
		Pattern: "* [я|еще] (ща|щас|сейчас|подожди|думаю|в процессе|не торопи*|подума*) *",
	},
	{
		Name:    SurrenderIntent,
		Pattern: "* сдаться | сдаю* *",
	},
	{
		Name: EndIntent,
		Pattern: fmt.Sprintf("* %s *|"+
			"* [давай] * перерыв *|"+
			"[я] устал*", stopGame),
	},
}

var SelectCommands = []sdk.Pattern{
	{
		Name:    YesIntent,
		Pattern: "[ну] (да|конечно|канешн|ага|угу|еп|yes|yep|sure|соглас*|обязател*|ладно|ок|окей|да давай)",
	},
	{
		Name:    NoIntent,
		Pattern: "[ну] (нет|не хочу|не|no|nope|пожалуй нет|не а|да не|да нет|ну нафиг|нет наверное|наверное нет|не надо)",
	},
}

var MyShootCommands = []sdk.Pattern{
	{
		Name:    KillIntent,
		Pattern: "* уби*| * доби*",
	},
	{
		Name:    InjuredIntent,
		Pattern: "* ран* *",
	},
	{
		Name:    AwayIntent,
		Pattern: "* мим*|не попал* *",
	},
	{
		Name: WinIntent,
		Pattern: "* [ты|вы] (выиграл*|победил*) *|" +
			"* [я] проиграл* *",
	},
}

var UserShootCommands = []sdk.Pattern{
	{
		Name:    ShootIntent,
		Pattern: "* $Cell *",
	},
	{
		Name:    FioShootIntent,
		Pattern: "* $FIO $NUMBER *",
	},
}

func GetCells() (cells map[string]map[string][]string, err error) {
	if err = json.Unmarshal(resource.Get("cells.json"), &cells); err != nil {
		return nil, xerrors.Errorf("Invalid resource")
	}
	return cells, nil
}
