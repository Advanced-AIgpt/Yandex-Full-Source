package game

import (
	"log"
	"math/rand"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/resources/replies"
)

const FirstUncertainAnswer = 15

type Game struct {
	random         rand.Rand
	repliesManager dialoglib.RepliesManager
	replies        []dialoglib.Cue
}

func NewMagicBallGame(random rand.Rand) *Game {
	game := &Game{
		random:         random,
		repliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates),
	}

	var err error
	if game.replies, err = replies.GetReplies(); err != nil {
		log.Fatal(err)
	}

	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	startReply := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartReply)
	if err := dialog.SayTemplate(startReply, struct {
		IsStation bool
	}{
		IsStation: dialoglib.IsYandexStation(meta),
	}); err != nil {
		return nil
	}

	waitQuestionReply := game.repliesManager.ChooseCueTemplate(replies.SelectState, replies.QuestionReply)
	if err := dialog.SayTemplate(waitQuestionReply, nil); err != nil {
		return err
	}

	dialog.AddButtons(buttons.RulesButton)
	ctx.GameState = replies.QuestionState
	ctx.UserWantsRules = false

	return nil
}

func (game *Game) StateQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.QuestionCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.NotQuestionIntent:
			notQuestionReply := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.NotQuestionReply)
			return dialog.SayTemplate(notQuestionReply, nil)
		}
	}

	return game.SayPrediction(log, ctx, request, meta, dialog)
}

func (game *Game) StateSelect(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.SelectCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesIntent:
			ctx.UserWantsRules = false
			dialog.AddButtons(buttons.RulesButton)
			questionReply := game.repliesManager.ChooseCueTemplate(replies.SelectState, replies.QuestionReply)
			ctx.GameState = replies.QuestionState
			return dialog.SayTemplate(questionReply, nil)
		}
	}

	if !ctx.UserWantsRules {
		return game.SayPrediction(log, ctx, request, meta, dialog)
	}

	return game.FallBack(log, ctx, request, meta, dialog)
}

func (game *Game) StateEnd(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	endReply := game.repliesManager.ChooseCueTemplate(replies.EndState, replies.EndReply)
	dialog.EndSession()
	if err := dialog.SayTemplate(endReply, nil); err != nil {
		return err
	}
	return nil
}

func (game *Game) FallBack(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	fallbackReply := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	return dialog.SayTemplate(fallbackReply, nil)
}

func (game *Game) GetPrediction(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	predictionReply := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.PredictionReply)

	index := game.random.Intn(len(game.replies))
	if err := dialog.SayTemplate(predictionReply, struct {
		Prediction dialoglib.Cue
	}{
		Prediction: game.replies[index],
	}); err != nil {
		return err
	}

	if index < FirstUncertainAnswer {
		var reply *dialoglib.CueTemplate

		if game.random.Intn(3) < 1 && ctx.LastNotUsedFact != -1 {
			index = game.random.Intn(ctx.LastNotUsedFact + 1)

			reply = game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.RareReply)
			ctx.RareFacts[index], ctx.RareFacts[ctx.LastNotUsedFact] = ctx.RareFacts[ctx.LastNotUsedFact], ctx.RareFacts[index]
			ctx.LastNotUsedFact--
			return dialog.SayTemplate(reply, struct {
				Fact string
			}{
				Fact: ctx.RareFacts[ctx.LastNotUsedFact+1],
			})
		}
		reply = game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.LessRareReply)
		return dialog.SayTemplate(reply, nil)
	}

	return nil
}

func (game *Game) SayPrediction(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if err := game.GetPrediction(log, ctx, request, meta, dialog); err != nil {
		return err
	}

	continueReply := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.AskToContinueReply)
	if err := dialog.SayTemplate(continueReply, nil); err != nil {
		return err
	}

	dialog.AddButtons(buttons.YesButton, buttons.RulesButton)
	ctx.GameState = replies.SelectState
	return nil
}

func (game *Game) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, &sdk.EmptyEntityExtractor)

	if err != nil {
		return false, err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.StartIntent:
			ctx.GameState = replies.GeneralState
			return true, game.StateGeneral(log, ctx, request, meta, dialog)
		case patterns.RulesIntent:
			rulesReply := game.repliesManager.ChooseCueTemplate(replies.QuestionReply, replies.RulesReply)
			ctx.GameState = replies.SelectState
			ctx.UserWantsRules = true
			dialog.AddButtons(buttons.YesButton)
			return true, dialog.SayTemplate(rulesReply, struct {
				IsStation bool
			}{
				IsStation: dialoglib.IsYandexStation(meta),
			})
		case patterns.NoIntent:
			if ctx.GameState != replies.QuestionState {
				return game.finishGame(log, ctx, request, meta, dialog)
			}
		case patterns.NotReadyIntent:
			if ctx.UserWantsRules {
				return false, nil
			}
			notReadyReply := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UserNotReadyReply)
			return true, dialog.SayTemplate(notReadyReply, nil)
		case patterns.EndIntent:
			return game.finishGame(log, ctx, request, meta, dialog)
		}
	}

	return false, nil
}

func (game *Game) finishGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	ctx.GameState = replies.EndState
	return true, game.StateEnd(log, ctx, request, meta, dialog)
}

func (game *Game) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	dialog := &dialoglib.Dialog{}
	if ctx.IsNewSession() {
		ctx.GameState = replies.GeneralState
		ctx.RareFacts = replies.GetRareFacts()
		ctx.LastNotUsedFact = len(ctx.RareFacts) - 1
		ctx.UserWantsRules = false

		if err := game.StateGeneral(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if ctx.GameState == replies.EndState {
		if err := game.FallBack(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog); err != nil {
		return nil, err
	} else if isGlobalCommand {
		return dialog.BuildResponse()
	}

	log.Debugf("new state: %s", ctx.GameState)

	var err error
	switch ctx.GameState {
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.SelectState:
		err = game.StateSelect(log, ctx, request, meta, dialog)
	case replies.QuestionState:
		err = game.StateQuestion(log, ctx, request, meta, dialog)
	default:
		err = game.FallBack(log, ctx, request, meta, dialog)
	}

	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}
