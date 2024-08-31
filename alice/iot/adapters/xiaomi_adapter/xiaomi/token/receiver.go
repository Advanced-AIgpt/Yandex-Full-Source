package token

import (
	"context"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Receiver struct {
	skillID      string
	appName      string
	trusted      bool
	socialClient socialism.IClient
}

func NewReceiver(skillID, appName string, trusted bool, socialClient socialism.IClient) Receiver {
	return Receiver{
		skillID:      skillID,
		appName:      appName,
		trusted:      trusted,
		socialClient: socialClient,
	}
}

func (tr Receiver) GetToken(context context.Context, userIDs []uint64) (string, error) {
	skillInfo := socialism.NewSkillInfo(tr.skillID, tr.appName, tr.trusted)

	var errors bulbasaur.Errors
	for _, userID := range userIDs {
		token, err := tr.socialClient.GetUserApplicationToken(context, userID, skillInfo)
		if err != nil {
			errors = append(errors, err)
			continue
		}
		return token, nil
	}
	if len(errors) > 0 {
		return "", errors
	}
	return "", xerrors.New("userIDs slice is empty")
}
