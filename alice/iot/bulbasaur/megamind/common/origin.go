package common

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

func NewOrigin(ctx context.Context, clientInfo libmegamind.ClientInfo, user model.User) model.Origin {
	if clientInfo.IsSmartSpeaker() {
		return model.NewOrigin(ctx, model.SpeakerSurfaceParameters{ID: clientInfo.DeviceID, Platform: clientInfo.Platform}, user)
	}
	return model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
}
