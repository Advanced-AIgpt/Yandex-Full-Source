package game

import (
	"image"
	"image/draw"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/ships"
)

func getCoordinates(cell ships.Cell) image.Rectangle {
	const (
		fieldSize  = 500
		leftShift  = 0
		rightShift = 0
		cellSize   = fieldSize / 10
	)
	return image.Rectangle{
		Min: image.Point{X: cell.X*cellSize + leftShift, Y: cell.Y*cellSize + rightShift},
		Max: image.Point{X: (cell.X+1)*cellSize + leftShift, Y: (cell.Y+1)*cellSize + rightShift},
	}
}

func (game *Game) ShowImage(log sdk.Logger, ctx *Context, dialog *dialoglib.Dialog, field image.Image) error {
	currentImage := image.NewRGBA(field.Bounds())
	draw.Draw(currentImage, currentImage.Bounds(), field, image.Point{}, draw.Src)
	for x := 0; x < 10; x++ {
		for y := 0; y < 10; y++ {
			var updatedCell image.Image = nil
			if ctx.MyField[x][y].Status == ships.InjuredCell {
				updatedCell = game.injuredCell
			} else if ctx.MyField[x][y].Status == ships.KilledCell {
				updatedCell = game.killedCell
			} else if ctx.MyField[x][y].Status == ships.AwayShipCell {
				updatedCell = game.awayCell
			}
			if updatedCell != nil {
				draw.Draw(currentImage, getCoordinates(ships.Cell{X: x, Y: y}), updatedCell, image.Point{}, draw.Src)
			}
		}
	}

	url, err := sdk.PostImage(currentImage, "http://yandex.ru/images-apphost/image-download")
	if err != nil {
		return err
	}
	dialog.SetCard(&sdk.Card{
		ImageID: url,
	})
	return nil
}
