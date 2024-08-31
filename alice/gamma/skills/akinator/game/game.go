package game

import (
	"log"
	"math/rand"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/actors"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/replies"
)

const CountOfHints = 5

type Game struct {
	Actors          []actors.Actor
	entityExtractor *sdk.EntityExtractor
	repliesManager  dialoglib.RepliesManager
	random          rand.Rand
}

func NewAkinatorGame(random rand.Rand) *Game {
	game := &Game{
		random:         random,
		repliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates),
	}
	var err error
	if game.Actors, err = actors.GetActors(); err != nil {
		log.Fatal(err)
	}
	actorsPatterns, err := actors.GetPatterns()
	if err != nil {
		log.Fatal(err)
	}
	game.entityExtractor = sdk.NewEntityExtractor(actorsPatterns)
	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	log.Debugf("State general")
	startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
	if err := dialog.SayTemplate(startGameTemplate, struct{ IsStation bool }{dialoglib.IsYandexStation(meta)}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.StartButtons...)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *Game) StateStartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	log.Debugf("State start game")
	matches, err := ctx.Match(request, patterns.StartGameCommands, game.entityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesStartGameIntent:
			if ctx.CountOfUsedActors != len(game.Actors) {
				yesStartTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.YesStartReply)
				if err = dialog.SayTemplate(yesStartTemplate, nil); err != nil {
					return err
				}
			}
			ctx.GameState = replies.GuessState
			return game.SayHint(log, ctx, request, meta, dialog)
		case patterns.NoStartGameIntent:
			ctx.GameState = replies.EndGameState
			return game.StateExitGame(log, ctx, request, meta, dialog)
		}
	}
	ctx.GameState = replies.GeneralState
	return game.MisUnderstand(log, ctx, request, meta, dialog)
}

func (game *Game) StateGuess(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	log.Debugf("Guess state")
	matches, err := ctx.Match(request, patterns.GuessCommands, game.entityExtractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		if ctx.RoundState.WantsKnowAnswer {
			switch matches[0].Name {
			case patterns.UserWantsToThinkIntent:
				noSayAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.DontSayAnswerReply)
				if err = dialog.SayTemplate(noSayAnswerTemplate, nil); err != nil {
					return err
				}
				ctx.RoundState.WantsKnowAnswer = false
				return game.sayCurrentHint(ctx, dialog)
			case patterns.UserDoesntWantToThinkIntent:
				fallthrough
			case patterns.SayAnswerIntent:
				return game.SayAnswer(log, ctx, request, meta, dialog)
			}
		}
		ctx.RoundState.WantsKnowAnswer = false

		switch matches[0].Name {
		case patterns.AnswerIntent:
			log.Debugf("Valid answer, check for correct")
			if len(matches[0].Variables["Actor"]) > 0 && matches[0].Variables["Actor"][0] == game.Actors[ctx.RoundState.LastActorIndex].ID {
				successTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.CorrectAnswerReply)
				if err = dialog.SayTemplate(successTemplate, nil); err != nil {
					return err
				}
				ctx.GameState = replies.EndRoundState
				return game.StateEndRound(log, ctx, request, meta, dialog)
			}
			wrongAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.WrongAnswerReply)
			if err = dialog.SayTemplate(wrongAnswerTemplate, nil); err != nil {
				return err
			}
			return game.SayHint(log, ctx, request, meta, dialog)
		case patterns.RepeatHintIntent:
			return game.sayCurrentHint(ctx, dialog)
		case patterns.LetUserThinkIntent:
			letUserThinkTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.LetUserThinkReply)
			if err = dialog.SayTemplate(letUserThinkTemplate, nil); err != nil {
				return err
			}
			dialog.AddButtons(buttons.LetThinkButtons...)
			return nil
		case patterns.SayAnswerIntent:
			if ctx.RoundState.WantsKnowAnswer {
				return game.SayAnswer(log, ctx, request, meta, dialog)
			}
			sayAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.AskAnswerReply)
			if err = dialog.SayTemplate(sayAnswerTemplate, nil); err != nil {
				return err
			}
			dialog.AddButtons(buttons.AnswerButtons...)
			ctx.RoundState.WantsKnowAnswer = true
			return nil
		case patterns.UserDoesntKnowIntent:
			fallthrough
		case patterns.NextHintIntent:
			nextHintTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.OneMoreHintReply)
			if ctx.RoundState.CountOfUsedHints < CountOfHints {
				if err = dialog.SayTemplate(nextHintTemplate, nil); err != nil {
					return err
				}
			}
			return game.SayHint(log, ctx, request, meta, dialog)
		}
	}
	ctx.RoundState.WantsKnowAnswer = false
	wrongAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.WrongAnswerReply)
	if err = dialog.SayTemplate(wrongAnswerTemplate, nil); err != nil {
		return err
	}
	return game.SayHint(log, ctx, request, meta, dialog)
}

func (game *Game) StateEndRound(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	log.Debugf("End round state")
	if ctx.State.CountOfUsedActors == len(game.Actors) {
		endTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.NoMoreActorsReply)
		if err := dialog.SayTemplate(endTemplate, nil); err != nil {
			return err
		}
		ctx.GameState = replies.EndGameState
		return nil
	}
	nextRoundQuestion := game.repliesManager.ChooseCueTemplate(replies.EndRoundState, replies.NextRoundReply)
	if err := dialog.SayTemplate(nextRoundQuestion, nil); err != nil {
		return err
	}
	ctx.GameState = replies.StartGameState
	ctx.RoundState.LastActorIndex = -1
	if ctx.IsPromo && dialoglib.IsDeviceAllowedForPromo(meta) {
		promoReply := game.repliesManager.ChooseCueTemplate(replies.EndRoundState, replies.PromoReply)
		if err := dialog.SayTemplate(promoReply, nil); err != nil {
			return err
		}
		ctx.IsPromo = false
		dialog.AddButtons(buttons.PromoButtons...)
	} else {
		dialog.AddButtons(buttons.ContinueButtons...)
	}
	return nil
}

func (game *Game) StateExitGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	log.Debugf("Exit game state")
	exitGameTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.ExitGameReply)
	dialog.EndSession()
	return dialog.SayTemplate(exitGameTemplate, struct{ IsStation bool }{dialoglib.IsYandexStation(meta)})
}

func (game *Game) Fallback(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	return dialog.SayTemplate(fallbackTemplate, nil)
}

func (game *Game) chooseRandomActor(ctx *Context) bool {
	roundState := &ctx.RoundState
	if ctx.CountOfUsedActors == 0 {
		ctx.UsedActorsIndex = make([]int, len(game.Actors))
		for i := range ctx.UsedActorsIndex {
			ctx.UsedActorsIndex[i] = i
		}
	}
	lastNotUsed := len(game.Actors) - ctx.CountOfUsedActors - 1
	if lastNotUsed == -1 {
		return false
	}
	index := game.random.Intn(lastNotUsed + 1)
	roundState.LastActorIndex = ctx.UsedActorsIndex[index]
	roundState.CountOfUsedHints = 0
	ctx.UsedActorsIndex[index], ctx.UsedActorsIndex[lastNotUsed] =
		ctx.UsedActorsIndex[lastNotUsed], ctx.UsedActorsIndex[index]
	roundState.Hints = game.Actors[roundState.LastActorIndex].Hints
	ctx.CountOfUsedActors++
	return true
}

func (game *Game) sayRandomHint(ctx *Context, dialog *dialoglib.Dialog) (bool, error) {
	roundState := &ctx.RoundState
	lastNotUsed := len(roundState.Hints) - roundState.CountOfUsedHints - 1
	if lastNotUsed == -1 {
		return false, nil
	}
	index := game.random.Intn(lastNotUsed + 1)
	roundState.CurrentHint = roundState.Hints[index]
	roundState.Hints[index], roundState.Hints[lastNotUsed] = roundState.Hints[lastNotUsed], roundState.Hints[index]
	roundState.CountOfUsedHints++
	if err := game.sayCurrentHint(ctx, dialog); err != nil {
		return false, err
	}
	ctx.GameState = replies.GuessState
	return true, nil
}

func (game *Game) SayAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	var answerTemplate *dialoglib.CueTemplate
	if ctx.RoundState.WantsKnowAnswer {
		ctx.RoundState.WantsKnowAnswer = false
		answerTemplate = game.repliesManager.ChooseCueTemplate(replies.EndRoundState, replies.SayAnswerReply)
	} else {
		answerTemplate = game.repliesManager.ChooseCueTemplate(replies.EndRoundState, replies.LoseReply)
	}
	if err := dialog.SayTemplate(answerTemplate, &game.Actors[ctx.RoundState.LastActorIndex]); err != nil {
		return err
	}

	ctx.GameState = replies.EndRoundState
	return game.StateEndRound(log, ctx, request, meta, dialog)
}

func (game *Game) SayHint(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if ctx.RoundState.LastActorIndex == -1 {
		if !game.chooseRandomActor(ctx) {
			ctx.GameState = replies.EndRoundState
			return game.StateEndRound(log, ctx, request, meta, dialog)
		}
	}
	ok, err := game.sayRandomHint(ctx, dialog)
	if err != nil {
		return err
	}
	if ok {
		return nil
	}
	ctx.GameState = replies.EndRoundState
	return game.SayAnswer(log, ctx, request, meta, dialog)
}

func (game *Game) sayCurrentHint(ctx *Context, dialog *dialoglib.Dialog) error {
	hintTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.HintReply)
	if err := dialog.SayTemplate(hintTemplate, &ctx.RoundState); err != nil {
		return err
	}
	dialog.AddButtons(buttons.HintButtons...)
	return nil
}

func (game *Game) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, game.entityExtractor)
	if err != nil {
		return false, err
	}
	log.Debugf("len(matches): %d", len(matches))
	if len(matches) > 0 {
		log.Debugf("matches[0].Name: %s", matches[0].Name)
		log.Debugf("Game state: %s", ctx.GameState)

		switch matches[0].Name {
		case patterns.StartGameIntent:
			ctx.RoundState.LastActorIndex = -1
			return true, game.StateGeneral(log, ctx, request, meta, dialog)
		case patterns.UserMakesGuessIntent:
			ctx.RoundState.LastActorIndex = -1
			makesGuessTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.UserMakesGameReply)
			if err = dialog.SayTemplate(makesGuessTemplate, nil); err != nil {
				return true, err
			}
			ctx.GameState = replies.StartGameState
			dialog.AddButtons(buttons.StartButtons...)
			return true, nil
		case patterns.ExitIntent:
			ctx.RoundState.LastActorIndex = -1
			ctx.GameState = replies.EndGameState
			return true, game.StateExitGame(log, ctx, request, meta, dialog)
		case patterns.NotReadyIntent:
			if ctx.GameState == replies.StartGameState {
				return true, nil
			}
			notReadyTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.NotReadyReply)
			if err = dialog.SayTemplate(notReadyTemplate, nil); err != nil {
				return true, nil
			}
			return true, nil
		case patterns.HowToPlayIntent:
			if ctx.GameState == replies.StartGameState {
				howToPlayTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.HowToPlayReply)
				if err = dialog.SayTemplate(howToPlayTemplate, nil); err != nil {
					return true, err
				}
				dialog.AddButtons(buttons.YesButton)
				return true, nil
			} else if ctx.GameState == replies.GuessState {
				howToPlayTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.HowToPlayAtTheGameReply)
				if err = dialog.SayTemplate(howToPlayTemplate, nil); err != nil {
					return true, err
				}
				return true, game.sayCurrentHint(ctx, dialog)
			}
		}
	}
	return false, nil
}

func (game *Game) MisUnderstand(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	misunderstandingTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.MisUnderstandingReply)
	if err := dialog.SayTemplate(misunderstandingTemplate, nil); err != nil {
		return err
	}
	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.DontUnderstandReply)
	if err := dialog.SayTemplate(fallbackTemplate, struct {
		IsStation bool
	}{dialoglib.IsYandexStation(meta)}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.YesButton)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *Game) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	if ctx.IsNewSession() {
		log.Debugf("Created new session")
		ctx.GameState = replies.GeneralState
		ctx.RoundState = RoundState{
			LastActorIndex:   -1,
			CountOfUsedHints: 0,
			WantsKnowAnswer:  false,
		}
		ctx.IsPromo = true
		ctx.CountOfUsedActors = 0
	}
	dialog := &dialoglib.Dialog{}

	if ctx.GameState != replies.EndGameState {
		isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog)
		if err != nil {
			return nil, err
		} else if isGlobalCommand {
			return dialog.BuildResponse()
		}
	}

	var err error
	switch ctx.GameState {
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.StartGameState:
		err = game.StateStartGame(log, ctx, request, meta, dialog)
	case replies.GuessState:
		err = game.StateGuess(log, ctx, request, meta, dialog)
	default:
		err = game.Fallback(log, ctx, request, meta, dialog)
	}

	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}
