package patterns

import (
	"fmt"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

const (
	StartGameIntent = "start"
	EndGameIntent   = "end"

	YesStartGameIntent = "yes"
	NoStartGameIntent  = "no"

	AnswerIntent                = "answer"
	InvalidAnswerIntent         = "invalid"
	GetAnswerIntent             = "get_answer"
	UserWantsToThinkIntent      = "want_think"
	UserDoesntWantToThinkIntent = "dont_want"
	RepeatQuestionIntent        = "repeat"
	GetHintIntent               = "hint"
	UnsuccessfulGetHintIntent   = "badhint"
	MeaningIntent               = "mean"
	ChangeLevelUpIntent         = "levelup"
	ChangeLevelDownIntent       = "leveldown"
)

const (
	stopGame = "(* (давай (хватит|закончим)|хватит играть|надоело|(больше не (хочу|хочется))|" +
		"прекращ*|прекрат*|стоп|останов*|заканчивай|заканчиваю|[давай] (закончим|закончи|закончить) [игр*]|устал* игр*|" +
		"(не (хочу|хочется) [больше] играть)|надоел|надоела|мне надоело играть|" +
		"((не нравится) * игра)|(выйти|выход) из * игры)*|" +
		"(конец|хватит|[я] устал*|выход|выйти|сдаюсь))"
	agree = "(* (давай|конечно|конешно|канешна|а то [нет]|очень|[ты] прав*|абсолютно|обязательно|непременно|а как же|" +
		"подтверждаю|точно|пожалуй|запросто|норм|(почему/что/че) [бы] [и] нет|хочу|было [бы] (неплохо|не плохо)|" +
		"[([ну] [конечно|всё|все|вроде|пожалуй|возможно] (да|даа|lf|ага|точно|угу|верно|ок|ok|окей|окай|okay|оке|именно|" +
		"подтвержд*|йес) [да|конечно|конешно|канешна|всё|все|вроде|пожалуй|возможно]|очень)] (хочу|хо чу|ладно|хорошо|" +
		"хoрoшo|можно|валяй*|договорились|согла*|вполне|в полной мере|" +
		"естественно|разумеется|(еще|ещё) как|не (против|возражаю|сомневаюсь)|я только за|безусловн*|" +
		"[это] так [и есть]|давай*) [(конечно|конешно|канешна)]|[все] правильно|все так) * |(логично|могу|было дело|" +
		"бывало|бывает)|[ну] [конечно|всё|все|вроде|пожалуй|возможно] (да|даа|lf|ага|точно|угу|верно|ок|ok|окей|" +
		"окай|okay|оке|именно|подтвержд*|йес) [да|конечно|конешно|канешна|всё|все|вроде|пожалуй|возможно])"
	startGame = "(начин*|начн*|попытаюсь|попробую|поехали|го|гоу|погнали|продолж*|готов*|давай|валяй|реди|рэди|камон|" +
		"сыграем|сыграю|играем|играю)"
	disagree = "(* [((нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо]|конечно|конешно|канешна)] " +
		"((нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо]|не сейчас|ни капли|отнюдь|нискол*|да ладно|не" +
		" (хоч*|хо чу|надо|могу|очень|думаю|нравится|стоит|буду|считаю|согла*|подтв*)|ненадо|нельзя|нехочу|ненавижу|" +
		"невозможно|никогда|никуда|ни за что|нисколько|никак*|никто|ниразу|[я] против|вряд ли|сомневаюсь|нихрена|" +
		"неправильно|неверно|невсегда|[это] (не так)|отказываюсь) [(конечно|конешно|канешна|спасибо)]" +
		" *|(да ну|не|нее|ничего)|(нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо])"
	notNow = "(не [могу] сейчас|мне (некогда|не до *того)|(в другой|не в этот) раз|[давай] (*позже|потом|не сегодня|" +
		"не сейчас|пока не надо) [поговорим]|нет времени|я (занят|занята))"
	dontKnow = "[([я] даже|ну [я]|сам*)] ((не|откуда [(у|про|для|за|из [за]|из-за|в|без|до|через|с|со|об|от|к|ко|о|обо|" +
		"об|при|по|на|с|над|под|перед)] (я|меня|мне|мной|мой|моя|мою|мое*|моим|моём|моей|" +
		"себ*|мня|мну)) (зна*|задумывал*|думал*|решил*|представляю|уверен*|помню)|" +
		"незнаю|неизвестно|надо подумать|без понятия|без понятий|понятия не имею|сомневаюсь|сложны* вопрос*|" +
		"сложно сказать|все сложно|ну ты [и]спросил|спросишь тоже|забыл*|сдаюсь)"
	yes = "[ну] [конечно|всё|все|вроде|пожалуй|возможно] (да|даа|lf|ага|точно|угу|верно|ок|ok|окей|окай|okay|оке|" +
		"именно|подтвержд*|йес) [да|конечно|конешно|канешна|всё|все|вроде|пожалуй|возможно]"
	no    = "(нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо]"
	maybe = "((может [быть]|наверн*|возможн*|вероятн*|скорее всего|вроде [бы|да]) [тогда])"
	sure  = "(конечно|канешна|конешно|безусловно|очевидно|логично|точно) [же]"
)

var GlobalCommands = []sdk.Pattern{
	{
		Name: StartGameIntent,
		Pattern: "* ([хочу|давай] (играть|сыграть|поиграть|игру|сыграем|поиграем) * [в|про] ([найди|найти|выбери] лишнее))" +
			" *|* [хочу|давай] * [в|про] * (лишнее слово|найди лишнее|выбери лишнее) *",
	},
	{
		Name:    EndGameIntent,
		Pattern: fmt.Sprintf("* (%s|(не хочу|надоело|не буду) * играть|отстань) *", stopGame),
	},
}

var StartGameCommands = []sdk.Pattern{
	{
		Name:    YesStartGameIntent,
		Pattern: fmt.Sprintf("* (%s|%s|говори) *", agree, startGame),
	},
	{
		Name:    NoStartGameIntent,
		Pattern: fmt.Sprintf("* (%s|%s|%s) *", disagree, notNow, stopGame),
	},
}

var QuestionCommands = []sdk.Pattern{
	{
		Name:    MeaningIntent,
		Pattern: "* ((что|кто) [это] [такая|такое|такой] * $Word) *",
	},
	{
		Name:    InvalidAnswerIntent,
		Pattern: fmt.Sprintf("* [%s|%s] * ($Word|$Number) * ($Word|$Number) * [%s|%s]", maybe, sure, maybe, sure),
	},
	{
		Name:    AnswerIntent,
		Pattern: fmt.Sprintf("* [%s|%s] * ($Word|$Number) * [%s|%s]", maybe, sure, maybe, sure),
	},
	{
		Name: GetAnswerIntent,
		Pattern: fmt.Sprintf("* (%s|не хочу|не буду|не надо|(скажи|какой) * ответ|сам (ответь|отвечай)|[давай]"+
			" (дальше|(следующий|следующую|следующая|другой|другая|другую) (вопрос|загадка|загадку))|хз) *", dontKnow),
	},
	{
		Name:    UserWantsToThinkIntent,
		Pattern: fmt.Sprintf("* (%s|%s|подумаю|может) *", yes, startGame),
	},
	{
		Name:    UserDoesntWantToThinkIntent,
		Pattern: fmt.Sprintf("* (%s|не подумаю) *", no),
	},
	{
		Name: RepeatQuestionIntent,
		Pattern: fmt.Sprintf("([повтори|скажи|прочитай|какие были] [еще [раз]] (список|запрос|вопрос|варианты|"+
			"слова|список слов))|") +
			"((повтори|скажи|прочитай) [еще [раз]])|[* повтори] еще раз|* (*медленнее|(очень|слишком) быстро) *",
	},
	{
		Name: ChangeLevelUpIntent,
		Pattern: "* (((посложнее|сложнее) [уровень|вопрос|вопросы])|(уровень (выше|повыше|следующий))) *|" +
			"* (другой|другую|другая) (загадка|загадку|вопрос) * [слишком] (легкий|легко|легкая|легкое) *|" +
			"* (([это|мне] * [слишком] * (просто|легко))) *|" +
			"* еще (сложнее|посложнее) *",
	},
	{
		Name: ChangeLevelDownIntent,
		Pattern: "* (((попроще|полегче|проще|легче) [уровень|вопрос|вопросы])|(уровень (ниже|пониже))) *|" +
			"* (другой|другую|другая) (загадка|загадку|вопрос) * [слишком] (сложный|сложное|сложно|сложные) *|" +
			"* (([это|мне] * [слишком] * (сложно|трудно))) *|" +
			"* еще (легче|полегче|проще|попроще) *",
	},
	{
		Name: UnsuccessfulGetHintIntent,
		Pattern: "* [не знаю] * ((другой|другая|другую|по-другому|еще) * (подскажи|намекни|помоги|подсказка|подсказку)) *|" +
			"* [не знаю] * (еще) * (подскажи|намекни|помоги|подсказка|подсказку) *",
	},
	{
		Name: GetHintIntent,
		Pattern: "* [не знаю] * ([повтори|еще раз] * (подскажи|намекни|помоги|подсказка|подсказку)) *|" +
			"* повтори * (подсказка|подсказку|подсказывать) *",
	},
}