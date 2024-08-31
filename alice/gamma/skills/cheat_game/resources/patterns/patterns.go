package patterns

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

const (
	StartGameIntent = "start"
	EndGameIntent   = "end"

	YesStartGameIntent = "yes"
	NoStartGameIntent  = "no"

	TruthAnswerIntent = "truth"
	LiesAnswerIntent  = "lies"
	PassAnswerIntent  = "pass"
	OtherAnswerIntent = "other"
)

var StartGameCommands = []sdk.Pattern{
	{
		Name:    StartGameIntent,
		Pattern: "* [давай|может] * (сыграем|игра*|игру|сыгра*|включи|активируй|~начать) * [в] верю не верю *",
	},
	{
		Name: YesStartGameIntent,
		Pattern: "* ((ага|угу|давай|да|хорошо|конечно|играем) * |" +
			"* (начин*|начн*|попытаюсь|попробую|поехали|го|гоу|погнали|продолж*|готов*|давай|валяй|реди|рэди|камон|сыграем|сыграю|играем|играю) (продолж*|снова|заново)) *",
	},
	{
		Name: NoStartGameIntent,
		Pattern: "* ((не надо|не хочу|неа|нет|не)|" +
			"(не [могу] сейчас|мне (некогда|не до *того)|(в другой|не в этот) раз |" +
			"[давай] (*позже|потом|не сегодня|завтра|не сейчас|пока не надо|в (следующий|другой) раз) [поговорим] |" +
			"нет времени|я (занят|занята)|не готов|не готова)) *",
	},
}

var QuestionCommands = []sdk.Pattern{
	{
		Name:    TruthAnswerIntent,
		Pattern: "* (ага|угу|да|конечно) * | (верю|поверю|правд*|истин*|верится|вериться|вероятно) *",
	},
	{
		Name: LiesAnswerIntent,
		Pattern: "*" + "(неа|нет|не)|" +
			"(не (верю|поверю|правд*|правильно|верится|вериться|было (такого/этого)|вероятно)|" +
			"неверю|неправд*|неправильно|невероятно|ложь|вранье|обман*|не может * быть|вряд ли|" +
			"сомневаюсь|не бывает|фейк|лажа|липа|брехня|фигня|хрень|чушь|брешешь) *",
	},
	{
		Name:    PassAnswerIntent,
		Pattern: "* [*давай] (следующ*|друго*|еще) (вопрос|факт|спроси) *",
	},
	{
		Name:    OtherAnswerIntent,
		Pattern: "*",
	},
}

var GlobalCommands = []sdk.Pattern{{
	Name: EndGameIntent,
	Pattern: "* (перерыв|хватит|перестань|замолчи|остановись) * |" +
		" [я] устал* |" +
		" * (сегодня (хватит|достаточно|все) [хватит] [достаточно]) * |" +
		" Алиса, хватит.",
}}
