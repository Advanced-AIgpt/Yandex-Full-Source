package game

import (
	"log"
	"math/rand"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/questions"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/replies"
)

const numQuestions = 5

type Game struct {
	questions      []questions.Question
	repliesManager replies.Manager
	random         rand.Rand
}

func NewCheatGame(random rand.Rand) *Game {
	game := &Game{
		random:         random,
		repliesManager: replies.Manager{RepliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates)},
	}
	var err error
	if game.questions, err = questions.GetQuestions(); err != nil {
		log.Fatal(err)
	}
	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
	if err := dialog.SayTemplate(startGameTemplate, struct{ IsStation bool }{dialoglib.IsYandexStation(meta)}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.StartGameButtons...)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *Game) StateStartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.StartGameCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesStartGameIntent:
			yesStartTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.YesStartGameReply)
			if err := dialog.SayTemplate(yesStartTemplate, nil); err != nil {
				return err
			}
			ctx.GameState = replies.QuestionState
			ctx.RoundState = RoundState{}
			return game.AskQuestion(ctx, dialog)
		case patterns.NoStartGameIntent:
			noStartTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NoStartGameReply)
			if err := dialog.SayTemplate(noStartTemplate, nil); err != nil {
				return err
			}
			return game.EndGame(ctx, dialog)
		}
	}
	return game.Fallback(dialog)
}

func (game *Game) StateQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.QuestionCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch answer := matches[0].Name; answer {
		case patterns.TruthAnswerIntent:
			fallthrough
		case patterns.LiesAnswerIntent:
			lastQuestion := game.questions[ctx.RoundState.LastQuestionIndex]
			if answer == patterns.TruthAnswerIntent {
				dialog.Say(&lastQuestion.YesAnswer)
			} else {
				dialog.Say(&lastQuestion.NoAnswer)
			}
			if lastQuestion.IsCorrect(answer) {
				ctx.RoundState.RightAnswers += 1
			}
			if ctx.RoundState.CurQuestion < numQuestions {
				nextQuestionTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.NextQuestionReply)
				if err := dialog.SayTemplate(nextQuestionTemplate, nil); err != nil {
					return err
				}
				return game.AskQuestion(ctx, dialog)
			} else {
				if err = game.EndRound(ctx, dialog); err != nil {
					return err
				}
				return game.AnotherRound(ctx, dialog)
			}
		case patterns.PassAnswerIntent:
			nextQuestionTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.NextQuestionReply)
			if err := dialog.SayTemplate(nextQuestionTemplate, nil); err != nil {
				return err
			}
			return game.AskQuestion(ctx, dialog)
		case patterns.OtherAnswerIntent:
			return game.RepeatQuestion(ctx, dialog)
		}
	} else {
		return game.Fallback(dialog)
	}
	return nil
}

func (game *Game) AskQuestion(ctx *Context, dialog *dialoglib.Dialog) error {
	if index, question, end := ctx.RandomQuestion(game.random, game.questions); !end {
		dialog.Say(&question.Fact)
		dialog.AddButtons(buttons.QuestionButtons...)

		ctx.UsedQuestionIndexes[index] = true
		ctx.RoundState.LastQuestionIndex = index
		ctx.RoundState.CurQuestion += 1
		ctx.GameState = replies.QuestionState
	} else {
		if err := game.EndRound(ctx, dialog); err != nil {
			return err
		}
		return game.RestartGame(ctx, dialog)
	}
	return nil
}

func (game *Game) RepeatQuestion(ctx *Context, dialog *dialoglib.Dialog) error {
	questionFallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.QuestionFallbackReply)
	if err := dialog.SayTemplate(questionFallbackTemplate, nil); err != nil {
		return err
	}
	dialog.Say(&game.questions[ctx.RoundState.LastQuestionIndex].Fact)
	dialog.AddButtons(buttons.QuestionButtons...)
	ctx.GameState = replies.QuestionState
	return nil
}

func (game *Game) Fallback(dialog *dialoglib.Dialog) error {
	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
		return err
	}
	return nil
}

func (game *Game) EndRound(ctx *Context, dialog *dialoglib.Dialog) error {
	roundResultsTemplate := game.repliesManager.ChooseRoundResultsCueTemplate(ctx.RoundState.RightAnswers)
	if err := dialog.SayTemplate(roundResultsTemplate, ctx.RoundResults()); err != nil {
		return err
	}
	return nil
}

func (game *Game) AnotherRound(ctx *Context, dialog *dialoglib.Dialog) error {
	anotherGameTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.AnotherGameReply)
	if err := dialog.SayTemplate(anotherGameTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.StartGameButtons...)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *Game) EndGame(ctx *Context, dialog *dialoglib.Dialog) error {
	endGameTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndGameReply)
	if err := dialog.SayTemplate(endGameTemplate, nil); err != nil {
		return err
	}
	dialog.EndSession()
	ctx.GameState = replies.EndGameState
	return nil
}

func (game *Game) RestartGame(ctx *Context, dialog *dialoglib.Dialog) error {
	restartGameTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.RestartGameReply)
	if err := dialog.SayTemplate(restartGameTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.StartGameButtons...)
	ctx.UsedQuestionIndexes = make(map[int]bool)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *Game) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return false, err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.EndGameIntent:
			return true, game.EndGame(ctx, dialog)
		}
	}
	return false, nil
}

func (game *Game) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	dialog := &dialoglib.Dialog{}

	if ctx.IsNewSession() {
		ctx.GameState = replies.GeneralState
		if err := game.StateGeneral(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog); err != nil {
		return nil, err
	} else if isGlobalCommand {
		return dialog.BuildResponse()
	}

	var err error
	switch ctx.GameState {
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.StartGameState:
		err = game.StateStartGame(log, ctx, request, meta, dialog)
	case replies.QuestionState:
		err = game.StateQuestion(log, ctx, request, meta, dialog)
	default:
		err = game.Fallback(dialog)
	}

	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}
