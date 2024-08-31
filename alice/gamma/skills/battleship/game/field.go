package game

import (
	"math/rand"

	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/ships"
)

type Direction struct {
	dx int
	dy int
}

func isInsideField(row, column int) bool {
	return 0 <= row && row < fieldSize && 0 <= column && column < fieldSize
}

func RandomizeCell(random rand.Rand, ctx *Context, emptyCells int) (int, int) {
	r := random.Intn(emptyCells)
	x := 0
	y := 0
	for r > 0 {
		if ctx.MyField[x][y].ShipIndex == ships.DefaultCell {
			r--
		}
		y++
		if y == fieldSize {
			x++
			y = 0
		}
		if x == fieldSize {
			x = 0
		}
	}
	for x < fieldSize && y < fieldSize {
		if ctx.MyField[x][y].ShipIndex == ships.DefaultCell {
			return x, y
		}
		y++
		if y == fieldSize {
			x++
			y = 0
		}
	}
	return -1, -1
}

func RandomizeUserCell(random rand.Rand, ctx *Context, emptyCells int) (int, int) {
	r := random.Intn(emptyCells)
	x := 0
	y := 0
	for r > 0 {
		if ctx.UserField[x][y] == ships.DefaultCell {
			r--
		}
		y++
		if y == fieldSize {
			x++
			y = 0
		}
		if x == fieldSize {
			x = 0
		}
	}
	for x < fieldSize && y < fieldSize {
		if ctx.UserField[x][y] == ships.DefaultCell {
			return x, y
		}
		y++
		if y == fieldSize {
			x++
			y = 0
		}
		if x == fieldSize {
			x = 0
		}
	}
	return -1, -1
}

func RandomizeShip(random rand.Rand, ctx *Context, shipSize int, emptyCells *int, shipIndex int) {
	directions := make([]Direction, 0)
	var x, y int
	for len(directions) == 0 {
		x, y = RandomizeCell(random, ctx, *emptyCells)
		if ctx.IsEnoughPlaceRight(x, y, shipSize) {
			directions = append(directions, Direction{
				dx: 1,
				dy: 0,
			})
		}
		if ctx.IsEnoughPlaceDown(x, y, shipSize) {
			directions = append(directions, Direction{
				dx: 0,
				dy: 1,
			})
		}
		if ctx.IsEnoughPlaceLeft(x, y, shipSize) {
			directions = append(directions, Direction{
				dx: -1,
				dy: 0,
			})
		}
		if ctx.IsEnoughPlaceUp(x, y, shipSize) {
			directions = append(directions, Direction{
				dx: 0,
				dy: -1,
			})
		}
	}
	index := random.Intn(len(directions))
	dir := directions[index]
	var cells []ships.Cell
	for i := 0; i < shipSize; i++ {
		cell := ships.Cell{X: x + (i * dir.dx), Y: y + (i * dir.dy)}
		ctx.MyField[cell.X][cell.Y] = ships.MyCell{ShipIndex: shipIndex}
		*emptyCells--
		cells = append(cells, cell)
	}
	curShip := ships.Ship{Status: 0, Cells: cells, LifeCells: shipSize}
	ctx.Ships = append(ctx.Ships, curShip)
	for i := 0; i < shipSize; i++ {
		curX, curY := x+(i*dir.dx), y+(i*dir.dy)
		for dx := -1; dx <= 1; dx++ {
			for dy := -1; dy <= 1; dy++ {
				row := curX + dx
				column := curY + dy
				if isInsideField(row, column) {
					if ctx.MyField[row][column].ShipIndex == ships.DefaultCell {
						*emptyCells--
						ctx.MyField[row][column].ShipIndex = -1
					}
				}
			}
		}
	}
}

func (ctx *Context) IsEnoughPlaceLeft(x, y, shipSize int) bool {
	if !(x >= shipSize-1 && isInsideField(x, y)) {
		return false
	}
	for i := 0; i < shipSize; i++ {
		if ctx.MyField[x-i][y].ShipIndex != ships.DefaultCell {
			return false
		}
	}
	return true
}

func (ctx *Context) IsEnoughPlaceRight(x, y, shipSize int) bool {
	if !(x < fieldSize-shipSize+1 && isInsideField(x, y)) {
		return false
	}
	for i := 0; i < shipSize; i++ {
		if ctx.MyField[x+i][y].ShipIndex != ships.DefaultCell {
			return false
		}
	}
	return true
}

func (ctx *Context) IsEnoughPlaceUp(x, y, shipSize int) bool {
	if !(y >= shipSize-1 && isInsideField(x, y)) {
		return false
	}
	for i := 0; i < shipSize; i++ {
		if ctx.MyField[x][y-i].ShipIndex != ships.DefaultCell {
			return false
		}
	}
	return true
}

func (ctx *Context) IsEnoughPlaceDown(x, y, shipSize int) bool {
	if !(y < fieldSize-shipSize+1 && isInsideField(x, y)) {
		return false
	}
	for i := 0; i < shipSize; i++ {
		if ctx.MyField[x][y+i].ShipIndex != ships.DefaultCell {
			return false
		}
	}
	return true
}

func TryGenerateField(random rand.Rand, ctx *Context) bool {
	emptyCells := fieldSize * fieldSize
	RandomizeShip(random, ctx, 4, &emptyCells, 1)
	for i := 0; i < 2; i++ {
		RandomizeShip(random, ctx, 3, &emptyCells, 2+i)
	}
	for i := 0; i < 3; i++ {
		RandomizeShip(random, ctx, 2, &emptyCells, 4+i)
	}
	for i := 0; i < 4; i++ {
		if emptyCells == 0 {
			return false
		}
		RandomizeShip(random, ctx, 1, &emptyCells, 7+i)
	}
	return true
}

func GenerateField(random rand.Rand, ctx *Context) {
	for {
		ctx.MyField = make([][]ships.MyCell, fieldSize)
		for i := 0; i < fieldSize; i++ {
			ctx.MyField[i] = make([]ships.MyCell, fieldSize)
		}
		ctx.Ships = make([]ships.Ship, 0, 10)
		if TryGenerateField(random, ctx) {
			normalizeShipsIndexes(ctx)
			ctx.LifeShips = len(ctx.Ships)
			return
		}
	}
}

func DefaultUserField() [][]ships.UserCell {
	userField := make([][]ships.UserCell, fieldSize)
	for i := range userField {
		userField[i] = make([]ships.UserCell, fieldSize)
	}
	return userField
}

func normalizeShipsIndexes(ctx *Context) {
	for i := range ctx.MyField {
		for j := range ctx.MyField[i] {
			if ctx.MyField[i][j].ShipIndex != -1 {
				ctx.MyField[i][j].ShipIndex--
			}
		}
	}
}
