package xtestdata

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func SearchAppOrigin(testUser *model.TestUser) model.Origin {
	return model.Origin{
		SurfaceParameters: model.SearchAppSurfaceParameters{},
		User:              testUser.User,
	}
}

func SpeakerOrigin(testUser *model.TestUser, deviceID string) model.Origin {
	return model.Origin{
		SurfaceParameters: model.SpeakerSurfaceParameters{ID: deviceID},
		User:              testUser.User,
	}
}

func CallbackOrigin(testUser *model.TestUser) model.Origin {
	return model.Origin{
		SurfaceParameters: model.CallbackSurfaceParameters{},
		User:              testUser.User,
	}
}
