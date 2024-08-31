package images

import (
	"bytes"
	"image"
	"image/png"

	"a.yandex-team.ru/library/go/core/resource"
)

func GetDefaultImages() (image.Image, image.Image, image.Image, image.Image, error) {
	var field, awayCell, injuredCell, killedCell image.Image
	var err error

	if field, err = png.Decode(bytes.NewReader(resource.Get("field.png"))); err != nil {
		return nil, nil, nil, nil, err
	}

	if awayCell, err = png.Decode(bytes.NewReader(resource.Get("away.png"))); err != nil {
		return nil, nil, nil, nil, err
	}

	if injuredCell, err = png.Decode(bytes.NewReader(resource.Get("injured.png"))); err != nil {
		return nil, nil, nil, nil, err
	}

	if killedCell, err = png.Decode(bytes.NewReader(resource.Get("killed.png"))); err != nil {
		return nil, nil, nil, nil, err
	}

	return field, awayCell, injuredCell, killedCell, nil
}
