package main

import (
	"math/rand"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/game"

	"golang.org/x/xerrors"
)

type CheatSkill struct {
	sdk.BaseSkill
	game *game.Game
}

func (skill *CheatSkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context)
}

func NewCheatSkill(random rand.Rand) *CheatSkill {
	return &CheatSkill{game: game.NewCheatGame(random)}
}

func (skill *CheatSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
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
