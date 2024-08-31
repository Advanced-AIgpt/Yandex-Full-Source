package main

import (
	"math/rand"

	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/battleship/game"
)

type BattleshipSkill struct {
	sdk.BaseSkill
	game *game.Game
}

func (skill *BattleshipSkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context)
}

func NewBattleshipSkill(random rand.Rand) *BattleshipSkill {
	return &BattleshipSkill{game: game.NewBattleshipGame(random, true)}
}

func (skill *BattleshipSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
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
