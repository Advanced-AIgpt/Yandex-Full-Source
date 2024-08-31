package main

import (
	"math/rand"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/word_game/game"

	"golang.org/x/xerrors"
)

type WordSkill struct {
	sdk.BaseSkill
	game *game.WordGame
}

func (skill *WordSkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context, len(skill.game.RoundContent))
}

func NewWordSkill(random rand.Rand) *WordSkill {
	return &WordSkill{game: game.NewWordGame(random)}
}

func (skill *WordSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	ctx, ok := context.(*game.Context)
	if !ok {
		return nil, xerrors.New("invalid skill context")
	}

	log.Debugf("%+v", request)
	response, err := skill.game.Handle(log, ctx, request, meta)
	if err != nil {
		log.Errorf("Error handling request: %+v", request)
		return nil, err
	}

	return response, nil
}
