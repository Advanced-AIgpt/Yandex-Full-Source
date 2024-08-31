package game

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/replies"
)

type Question struct {
	Right   dialoglib.Cue
	Wrong   dialoglib.Cue
	KeyWord dialoglib.Cue
	Value   interface{}
	Order   string
	Buttons []sdk.Button
}

func (game *Game) SayQuestion(ctx *Context, dialog *dialoglib.Dialog) error {
	dialog.AddButtons(ctx.RoundState.CurrentQuestion.Buttons...)
	questionTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, ctx.RoundState.CurrentType)
	return dialog.SayTemplate(questionTemplate, struct {
		Question *Question
	}{
		Question: ctx.RoundState.CurrentQuestion,
	})
}
