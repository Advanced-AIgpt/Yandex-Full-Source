package patterns

import (
	"fmt"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

const (
	StartIntent = "start"
	RulesIntent = "rules"
	NoIntent    = "no"
	EndIntent   = "end"

	YesIntent = "yes"

	NotQuestionIntent = "noq_question"
	NotReadyIntent    = "not_ready"
)

const (
	yes       = "[ну] (да|конечно|канешн|ага|угу|еп|yes|yep|sure|соглас*|обязател*|ладно|ок|окей|да давай)"
	no        = "[ну] (нет|не хочу|не|no|nope|пожалуй нет|не а|да не|да нет|ну нафиг|нет наверно|наверное нет|не надо)"
	startGame = "(начин*|начн*|попытаюсь|попробую|поехали|го|гоу|погнали|продолж*|готов*|давай|валяй|реди|рэди|камон|" +
		"сыграем|сыграю|играем|играю)"
)

var GlobalCommands = []sdk.Pattern{
	{
		Name: StartIntent,
		Pattern: "[алис*] * (сыгра*|поигра*|принят*|играт*) * решен*|" +
			"* [алис*] * (отвеча*|поотвеча*|ответь) * вопрос*|" +
			"* (магическ*|меджик*|мэджик*) (бол*|шар) *|" +
			"* шар (предсказаний|судьбы) *",
	},
	{
		Name: RulesIntent,
		Pattern: "(как игра*|правила|что дела*|запутал*|суть игр*) *|" +
			"[а] что [тут|я|мне] (надо|нужно|должен|должн*) [мне] (сделать|делать) *|" +
			"[блин] [я] запутал* *|" +
			"* [больше] не игра* *|" +
			"(как * играть|[забы*] правил*|не понял*)",
	},
	{
		Name:    NoIntent,
		Pattern: no,
	},
	{
		Name: EndIntent,
		Pattern: fmt.Sprintf("(хватит|стоп|надоел*|останови*|стой|прекрати|завязывай*|закончили)|"+
			"%s *|"+
			"[с меня] (стоп|хватит|достаточно|закончить) [на сегодня]|"+
			"[мне|я] (погоди|потом|позже|надоело|устал*)|"+
			"закончить [игр*]|"+
			"не готов*", no),
	},
	{
		Name: NotReadyIntent,
		Pattern: "(подожди|сек*|позже)|" +
			"(сейчас|дай время) [я|мы] (подумаю|подумаем)",
	},
}

var SelectCommands = []sdk.Pattern{
	{
		Name:    YesIntent,
		Pattern: yes,
	},
}

var QuestionCommands = []sdk.Pattern{
	{
		Name:    NotQuestionIntent,
		Pattern: fmt.Sprintf("%s|%s|%s", yes, no, startGame),
	},
	{
		Name: NotReadyIntent,
		Pattern: "(подожди|сек*|позже)|" +
			"(сейчас|дай время) [я|мы] (подумаю|подумаем)",
	},
}
