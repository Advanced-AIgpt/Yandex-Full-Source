package main

import (
	"math/rand"

	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/zoology/game"
)

type ZoologySkill struct {
	sdk.BaseSkill
	game *game.Game
}

func (skill *ZoologySkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context)
}

func NewZoologySkill(random rand.Rand) *ZoologySkill {
	return &ZoologySkill{game: game.NewZoologyGame(random)}
}

func (skill *ZoologySkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
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
