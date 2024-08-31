package game

import (
	"image"
	"log"
	"math/rand"
	"strings"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/images"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/replies"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/ships"
)

const (
	PlayStatus        = "play"
	WinStatus         = "win"
	LoseStatus        = "lose"
	CheatingStatus    = "cheating"
	SurrenderedStatus = "surrendered"
)

const (
	HardDifficulty   = "Hard"
	NormalDifficulty = "Normal"
	EasyDifficulty   = "Easy"
)

const (
	WrongShipSequenceIssue = "wrongShips"
	WrongInjuryIssue       = "wrongInjury"
	LargeShipIssue         = "LargeShipIssue"
)

type Game struct {
	repliesManager dialoglib.RepliesManager
	random         rand.Rand
	extractor      *sdk.EntityExtractor
	letters        []string
	field          image.Image
	injuredCell    image.Image
	killedCell     image.Image
	awayCell       image.Image
	isShowImage    bool
}

func NewBattleshipGame(random rand.Rand, showImage bool) *Game {
	game := &Game{
		random:         random,
		repliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates),
		letters:        ships.GetLetters(),
		isShowImage:    showImage,
	}

	if cellsPatterns, err := patterns.GetCells(); err != nil {
		log.Fatal(err)
	} else {
		game.extractor = sdk.NewEntityExtractor(cellsPatterns)
	}

	var err error
	if game.field, game.awayCell, game.injuredCell, game.killedCell, err = images.GetDefaultImages(); err != nil {
		log.Fatal(err)
	}

	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartReply)
	if err := dialog.SayTemplate(startGameTemplate, struct {
		IsStation bool
	}{
		IsStation: dialoglib.IsYandexStation(meta),
	}); err != nil {
		return err
	}
	ctx.GameState = replies.StartState
	dialog.AddButtons(buttons.DefaultContinueButtons...)
	return nil
}

func (game *Game) updateContext(ctx *Context) {
	ctx.GameState = replies.UserShootState
	ctx.Status = PlayStatus
	GenerateField(game.random, ctx)
	ctx.UserField = DefaultUserField()
	ctx.UserEmptyCells = fieldSize * fieldSize
	ctx.UserShipsCounter = []int{4, 3, 2, 1}
	ctx.IsLastShootInjury = false
	ctx.Difficulty = NormalDifficulty
}

func (game *Game) StateStart(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.SelectCommands, game.extractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesIntent:
			dialog.AddButtons(buttons.DefaultGameButtons...)
			startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.StartState, replies.PrepareFieldReply)
			if err := dialog.SayTemplate(startGameTemplate, struct {
				IsStation bool
			}{
				IsStation: dialoglib.IsYandexStation(meta),
			}); err != nil {
				return err
			}
			game.updateContext(ctx)
			return nil
		case patterns.NoIntent:
			ctx.GameState = replies.EndGameState
			return game.StateEnd(log, ctx, request, meta, dialog)
		}
	}

	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *Game) GetNumberValue(hypothesis []sdk.Hypothesis) interface{} {
	if len(hypothesis[0].Variables["NUMBER"]) > 0 {
		obj := hypothesis[0].Variables["NUMBER"][0].(map[string]interface{})
		return obj["Kind"].(map[string]interface{})["NumberValue"]
	}
	return nil
}

func (game *Game) GetFirstNameValue(hypothesis []sdk.Hypothesis) interface{} {
	if len(hypothesis[0].Variables["FIO"]) > 0 {
		obj := hypothesis[0].Variables["FIO"][0].(map[string]interface{})
		fields := obj["Kind"].(map[string]interface{})["StructValue"].(map[string]interface{})["fields"]
		if _, ok := fields.(map[string]interface{})["first_name"]; !ok {
			return nil
		}
		return fields.(map[string]interface{})["first_name"].(map[string]interface{})["Kind"].(map[string]interface{})["StringValue"]
	}
	return nil
}

func (game *Game) GetRawNumber(hypothesis []sdk.Hypothesis) int {
	firstName := game.GetFirstNameValue(hypothesis)
	if firstName == nil {
		return -1
	}
	name := []rune(firstName.(string))
	letter := strings.ToUpper(string(name[0]))
	for i := 0; i < len(game.letters); i++ {
		if letter == game.letters[i] {
			return i
		}
	}
	return -1
}

func (game *Game) StateUserShoot(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.UserShootCommands, game.extractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		shoot := ships.Cell{X: -1, Y: -1}
		switch matches[0].Name {
		case patterns.ShootIntent:
			shoot = convertToShoot(matches[0].Variables["Cell"][0].(string))
		case patterns.FioShootIntent:
			shoot = ships.Cell{X: int(game.GetNumberValue(matches).(float64) - 1), Y: game.GetRawNumber(matches)}
		}

		if !isInsideField(shoot.X, shoot.Y) {
			return game.Fallback(log, ctx, request, meta, dialog)
		}

		if cell := ctx.MyField[shoot.X][shoot.Y]; cell.Used {
			return game.ReactForReShoot(log, ctx, request, meta, dialog, cell)
		} else {
			ctx.MyField[shoot.X][shoot.Y].Used = true
			if cell.ShipIndex != -1 {
				dialog.AddButtons(buttons.DefaultGameButtons...)
				ctx.Ships[cell.ShipIndex].LifeCells--
				if ctx.Ships[cell.ShipIndex].LifeCells == 0 {
					ctx.MyField[shoot.X][shoot.Y].Status = ships.KilledCell
					killedTemplate := game.repliesManager.ChooseCueTemplate(replies.UserShootState, replies.KillReply)
					if err = dialog.SayTemplate(killedTemplate, nil); err != nil {
						return err
					}
					ctx.LifeShips--
					if ctx.LifeShips == 0 {
						ctx.Status = WinStatus
						return game.StateEndRound(log, ctx, request, meta, dialog)
					}
					if game.isShowImage {
						if err := game.ShowImage(log, ctx, dialog, game.field); err != nil {
							return err
						}
					}
					return nil
				}
				ctx.MyField[shoot.X][shoot.Y].Status = ships.InjuredCell
				injuredTemplate := game.repliesManager.ChooseCueTemplate(replies.UserShootState, replies.InjuredReply)
				if game.isShowImage {
					if err := game.ShowImage(log, ctx, dialog, game.field); err != nil {
						return err
					}
				}
				return dialog.SayTemplate(injuredTemplate, nil)
			}
			ctx.MyField[shoot.X][shoot.Y].Status = ships.AwayShipCell
			ctx.GameState = replies.MyShootState
			awayTemplate := game.repliesManager.ChooseCueTemplate(replies.UserShootState, replies.AwayReply)
			if err = dialog.SayTemplate(awayTemplate, nil); err != nil {
				return err
			}
			return game.MakeShoot(log, ctx, request, meta, dialog)
		}
	}

	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *Game) StateEndRound(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	endRoundTemplate := game.repliesManager.ChooseCueTemplate(replies.EndRoundState, replies.EndRoundReply)
	ctx.GameState = replies.StartState
	dialog.AddButtons(buttons.DefaultContinueButtons...)
	return dialog.SayTemplate(endRoundTemplate, struct {
		Status string
	}{
		Status: ctx.Status,
	})
}

func (game *Game) StateEnd(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	endTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndReply)
	dialog.EndSession()
	return dialog.SayTemplate(endTemplate, nil)
}

func (game *Game) Fallback(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	return dialog.SayTemplate(fallbackTemplate, nil)
}

func (game *Game) RestartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	dialog.AddButtons(buttons.DefaultContinueButtons...)
	restartTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.RestartReply)
	if err := dialog.SayTemplate(restartTemplate, nil); err != nil {
		return err
	}
	ctx.State = State{
		GameState: replies.StartState,
	}
	return nil
}

func (game *Game) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, game.extractor)
	if err != nil {
		return false, err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.StartIntent:
			ctx.GameState = replies.GeneralState
			return true, game.StateGeneral(log, ctx, request, meta, dialog)
		case patterns.RestartIntent:
			return true, game.RestartGame(log, ctx, request, meta, dialog)
		case patterns.RulesIntent:
			dialog.AddButtons(buttons.DefaultGameButtons...)
			rulesTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.RulesReply)
			if err = dialog.SayTemplate(rulesTemplate, nil); err != nil {
				return false, err
			}
			return true, nil
		case patterns.WaitIntent:
			dialog.AddButtons(buttons.DefaultGameButtons...)
			waitTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.WaitReply)
			if err = dialog.SayTemplate(waitTemplate, nil); err != nil {
				return false, err
			}
			return true, nil
		case patterns.SurrenderIntent:
			ctx.GameState = replies.EndRoundState
			ctx.Status = SurrenderedStatus
			return true, game.StateEndRound(log, ctx, request, meta, dialog)
		case patterns.EndIntent:
			ctx.GameState = replies.EndGameState
			return true, game.StateEnd(log, ctx, request, meta, dialog)
		}
	}

	return false, nil
}

func convertToShoot(s string) ships.Cell {
	return ships.Cell{X: int(s[0] - '0'), Y: int(s[1] - '0')}
}

func (game *Game) CheckForWin(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) bool {
	for _, counter := range ctx.UserShipsCounter {
		if counter > 0 {
			return false
		}
	}
	return true
}

func (game *Game) FinishShip(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (int, int, error) {
	curX, curY := ctx.InjuryLastShoot.X, ctx.InjuryLastShoot.Y
	var possibleCells []ships.Cell
	mainDirection := Direction{dx: -1, dy: -1}
	for dirX := -1; dirX <= 1; dirX++ {
		for dirY := -1; dirY <= 1; dirY++ {
			if !(dirX == 0 || dirY == 0) || (dirX == 0 && dirY == 0) {
				continue
			}
			dx, dy := dirX, dirY
			x, y := curX+dx, curY+dy
			if isInsideField(x, y) {
				if ctx.UserField[x][y] == ships.DefaultCell {
					if (mainDirection == Direction{-1, -1} || mainDirection.dx == dx || mainDirection.dy == dy) {
						possibleCells = append(possibleCells, ships.Cell{X: x, Y: y})
					}
				} else if ctx.UserField[x][y] == ships.InjuredCell {
					mainDirection = Direction{dx: dx, dy: dy}
					possibleCells = []ships.Cell{}
					for isInsideField(x+dx, y+dy) && ctx.UserField[x+dx][y+dy] == ships.InjuredCell {
						x, y = x+dx, y+dy
					}
					if isInsideField(x+dx, y+dy) && ctx.UserField[x+dx][y+dy] == ships.DefaultCell {
						possibleCells = append(possibleCells, ships.Cell{X: x + dx, Y: y + dy})
					}
					dx = -1 * dx
					dy = -1 * dy
					for isInsideField(x+dx, y+dy) && ctx.UserField[x+dx][y+dy] == ships.InjuredCell {
						x, y = x+dx, y+dy
					}
					if isInsideField(x+dx, y+dy) && ctx.UserField[x+dx][y+dy] == ships.DefaultCell {
						possibleCells = append(possibleCells, ships.Cell{X: x + dx, Y: y + dy})
					}
				}
			}
		}
	}
	if len(possibleCells) == 0 {
		return -1, -1, game.UserCheatingState(log, ctx, request, meta, dialog, WrongInjuryIssue)
	}
	r := game.random.Intn(len(possibleCells))
	return possibleCells[r].X, possibleCells[r].Y, nil
}

func (game *Game) MakeShoot(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	var x, y int
	var err error
	if ctx.Difficulty != EasyDifficulty && ctx.IsLastShootInjury {
		x, y, err = game.FinishShip(log, ctx, request, meta, dialog)
		if err != nil {
			return err
		}
		if x == -1 && y == -1 {
			return nil
		}
	} else {
		x, y = RandomizeUserCell(game.random, ctx, ctx.UserEmptyCells)
	}
	ctx.LastShoot = ships.Cell{X: x, Y: y}
	shootTemplate := game.repliesManager.ChooseCueTemplate(replies.MyShootState, replies.MyShootReply)
	if err := dialog.SayTemplate(shootTemplate, struct {
		Letter dialoglib.Cue
		Number int
	}{
		Letter: dialoglib.Cue{
			Text:  game.letters[y],
			Voice: replies.LettersTTS[strings.ToLower(game.letters[y])],
		},
		Number: x + 1,
	}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.AnswersOnShootingButtons...)
	return nil
}

func (game *Game) UserCheatingState(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog, issue string) error {
	var issueTemplate *dialoglib.CueTemplate
	switch issue {
	case WrongShipSequenceIssue:
		issueTemplate = game.repliesManager.ChooseCueTemplate(replies.UserCheatingState, replies.WrongShipSequenceReply)

	case LargeShipIssue:
		issueTemplate = game.repliesManager.ChooseCueTemplate(replies.UserCheatingState, replies.LargeShipReply)

	case WrongInjuryIssue:
		issueTemplate = game.repliesManager.ChooseCueTemplate(replies.UserCheatingState, replies.WrongInjuryReply)
		if err := dialog.SayTemplate(issueTemplate, nil); err != nil {
			return err
		}
		ctx.IsLastShootInjury = false
		ship := game.FindDeadShip(log, ctx, request, meta, dialog)
		if len(ship.Cells) > 4 {
			return game.UserCheatingState(log, ctx, request, meta, dialog, LargeShipIssue)
		}
		ctx.UserShipsCounter[len(ship.Cells)-1]--
		if ctx.UserShipsCounter[len(ship.Cells)-1] < 0 {
			return game.UserCheatingState(log, ctx, request, meta, dialog, WrongShipSequenceIssue)
		}
		game.MarkAfterKill(log, ctx, request, meta, dialog, ship)
		return game.MakeShoot(log, ctx, request, meta, dialog)
	}
	if err := dialog.SayTemplate(issueTemplate, nil); err != nil {
		return err
	}
	ctx.GameState = replies.EndRoundState
	ctx.Status = CheatingStatus
	return game.StateEndRound(log, ctx, request, meta, dialog)
}

func (game *Game) isInsideShip(ship ships.Ship, cell ships.Cell) bool {
	for _, shipCell := range ship.Cells {
		if shipCell == cell {
			return true
		}
	}
	return false
}

func (game *Game) MarkAfterKill(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog, ship ships.Ship) {
	for _, cell := range ship.Cells {
		curX, curY := cell.X, cell.Y
		for dx := -1; dx <= 1; dx++ {
			for dy := -1; dy <= 1; dy++ {
				x, y := curX+dx, curY+dy
				if 0 <= x && x < fieldSize && 0 <= y && y < fieldSize {
					if !game.isInsideShip(ship, ships.Cell{X: x, Y: y}) {
						if ctx.UserField[x][y] == ships.DefaultCell {
							ctx.UserEmptyCells--
						}
						ctx.UserField[x][y] = ships.NearShipCell
					}
				}
			}
		}
		if ctx.UserField[curX][curY] == ships.DefaultCell {
			ctx.UserEmptyCells--
		}
		ctx.UserField[curX][curY] = ships.KilledCell
	}
}

func (game *Game) FindDeadShip(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) ships.Ship {
	var directions []Direction
	x, y := ctx.LastShoot.X, ctx.LastShoot.Y
	for dx := -1; dx <= 1; dx++ {
		for dy := -1; dy <= 1; dy++ {
			if (dx == 0 || dy == 0) && dx != dy && isInsideField(x+dx, y+dy) {
				if ctx.UserField[x+dx][y+dy] == ships.InjuredCell {
					directions = append(directions, Direction{dx, dy})
				}
			}
		}
	}
	ship := ships.Ship{}
	ship.Cells = append(ship.Cells, ships.Cell{X: x, Y: y})
	for _, dir := range directions {
		curX, curY := x+dir.dx, y+dir.dy
		for 0 <= curX && curX < fieldSize && 0 <= curY && curY < fieldSize && ctx.UserField[curX][curY] == ships.InjuredCell {
			ship.Cells = append(ship.Cells, ships.Cell{X: curX, Y: curY})
			curX += dir.dx
			curY += dir.dy
		}
	}
	return ship
}

func (game *Game) UserShipKill(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.IsLastShootInjury = false
	ship := game.FindDeadShip(log, ctx, request, meta, dialog)
	if len(ship.Cells) > 4 {
		return game.UserCheatingState(log, ctx, request, meta, dialog, LargeShipIssue)
	}
	game.MarkAfterKill(log, ctx, request, meta, dialog, ship)
	ctx.UserShipsCounter[len(ship.Cells)-1]--
	if ctx.UserShipsCounter[len(ship.Cells)-1] < 0 {
		return game.UserCheatingState(log, ctx, request, meta, dialog, WrongShipSequenceIssue)
	} else if game.CheckForWin(log, ctx, request, meta, dialog) {
		ctx.Status = LoseStatus
		ctx.GameState = replies.EndRoundState
		return game.StateEndRound(log, ctx, request, meta, dialog)
	}
	killTemplate := game.repliesManager.ChooseCueTemplate(replies.MyShootState, replies.KillReply)
	if err := dialog.SayTemplate(killTemplate, nil); err != nil {
		return err
	}
	return game.MakeShoot(log, ctx, request, meta, dialog)
}

func (game *Game) UserShipInjured(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.IsLastShootInjury = true
	ctx.InjuryLastShoot = ctx.LastShoot
	if ctx.UserField[ctx.LastShoot.X][ctx.LastShoot.Y] == ships.DefaultCell {
		ctx.UserEmptyCells--
	}
	ctx.UserField[ctx.LastShoot.X][ctx.LastShoot.Y] = ships.InjuredCell
	injuredTemplate := game.repliesManager.ChooseCueTemplate(replies.MyShootState, replies.InjuredReply)
	if err := dialog.SayTemplate(injuredTemplate, nil); err != nil {
		return err
	}
	return game.MakeShoot(log, ctx, request, meta, dialog)
}

func (game *Game) ReactForWinIntent(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if game.CheckForWin(log, ctx, request, meta, dialog) {
		ctx.Status = LoseStatus
		ctx.GameState = replies.EndRoundState
		return game.StateEndRound(log, ctx, request, meta, dialog)
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	notWinTemplate := game.repliesManager.ChooseCueTemplate(replies.MyShootState, replies.NotWinReply)
	return dialog.SayTemplate(notWinTemplate, nil)
}

func (game *Game) MissShoot(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.UserEmptyCells--
	ctx.GameState = replies.UserShootState
	ctx.UserField[ctx.LastShoot.X][ctx.LastShoot.Y] = ships.AwayShipCell
	dialog.AddButtons(buttons.DefaultGameButtons...)
	if game.isShowImage {
		if err := game.ShowImage(log, ctx, dialog, game.field); err != nil {
			return err
		}
	}
	awayTemplate := game.repliesManager.ChooseCueTemplate(replies.MyShootState, replies.AwayReply)
	if err := dialog.SayTemplate(awayTemplate, nil); err != nil {
		return err
	}
	return nil
}

func (game *Game) StateMyShoot(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.MyShootCommands, game.extractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.KillIntent:
			return game.UserShipKill(log, ctx, request, meta, dialog)

		case patterns.InjuredIntent:
			return game.UserShipInjured(log, ctx, request, meta, dialog)

		case patterns.AwayIntent:
			return game.MissShoot(log, ctx, request, meta, dialog)

		case patterns.WinIntent:
			return game.ReactForWinIntent(log, ctx, request, meta, dialog)
		}
	}

	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *Game) ReactForReShoot(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog, cell ships.MyCell) error {
	dialog.AddButtons(buttons.DefaultGameButtons...)
	usedTemplate := game.repliesManager.ChooseCueTemplate(replies.UserShootState, replies.CellUsedReply)
	if err := dialog.SayTemplate(usedTemplate, nil); err != nil {
		return err
	}
	if cell.ShipIndex == -1 {
		awayTemplate := game.repliesManager.ChooseCueTemplate(replies.UserShootState, replies.AwayReShootReply)
		return dialog.SayTemplate(awayTemplate, nil)
	}
	injuredTemplate := game.repliesManager.ChooseCueTemplate(replies.UserShootState, replies.InjuredReShootReply)
	return dialog.SayTemplate(injuredTemplate, nil)
}

func (game *Game) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	if ctx.IsNewSession() {
		log.Debugf("Created new session")
		ctx.State = State{
			GameState: replies.GeneralState,
		}
	}
	dialog := &dialoglib.Dialog{}
	var err error

	if ctx.GameState == replies.EndGameState {
		if err = game.Fallback(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog)
	if err != nil {
		return nil, err
	} else if isGlobalCommand {
		return dialog.BuildResponse()
	}

	switch ctx.GameState {
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.StartState:
		err = game.StateStart(log, ctx, request, meta, dialog)
	case replies.MyShootState:
		err = game.StateMyShoot(log, ctx, request, meta, dialog)
	case replies.UserShootState:
		err = game.StateUserShoot(log, ctx, request, meta, dialog)
	default:
		err = game.Fallback(log, ctx, request, meta, dialog)
	}

	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}
