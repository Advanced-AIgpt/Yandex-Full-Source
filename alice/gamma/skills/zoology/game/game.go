package game

import (
	"log"
	"math/rand"
	"strings"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/animals"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/replies"
)

const (
	OkAnswer        = "ok"
	DontKnowAnswer  = "dont_know"
	NegativeAnswer  = "negative"
	UncertainAnswer = "uncertain"
	UndefinedAnswer = "undefined"
)

const (
	AnimalQuestion         = "Animal"
	CoatingQuestion        = "Coating"
	EmploymentQuestion     = "Employment"
	ExtremitiesQuestion    = "Extremities"
	NumExtremitiesQuestion = "NUMBER"
	ClassQuestion          = "Class"
	SavageryQuestion       = "Savagery"
	PartsQuestion          = "Parts"
)

const OrderAnswer = "Order"
const CountOfQuestion = 5

type Part struct {
	Cue  dialoglib.Cue
	Name string
}

type Game struct {
	repliesManager dialoglib.RepliesManager
	random         rand.Rand
	extractor      *sdk.EntityExtractor
	questionTypes  []string
	parts          map[string]animals.Property
	animals        []animals.Animal
}

func NewZoologyGame(random rand.Rand) *Game {
	game := &Game{
		repliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates),
		random:         random,
	}

	var err error
	if game.animals, err = animals.GetAnimals(); err != nil {
		log.Fatal(err)
	}

	animalsPatterns, err := animals.GetPatterns()
	if err != nil {
		log.Fatal(err)
	}
	game.extractor = sdk.NewEntityExtractor(animalsPatterns)

	game.initTypes()
	game.parts = animals.GetParts()

	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.StartGameState

	startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
	if err := dialog.SayTemplate(startGameTemplate, struct {
		IsStation bool
	}{
		IsStation: dialoglib.IsYandexStation(meta),
	}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.YesButton, buttons.RulesButton)

	return nil
}

func (game *Game) StateStartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.SelectCommands, game.extractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesIntent:
			yesStartTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.YesStartReply)
			if err := dialog.SayTemplate(yesStartTemplate, nil); err != nil {
				return err
			}
			ctx.GameState = replies.QuestionState
			return game.NextQuestion(log, ctx, request, meta, dialog)
		case patterns.NoIntent:
			noStartTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NoStartReply)
			if err = dialog.SayTemplate(noStartTemplate, nil); err != nil {
				return err
			}
			dialog.EndSession()
			ctx.GameState = replies.EndGameState
			return nil
		}
	}

	return game.FallBack(log, ctx, request, meta, dialog)
}

func (game *Game) StateQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.QuestionCommands, game.extractor)
	if err != nil {
		return err
	}

	if ctx.RoundState.LastAnswerType == UndefinedAnswer {
		if len(matches) > 0 {
			if matches[0].Name == patterns.YesContinueIntent {
				cantUnderstandTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.CantUnderstandReply)
				return dialog.SayTemplate(cantUnderstandTemplate, nil)
			}
		}
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.AnswerIntent:
			var key string
			if ctx.RoundState.CurrentType == PartsQuestion || ctx.RoundState.CurrentType == EmploymentQuestion {
				key = AnimalQuestion
			} else {
				key = ctx.RoundState.CurrentType
			}
			if len(matches[0].Variables[key]) > 0 {
				var IsRight bool
				if key == NumExtremitiesQuestion {
					IsRight = ctx.RoundState.CurrentQuestion.Value == game.GetNumberValue(matches)
				} else {
					IsRight = matches[0].Variables[key][0] == ctx.RoundState.CurrentQuestion.Value
				}
				if IsRight {
					return game.RightAnswer(log, ctx, request, meta, dialog)
				}
				return game.WrongAnswer(log, ctx, request, meta, dialog)
			} else if len(matches[0].Variables[OrderAnswer]) > 0 && ctx.RoundState.CurrentType != NumExtremitiesQuestion {
				if matches[0].Variables[OrderAnswer][0].(string) == ctx.RoundState.CurrentQuestion.Order {
					return game.RightAnswer(log, ctx, request, meta, dialog)
				}
				return game.WrongAnswer(log, ctx, request, meta, dialog)
			}
		case patterns.InvalidAnswerIntent:
			return game.ReactForInvalidAnswer(log, ctx, request, meta, dialog, replies.InvalidAnswersReply, DontKnowAnswer)
		case patterns.UserDoesntKnowIntent:
			return game.ReactForInvalidAnswer(log, ctx, request, meta, dialog, replies.AskToGuessReply, DontKnowAnswer)
		case patterns.NegativeAnswerIntent:
			return game.ReactForInvalidAnswer(log, ctx, request, meta, dialog, replies.AskRightAnswerReply, NegativeAnswer)
		case patterns.UncertainAnswerIntent:
			return game.ReactForInvalidAnswer(log, ctx, request, meta, dialog, replies.NotNumberAnswerReply, UncertainAnswer)
		case patterns.UndefinedAnswerIntent:
			return game.ReactForInvalidAnswer(log, ctx, request, meta, dialog, replies.UndefinedAnswerReply, UndefinedAnswer)
		case patterns.NoContinueIntent:
			ctx.RoundState.LastAnswerType = OkAnswer
			dontTryTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.DontTryReply)
			if err = dialog.SayTemplate(dontTryTemplate, nil); err != nil {
				return err
			}
			return game.NextQuestion(log, ctx, request, meta, dialog)
		}
	}

	return game.ReactForInvalidAnswer(log, ctx, request, meta, dialog, replies.UndefinedAnswerReply, UndefinedAnswer)
}

func (game *Game) StateRules(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.SelectCommands, game.extractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesIntent:
			ctx.GameState = replies.QuestionState
			if ctx.RoundState.CurrentQuestion != nil {
				return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
			}
			return game.NextQuestion(log, ctx, request, meta, dialog)
		case patterns.NoIntent:
			ctx.GameState = replies.EndGameState
			if ctx.RoundState.CurrentQuestion != nil {
				return game.StateEndGame(log, ctx, request, meta, dialog)
			}
			noContinueTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NoStartReply)
			return dialog.SayTemplate(noContinueTemplate, nil)
		}
	}

	return game.FallBack(log, ctx, request, meta, dialog)
}

func (game *Game) StateEndGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	endTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndGameReply)
	if err := dialog.SayTemplate(endTemplate, nil); err != nil {
		return err
	}
	dialog.EndSession()
	return game.SayResult(log, ctx, request, meta, dialog, true)
}

func (game *Game) FallBack(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	return dialog.SayTemplate(fallbackTemplate, nil)
}

func (game *Game) NextQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.RoundState.LastAnswerType = OkAnswer
	if ctx.RoundState.Questions == CountOfQuestion {
		if err := game.SayResult(log, ctx, request, meta, dialog, true); err != nil {
			return err
		}

		ctx.RoundState.Questions = 0
		ctx.RoundState.RightAnswers = 0

		nextRoundTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.NextRoundReply)
		if err := dialog.SayTemplate(nextRoundTemplate, nil); err != nil {
			return err
		}
		ctx.GameState = replies.StartGameState

		if ctx.IsPromo && dialoglib.IsDeviceAllowedForPromo(meta) {
			ctx.IsPromo = false
			dialog.AddButtons(buttons.PromoButtons...)
			promoTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.PromoReply)
			return dialog.SayTemplate(promoTemplate, nil)
		} else {
			dialog.AddButtons(buttons.YesButton, buttons.RulesButton)
		}
	} else {
		return game.AskQuestion(log, ctx, request, meta, dialog)
	}
	return nil
}

func (game *Game) RightAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.RoundState.RightAnswers++
	rightAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.RightAnswerReply)
	if err := dialog.SayTemplate(rightAnswerTemplate, nil); err != nil {
		return err
	}
	return game.NextQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) WrongAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	wrongAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.WrongAnswerReply)
	if err := dialog.SayTemplate(wrongAnswerTemplate, nil); err != nil {
		return err
	}
	return game.NextQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) GetNumberValue(hypothesis []sdk.Hypothesis) interface{} {
	if len(hypothesis[0].Variables[NumExtremitiesQuestion]) > 0 {
		obj := hypothesis[0].Variables[NumExtremitiesQuestion][0].(map[string]interface{})

		return obj["Kind"].(map[string]interface{})["NumberValue"]
	}
	return nil
}

func (game *Game) GenerateAnimal(ctx *Context) (*animals.Animal, int) {
	animalsIndexes := make([]int, 0)
	for i, animal := range game.animals {
		if i != ctx.RoundState.PreviousAnimal {
			if ctx.RoundState.CurrentType == ExtremitiesQuestion || ctx.RoundState.CurrentType == PartsQuestion {
				for key := range game.parts {
					if animal.HasProperty(key) {
						animalsIndexes = append(animalsIndexes, i)
						break
					}
				}
			} else if animal.HasProperty(ctx.RoundState.CurrentType) {
				animalsIndexes = append(animalsIndexes, i)
			}
		}
	}
	index := game.getRandomIntItem(animalsIndexes)
	return &game.animals[index], index
}

func (game *Game) GenerateQuestion(ctx *Context) error {
	question := &Question{}
	ctx.RoundState.CurrentType = game.getRandomStringItem(game.questionTypes)
	animal, index := game.GenerateAnimal(ctx)
	questionType := ctx.RoundState.CurrentType
	ctx.RoundState.PreviousAnimal = index

	switch questionType {
	case ExtremitiesQuestion:
		question.Value, question.Right, question.Wrong = game.GenerateExtremities(animal)
		question.KeyWord = animal.GenitiveForm
	case PartsQuestion:
		question.Value = animal.ID
		parts := make([]string, 0)
		for part := range game.parts {
			if animal.HasProperty(part) {
				parts = append(parts, part)
			}
		}
		part := game.getRandomStringItem(parts)
		question.Right, question.Wrong, question.KeyWord = game.GenerateParts(animal, part)
	case EmploymentQuestion:
		question.KeyWord = animal.Properties[questionType].Cue
		question.Value = animal.ID
		question.Right = animal.Name
		question.Wrong = game.GeneratePair(animal, questionType)
	case SavageryQuestion:
		fallthrough
	case ClassQuestion:
		question.KeyWord = dialoglib.Cue{
			Text:  strings.Title(animal.Name.Text),
			Voice: strings.Title(animal.Name.Voice),
		}
		question.Right = animal.Properties[questionType].Cue
		question.Value = animal.Properties[questionType].Value
		question.Wrong = game.GeneratePair(animal, questionType)
	default:
		question.KeyWord = animal.GenitiveForm
		if questionType != NumExtremitiesQuestion {
			question.Right = animal.Properties[questionType].Cue
		} else {
			question.Right = animal.Properties[ExtremitiesQuestion].Cue
		}
		question.Value = animal.Properties[questionType].Value
		question.Wrong = game.GeneratePair(animal, questionType)
	}

	if questionType != NumExtremitiesQuestion {
		var err error
		question.Buttons, err = buttons.CreateButtons(question.Right.Text, question.Wrong.Text, questionType)
		if err != nil {
			return err
		}
	}
	ctx.RoundState.CurrentQuestion = question
	return nil
}

func (game *Game) GeneratePair(animal *animals.Animal, questionType string) dialoglib.Cue {
	animalsIndexes := make([]int, 0)
	for i, animal_ := range game.animals {
		if animal_.HasProperty(questionType) && animal_.Properties[questionType] != animal.Properties[questionType] {
			animalsIndexes = append(animalsIndexes, i)
		}
	}

	secondAnimal := game.animals[game.getRandomIntItem(animalsIndexes)]

	if questionType != EmploymentQuestion {
		return secondAnimal.Properties[questionType].Cue
	}
	return secondAnimal.Name
}

func (game *Game) GenerateExtremities(animal *animals.Animal) (string, dialoglib.Cue, dialoglib.Cue) {
	var right, wrong dialoglib.Cue
	var id string
	hasExtremities := make([]string, 0)
	hasNotExtremities := make([]string, 0)
	for key := range game.parts {
		if animal.HasProperty(key) {
			hasExtremities = append(hasExtremities, key)
		} else {
			hasNotExtremities = append(hasNotExtremities, key)
		}
	}
	extremity := game.parts[game.getRandomStringItem(hasExtremities)]
	right = extremity.Cue
	id = extremity.Value.(string)
	wrong = game.parts[game.getRandomStringItem(hasNotExtremities)].Cue
	return id, right, wrong
}

func (game *Game) GenerateParts(animal *animals.Animal, questionType string) (dialoglib.Cue, dialoglib.Cue, dialoglib.Cue) {
	var right, wrong dialoglib.Cue
	extremity := game.parts[questionType].Cue
	right = animal.GenitiveForm
	animalsIndexes := make([]int, 0)
	for i, animal_ := range game.animals {
		if !animal_.HasProperty(questionType) {
			animalsIndexes = append(animalsIndexes, i)
		}
	}
	wrong = game.animals[game.getRandomIntItem(animalsIndexes)].GenitiveForm
	return right, wrong, extremity
}

func (game *Game) SayRules(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	rulesTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.RulesReply)
	if err := dialog.SayTemplate(rulesTemplate, nil); err != nil {
		return err
	}

	dialog.AddButtons(buttons.YesButton)
	ctx.GameState = replies.RulesState

	return nil
}

func (game *Game) RestartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.QuestionState
	restartTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.RestartReply)
	if err := dialog.SayTemplate(restartTemplate, nil); err != nil {
		return err
	}

	return game.NextQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) SayHowDoesAliceKnow(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	howAliceKnowTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.HowDoesAliceKnowReply)
	if err := dialog.SayTemplate(howAliceKnowTemplate, nil); err != nil {
		return err
	}

	if ctx.RoundState.CurrentQuestion != nil {
		return game.RepeatQuestion(log, ctx, request, meta, dialog)
	}
	return nil
}

func (game *Game) SayResult(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog, isEnd bool) error {
	if !isEnd && ctx.RoundState.Questions == 0 {
		cantSayResultTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.CantSayResultReply)
		return dialog.SayTemplate(cantSayResultTemplate, nil)
	}

	if ctx.RoundState.Questions == 0 {
		return nil
	}

	if ctx.RoundState.RightAnswers == 0 {
		if ctx.RoundState.Questions == 1 {
			oneLoseTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.OneLoseQuestionReply)
			if err := dialog.SayTemplate(oneLoseTemplate, nil); err != nil {
				return err
			}
		} else {
			loseTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.LoseResultReply)
			if err := dialog.SayTemplate(loseTemplate, struct {
				Questions int
			}{
				Questions: ctx.RoundState.Questions,
			}); err != nil {
				return err
			}
		}
		if isEnd {
			comfortTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.ComfortUserReply)
			if err := dialog.SayTemplate(comfortTemplate, nil); err != nil {
				return err
			}
		}
		return nil
	}

	if ctx.RoundState.RightAnswers == ctx.RoundState.Questions {
		if ctx.RoundState.Questions == 1 {
			oneSuccessTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.OneSuccessQuestionReply)
			if err := dialog.SayTemplate(oneSuccessTemplate, nil); err != nil {
				return err
			}
		} else {
			successTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.SuccessResultReply)
			if err := dialog.SayTemplate(successTemplate, struct {
				Count   int
				Answers string
			}{
				Count:   ctx.RoundState.RightAnswers,
				Answers: chooseCase(ctx.RoundState.RightAnswers),
			}); err != nil {
				return err
			}
		}
		if isEnd {
			complimentTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.ComplimentUserReply)
			return dialog.SayTemplate(complimentTemplate, nil)
		}
		return nil
	}

	resultTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.ResultReply)
	if err := dialog.SayTemplate(resultTemplate, struct {
		Questions int
		Count     int
		Answers   string
	}{
		Questions: ctx.RoundState.Questions,
		Count:     ctx.RoundState.RightAnswers,
		Answers:   chooseCase(ctx.RoundState.RightAnswers),
	}); err != nil {
		return err
	}
	if isEnd {
		stimulateTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.StimulateUserReply)
		return dialog.SayTemplate(stimulateTemplate, struct {
			Count   int
			Answers string
		}{
			Count:   ctx.RoundState.RightAnswers,
			Answers: chooseCase(ctx.RoundState.RightAnswers),
		})
	}
	return nil
}

func (game *Game) RepeatQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	repeatTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.RepeatQuestionReply)
	if err := dialog.SayTemplate(repeatTemplate, nil); err != nil {
		return err
	}
	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func chooseCase(count int) string {
	if count%10 == 1 && count%100 != 11 {
		return "вопрос"
	}
	if count%10 >= 2 && count%10 <= 4 && (count%100 < 10 || count%10 >= 20) {
		return "вопроса"
	}
	return "вопросов"
}

func (game *Game) SayCurrentQuestion(logger sdk.Logger, context *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	return game.SayQuestion(context, dialog)
}

func (game *Game) SkipQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if ctx.RoundState.LastAnswerType == UndefinedAnswer {
		misunderstandingTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.MisunderstandingReply)
		if err := dialog.SayTemplate(misunderstandingTemplate, nil); err != nil {
			return err
		}
	} else {
		answerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.SkipQuestionReply)
		var rightAnswer dialoglib.Cue
		if ctx.RoundState.CurrentType == NumExtremitiesQuestion {
			rightAnswer = game.animals[ctx.RoundState.PreviousAnimal].Properties[NumExtremitiesQuestion].Cue
		} else {
			rightAnswer = ctx.RoundState.CurrentQuestion.Right
		}
		if err := dialog.SayTemplate(answerTemplate, struct {
			RightAnswer  dialoglib.Cue
			QuestionType string
		}{
			RightAnswer:  rightAnswer,
			QuestionType: ctx.RoundState.CurrentType,
		}); err != nil {
			return err
		}
	}
	ctx.RoundState.LastAnswerType = OkAnswer
	return game.NextQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) ReactForInvalidAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog,
	reply string, answerType string) error {
	if ctx.RoundState.LastAnswerType == answerType {
		return game.SkipQuestion(log, ctx, request, meta, dialog)
	}

	invalidAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, reply)
	if err := dialog.SayTemplate(invalidAnswerTemplate, nil); err != nil {
		return err
	}
	ctx.RoundState.LastAnswerType = answerType

	if answerType != UndefinedAnswer {
		return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
	} else {
		dialog.AddButtons(buttons.TryAgainButton, buttons.NextQuestionButton)
	}

	return nil
}

func (game *Game) AskQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if err := game.GenerateQuestion(ctx); err != nil {
		return err
	}
	ctx.RoundState.Questions++
	return game.SayQuestion(ctx, dialog)
}

func (game *Game) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, game.extractor)
	if err != nil {
		return false, err
	}

	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.StartGameIntent:
			return true, game.StateGeneral(log, ctx, request, meta, dialog)
		case patterns.RulesIntent:
			return true, game.SayRules(log, ctx, request, meta, dialog)
		case patterns.RestartGameIntent:
			if ctx.RoundState.LastAnswerType == UndefinedAnswer {
				return true, game.RepeatQuestion(log, ctx, request, meta, dialog)
			}
			return true, game.RestartGame(log, ctx, request, meta, dialog)
		case patterns.HowDoesAliceKnowIntent:
			return true, game.SayHowDoesAliceKnow(log, ctx, request, meta, dialog)
		case patterns.ResultIntent:
			return true, game.SayResult(log, ctx, request, meta, dialog, false)
		case patterns.EndGameIntent:
			return true, game.StateEndGame(log, ctx, request, meta, dialog)
		}
	}

	return false, nil
}

func (game *Game) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	dialog := &dialoglib.Dialog{}
	if ctx.IsNewSession() {
		ctx.GameState = replies.GeneralState
		ctx.IsPromo = true
		ctx.RoundState = RoundState{}
		if err := game.StateGeneral(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if ctx.GameState == replies.EndGameState {
		if err := game.FallBack(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog); err != nil {
		return nil, err
	} else if isGlobalCommand {
		return dialog.BuildResponse()
	}

	var err error
	switch ctx.GameState {
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.StartGameState:
		err = game.StateStartGame(log, ctx, request, meta, dialog)
	case replies.RulesState:
		err = game.StateRules(log, ctx, request, meta, dialog)
	case replies.QuestionState:
		err = game.StateQuestion(log, ctx, request, meta, dialog)
	default:
		err = game.FallBack(log, ctx, request, meta, dialog)
	}

	if err != nil {
		return nil, err
	}

	return dialog.BuildResponse()
}

func (game *Game) initTypes() {
	game.questionTypes = []string{
		CoatingQuestion, EmploymentQuestion, ExtremitiesQuestion, NumExtremitiesQuestion, ClassQuestion, SavageryQuestion, PartsQuestion,
	}
}

func (game *Game) getRandomIntItem(slice []int) int {
	return slice[game.random.Intn(len(slice))]
}

func (game *Game) getRandomStringItem(slice []string) string {
	return slice[game.random.Intn(len(slice))]
}
