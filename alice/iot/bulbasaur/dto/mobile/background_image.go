package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type BackgroundImageView struct {
	ID model.BackgroundImageID `json:"id"`
}

func NewBackgroundImageView(backgroundImageID model.BackgroundImageID) BackgroundImageView {
	backgroundImage := model.KnownBackgroundImages[backgroundImageID]
	return BackgroundImageView{
		ID: backgroundImage.ID,
	}
}
