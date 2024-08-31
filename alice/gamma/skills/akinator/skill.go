package main

import (
	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/akinator/game"
	"golang.org/x/xerrors"
	"math/rand"
)

type AkinatorSkill struct {
	sdk.BaseSkill
	game *game.Game
}

func (skill *AkinatorSkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context)
}

func NewAkinatorSkill(random rand.Rand) *AkinatorSkill {
	return &AkinatorSkill{game: game.NewAkinatorGame(random)}
}

func (skill *AkinatorSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	ctx, ok := context.(*game.Context)
	if !ok {
		return nil, xerrors.New("invalid skill context")
	}
	log.Debugf("%+v", request)
	log.Debugf("state: %s", context.GetState())
	response, err := skill.game.Handle(log, ctx, request, meta)
	if err != nil {
		log.Errorf("Error handling request: %+v", request)
		return nil, err
	}
	log.Debugf("new state: %s", context.GetState())
	return response, err
}
