package patterns

import (
	"fmt"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

const (
	StartGameIntent      = "start"
	UserMakesGuessIntent = "makeGuess"
	ExitIntent           = "exit"
	NotReadyIntent       = "notReady"
	HowToPlayIntent      = "howToPlay"

	YesStartGameIntent = "yes"
	NoStartGameIntent  = "no"

	AnswerIntent                = "guess actor"
	LetUserThinkIntent          = "letThink"
	UserDoesntKnowIntent        = "userDoesntKnow"
	NextHintIntent              = "nextHint"
	SayAnswerIntent             = "sayAnswer"
	UserWantsToThinkIntent      = "noSay"
	UserDoesntWantToThinkIntent = "yesSay"
	RepeatHintIntent            = "repeatHint"
)

const (
	yes       = "[ну] (да|конечно|канешн|ага|угу|еп|yes|yep|sure|соглас*|обязател*|ладно|ок|окей|да давай)"
	startGame = "(начин*|начн*|попытаюсь|попробую|поехали|го|гоу|погнали|продолж*|готов*|давай|валяй|реди|рэди|камон|сыграем|сыграю|играем|играю)"
	no        = "[ну] (нет|не хочу|не|no|nope|пожалуй нет|не а|да не|да нет|ну нафиг|нет наверное|наверное нет|не надо)"
	howToPlay = "(как * играть|[забы*] правил*|не понял*)"
	wait      = "(подожди|сек*|позже)"
	stop      = "(хватит|стоп|надоел*|останови*|стой|прекрати|завязывай*|закончили)"
	stopGame  = "(* (давай (хватит|закончим)|хватит играть|надоело|(больше не (хочу|хочется))|" +
		"прекращ*|прекрат*|стоп|останов*|заканч*|[давай] законч* [игр*]|устал* игр*|" +
		"(не (хочу|хочется) [больше] играть)|надоел|надоела|мне надоело играть|" +
		"((не нравится) * игра)|(выйти|выход) из * игры)*|" +
		"(конец|хватит|[я] устал*|выход|выйти|сдаюсь))"
)

var GlobalCommands = []sdk.Pattern{
	{
		Name: StartGameIntent,
		Pattern: "[алис*] * (загадай|загадыва*) * (актер*|актрис*) *|" +
			"[алис*] * [поигра*|давай|хоч*] [игра*] * (угадыв*|угад*) * (актер*|актрис*)|" +
			"* акинатор* *",
	},
	{
		Name: UserMakesGuessIntent,
		Pattern: "* [теперь] я хочу загадать *|" +
			"* можно я [тоже|тебе] загадаю *|" +
			"* [теперь] я загадыв* *",
	},
	{
		Name: ExitIntent,
		Pattern: fmt.Sprintf("* да что * (хочешь * меня) *|"+
			"* погоди *|"+"* (%s|может позже|потом)|"+
			"* (не хочу|не буду) * играть *|"+
			"* %s *", stop, stopGame),
	},
	{
		Name:    NotReadyIntent,
		Pattern: fmt.Sprintf("* %s *", wait),
	},
	{
		Name:    HowToPlayIntent,
		Pattern: fmt.Sprintf("* %s *", howToPlay),
	},
}

var StartGameCommands = []sdk.Pattern{
	{
		Name: YesStartGameIntent,
		Pattern: fmt.Sprintf("* (%s|%s|играем|сыграем*) *|"+
			"* ([если] * настаиваеш*|попроб*) *|"+
			"* (последн* [раз]|допустим|можно|нужно|удовольст*|разумеетс*|загадывай) *|"+
			"* (готов*|давай|игра*|поигра*|начн*|го|гоу) *", yes, startGame),
	},
	{
		Name:    NoStartGameIntent,
		Pattern: fmt.Sprintf("* %s *", no),
	},
}

var GuessCommands = []sdk.Pattern{
	{
		Name:    AnswerIntent,
		Pattern: "* [мб|может быть|не знаю|думаю|как насчет|мне кажетс*|мой вариант] $Actor *",
	},
	{
		Name:    RepeatHintIntent,
		Pattern: "* (повтори*|скажи еще раз) *",
	},
	{
		Name: UserDoesntKnowIntent,
		Pattern: "* (хз|не знаю|дальше|подсказк*|не смотрел*|смотрел* только|первый раз слышу|хм|без поняти*) *|" +
			"* (забыл*|запамятовал*|выскочи* * голов*|не припомина*|не помню|не [могу] вспомн*|таких много|нет иде*|откуда [я|мне] зна*) *",
	},
	{
		Name: SayAnswerIntent,
		Pattern: "* (скажи|хочу знать|хочу|говори|какой) * (ответ|актер*|актрис*) *|" +
			"* (ответ|актер*|актрис*) * (скажи|хочу знать|хочу|говори) *|" +
			"* (кто это|ответь|колись) *|" +
			"* (сдаюсь|больше не хочу) *|" +
			"* [интерес*]  узнать * ответ *",
	},
	{
		Name: UserDoesntWantToThinkIntent,
		Pattern: fmt.Sprintf("* %s *|", no) +
			fmt.Sprintf("* (%s|повтор*) * (скажи|хочу знать|хочу) *  (ответ|актер*|актрис*)|", no) +
			fmt.Sprintf("* (%s|повтор*) * (кто это|ответь)|", no) +
			fmt.Sprintf("* (%s|повтор*) * (сдаюсь|больше не хочу)|", no) +
			"* скажи ответ *",
	},
	{
		Name:    LetUserThinkIntent,
		Pattern: "* [я|еще] (ща|щас|сейчас|подожди|думаю|в процессе|не торопи*|подума*) *",
	},
	{
		Name:    UserWantsToThinkIntent,
		Pattern: fmt.Sprintf("* (%s|%s) *", yes, startGame),
	},
	{
		Name:    NextHintIntent,
		Pattern: "еще|еще подсказка",
	},
}
