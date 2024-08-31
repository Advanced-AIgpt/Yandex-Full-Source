package game

import (
	"math/rand"
	"strings"
	"time"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/song_search/api"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/replies"
)

type Game struct {
	random         rand.Rand
	repliesManager replies.Manager
}

func NewSongSearchGame(random rand.Rand) *Game {
	game := &Game{
		random:         random,
		repliesManager: replies.Manager{RepliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates)},
	}
	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	startTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
	if err := dialog.SayTemplate(startTemplate, struct{ IsStation bool }{dialoglib.IsYandexStation(meta)}); err != nil {
		return err
	}

	ctx.GameState = replies.SearchState

	engageTemplate := game.repliesManager.ChooseCueTemplate(replies.SearchState, replies.EngageReply)
	if err := dialog.SayTemplate(engageTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.GeneralButtons...)
	return nil
}

func (game *Game) StateSearch(api api.SongSearch, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if len(strings.Split(request.Command, " ")) < 3 {
		shortSearchTemplate := game.repliesManager.ChooseCueTemplate(replies.SearchState, replies.ShortSearchReply)
		if err := dialog.SayTemplate(shortSearchTemplate, nil); err != nil {
			return err
		}
		moreWordsTemplate := game.repliesManager.ChooseCueTemplate(replies.SearchState, replies.MoreWordsReply)
		return dialog.SayTemplate(moreWordsTemplate, nil)
	}
	searchStart := time.Now()
	var err error
	if ctx.Results, err = api.Search(request.Command); err != nil {
		return err
	}
	log.Debugf("Music api search took %s", time.Since(searchStart))
	ctx.LastIndex = -1
	if err := game.GuessSong(ctx, dialog); err != nil {
		return err
	}
	return nil
}

func (game *Game) StateGuess(api api.SongSearch, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.GuessSong, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.CorrectGuessIntent:
			thankfulTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.ThankfulReply)
			if err := dialog.SayTemplate(thankfulTemplate, nil); err != nil {
				return err
			}

			anotherRoundTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.AnotherRoundReply)
			if err := dialog.SayTemplate(anotherRoundTemplate, nil); err != nil {
				return err
			}
			dialog.AddButtons(buttons.GeneralButtons...)
			ctx.GameState = replies.SearchState
			ctx.ResetSearchResults()
			return nil
		case patterns.IncorrectGuessIntent:
			return game.GuessSong(ctx, dialog)
		case patterns.AnotherSearchIntent:
			ctx.GameState = replies.SearchState
			engageTemplate := game.repliesManager.ChooseCueTemplate(replies.SearchState, replies.EngageReply)
			if err := dialog.SayTemplate(engageTemplate, nil); err != nil {
				return err
			}
			ctx.ResetSearchResults()
			return nil
		}
	}
	ctx.GameState = replies.SearchState
	return game.StateSearch(api, log, ctx, request, meta, dialog)
}

func (game *Game) StatePause(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.ContinueGame, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesContinueIntent:
			yesContinueTemplate := game.repliesManager.ChooseCueTemplate(replies.PauseState, replies.YesContinueReply)
			if err := dialog.SayTemplate(yesContinueTemplate, nil); err != nil {
				return err
			}
			ctx.GameState = replies.SearchState
			engageTemplate := game.repliesManager.ChooseCueTemplate(replies.SearchState, replies.EngageReply)
			if err := dialog.SayTemplate(engageTemplate, nil); err != nil {
				return err
			}
			return nil
		case patterns.NoContinueIntent:
			noContinueTemplate := game.repliesManager.ChooseCueTemplate(replies.PauseState, replies.NoContinueReply)
			if err := dialog.SayTemplate(noContinueTemplate, nil); err != nil {
				return err
			}
			return game.EndGame(ctx, dialog)
		}
	}
	return game.PauseFallback(ctx, dialog)
}

func (game *Game) StateLose(api api.SongSearch, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.ContinueGame, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesContinueIntent:
			yesContinueTemplate := game.repliesManager.ChooseCueTemplate(replies.PauseState, replies.YesContinueReply)
			if err := dialog.SayTemplate(yesContinueTemplate, nil); err != nil {
				return err
			}
			ctx.GameState = replies.SearchState
			engageTemplate := game.repliesManager.ChooseCueTemplate(replies.SearchState, replies.EngageReply)
			if err := dialog.SayTemplate(engageTemplate, nil); err != nil {
				return err
			}
			return nil
		case patterns.NoContinueIntent:
			noContinueTemplate := game.repliesManager.ChooseCueTemplate(replies.PauseState, replies.NoContinueReply)
			if err := dialog.SayTemplate(noContinueTemplate, nil); err != nil {
				return err
			}
			return game.EndGame(ctx, dialog)
		}
	}
	return game.StateSearch(api, log, ctx, request, meta, dialog)
}

func (game *Game) LoseGame(ctx *Context, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.LoseState
	anotherGameTemplate := game.repliesManager.ChooseCueTemplate(replies.LoseState, replies.AnotherGameReply)
	if err := dialog.SayTemplate(anotherGameTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.NewGameButtons...)
	ctx.ResetSearchResults()
	return nil
}

func (game *Game) isExplicit(ctx *Context, lines ...string) (bool, error) {
	request := &sdk.Request{
		Command: strings.Join(lines, " | "),
		Type:    "SimpleUtterance",
	}

	matches, err := ctx.Match(request, patterns.Explicit, &sdk.EmptyEntityExtractor)
	if err != nil {
		return false, err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.ExplicitIntent:
			return true, nil
		}
	}
	return false, nil
}

func (game *Game) GuessSong(ctx *Context, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.GuessState
	if len(ctx.Results) > 0 {
		ctx.LastIndex++
		if ctx.LastIndex < len(ctx.Results) {
			guess := ctx.Results[ctx.LastIndex]

			explicitPreview, err := game.isExplicit(ctx, guess.Artist, guess.Title)
			if err != nil {
				return err
			}

			if ctx.LastIndex > 0 && !(guess.Explicit || explicitPreview) {
				anotherGuessTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.AnotherGuessReply)
				if err := dialog.SayTemplate(anotherGuessTemplate, nil); err != nil {
					return err
				}
			}

			guessTemplate := game.repliesManager.ChooseGuessCueTemplate(guess, explicitPreview)
			if err := dialog.SayTemplate(guessTemplate, guess); err != nil {
				return err
			}
			dialog.AddButtons(buttons.FormGuessButtons(guess.URL)...)

			if ctx.IsFirstSearch {
				firstSearchTemplate := game.repliesManager.ChooseCueTemplate(replies.GuessState, replies.FirstSearchReply)
				if err := dialog.SayTemplate(firstSearchTemplate, nil); err != nil {
					return err
				}
				ctx.IsFirstSearch = false
			}
			return nil
		} else {
			searchEndTemplate := game.repliesManager.ChooseCueTemplate(replies.LoseState, replies.SearchEndReply)
			if err := dialog.SayTemplate(searchEndTemplate, nil); err != nil {
				return err
			}
		}
	} else {
		noResultsTemplate := game.repliesManager.ChooseCueTemplate(replies.LoseState, replies.NoResultsReply)
		if err := dialog.SayTemplate(noResultsTemplate, nil); err != nil {
			return err
		}
	}
	return game.LoseGame(ctx, dialog)
}

func (game *Game) ShowRules(ctx *Context, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	rulesTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.RulesReply)
	if err := dialog.SayTemplate(rulesTemplate, struct{ IsStation bool }{dialoglib.IsYandexStation(meta)}); err != nil {
		return err
	}
	ctx.GameState = replies.PauseState
	dialog.AddButtons(buttons.PauseButtons...)
	ctx.ResetSearchResults()
	return nil
}

func (game *Game) PauseFallback(ctx *Context, dialog *dialoglib.Dialog) error {
	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.PauseState, replies.PauseFallbackReply)
	if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.PauseButtons...)
	return nil
}

func (game *Game) Fallback(ctx *Context, dialog *dialoglib.Dialog) error {
	pauseFallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	if err := dialog.SayTemplate(pauseFallbackTemplate, nil); err != nil {
		return err
	}
	return nil
}

func (game *Game) EndGame(ctx *Context, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.EndGameState
	endGameTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndGameReply)
	if err := dialog.SayTemplate(endGameTemplate, nil); err != nil {
		return err
	}
	dialog.EndSession()
	ctx.ResetSearchResults()
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
		case patterns.ShowRulesIntent:
			return true, game.ShowRules(ctx, meta, dialog)
		}
	}
	return false, nil
}

func (game *Game) Handle(api api.SongSearch, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	dialog := &dialoglib.Dialog{}

	if ctx.IsNewSession() {
		ctx.GameState = replies.GeneralState
		ctx.IsFirstSearch = true
		ctx.ResetSearchResults()
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
	case replies.SearchState:
		err = game.StateSearch(api, log, ctx, request, meta, dialog)
	case replies.GuessState:
		err = game.StateGuess(api, log, ctx, request, meta, dialog)
	case replies.PauseState:
		err = game.StatePause(log, ctx, request, meta, dialog)
	case replies.LoseState:
		err = game.StateLose(api, log, ctx, request, meta, dialog)
	default:
		err = game.Fallback(ctx, dialog)
	}

	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}
