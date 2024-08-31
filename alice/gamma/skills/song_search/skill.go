package main

import (
	"math/rand"

	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/song_search/api"
	"a.yandex-team.ru/alice/gamma/skills/song_search/game"
)

const (
	searchLimit   = 7
	snippetLength = 4
)

var MusicOptionNotSetError = xerrors.New("music api url not set")

type SongSearchSkill struct {
	game   *game.Game
	apiURL string
	sdk.BaseSkill
}

func (skill *SongSearchSkill) Setup(log sdk.Logger, options sdk.Options) (err error) {
	skillOptions, ok := options.(*MusicSkillOptions)
	if !ok {
		return xerrors.New("invalid options in setup")
	}
	if skillOptions.MusicAPIURL == "" {
		return MusicOptionNotSetError
	}

	skill.apiURL = skillOptions.MusicAPIURL
	log.Infof("Music api url: %s", skill.apiURL)
	return nil
}

func (skill *SongSearchSkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context)
}

func NewSongSearchSkill(random rand.Rand) *SongSearchSkill {
	return &SongSearchSkill{game: game.NewSongSearchGame(random)}
}

func (skill *SongSearchSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	ctx, ok := context.(*game.Context)
	if !ok {
		return nil, xerrors.New("invalid skill context")
	}

	log.Infof("Got request: %+v", request)
	searchAPI := &api.MusicSearchAPI{Logger: log, URL: skill.apiURL, Limit: searchLimit, SnippetLength: snippetLength}
	response, err := skill.game.Handle(searchAPI, log, ctx, request, meta)
	if err != nil {
		log.Errorf("Error handling request: %+v", request)
		return nil, err
	}

	return response, nil
}
