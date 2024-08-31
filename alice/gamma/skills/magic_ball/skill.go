package main

import (
	"math/rand"

	"golang.org/x/xerrors"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/game"
)

type MagicBallSkill struct {
	sdk.BaseSkill
	game *game.Game
}

func (skill *MagicBallSkill) CreateContext(context sdk.Context) sdk.Context {
	return game.CreateContext(context)
}

func NewMagicBallSkill(random rand.Rand) *MagicBallSkill {
	return &MagicBallSkill{game: game.NewMagicBallGame(random)}
}

func (skill *MagicBallSkill) Handle(log sdk.Logger, context sdk.Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
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
