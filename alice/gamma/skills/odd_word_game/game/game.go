package game

import (
	"log"
	"math/rand"
	"strconv"
	"strings"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/levels"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/replies"
)

const (
	LastWord         = "last"
	PenultimateWord  = "penultimate"
	ThreeHiddenWords = 3
	FourHiddenWords  = 4
	LastLevel        = 3
	FirstLevel       = 0
)

type Game struct {
	words          map[string]levels.Word
	levels         []levels.Level
	repliesManager dialoglib.RepliesManager
	random         rand.Rand
	extractor      *sdk.EntityExtractor
}

func NewOddWordGame(random rand.Rand) *Game {
	game := &Game{
		random:         random,
		repliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates),
	}
	var err error
	if game.levels, err = levels.GetLevels(); err != nil {
		log.Fatal(err)
	}
	if game.words, err = levels.GetWords(); err != nil {
		log.Fatal(err)
	}
	game.initExtractor()
	return game
}

func (game *Game) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
	if err := dialog.SayTemplate(startGameTemplate, struct{ IsStation bool }{dialoglib.IsYandexStation(meta)}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.YesButton)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *Game) StateStartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.StartGameCommands, game.extractor)
	log.Debugf("len(matches): %d", len(matches))

	ctx.RoundState.IsFirstQuestion = true
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		log.Debugf("matches[0].Name: %s", matches[0].Name)

		switch matches[0].Name {
		case patterns.YesStartGameIntent:
			var yesStartGameTemplate *dialoglib.CueTemplate
			if ctx.IsFirstGame {
				yesStartGameTemplate = game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.YesStartReply)
				ctx.IsFirstGame = false
			} else {
				yesStartGameTemplate = game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.ContinueReply)
			}
			if err = dialog.SayTemplate(yesStartGameTemplate, nil); err != nil {
				return err
			}
			ctx.GameState = replies.QuestionState
			return game.AskQuestion(log, ctx, request, meta, dialog)
		case patterns.NoStartGameIntent:
			noStartGameTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NoStartReply)
			if err = dialog.SayTemplate(noStartGameTemplate, nil); err != nil {
				return err
			}
			dialog.EndSession()
			ctx.GameState = replies.EndGameState
			return nil
		}
	}
	return game.Fallback(log, dialog)
}

func (game *Game) AskQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	currentTypes := game.levels[ctx.CurrentLevel].PairsOfTypes[game.random.Intn(len(game.levels[ctx.CurrentLevel].PairsOfTypes))]

	r := game.random.Intn(2)
	ctx.RoundState.MajorityTypeID = currentTypes[r]
	ctx.RoundState.OddTypeID = currentTypes[1-r]

	ctx.RoundState.MajorityWords = make([]string, ctx.RoundState.CurrentCountOfMajorityWords)
	rightType := game.levels[ctx.CurrentLevel].Types[ctx.RoundState.MajorityTypeID]
	for i := 0; i < ctx.RoundState.CurrentCountOfMajorityWords; i++ {
		length := len(rightType.Words)
		index := game.random.Intn(length - i)
		ctx.RoundState.MajorityWords[i] = rightType.Words[index]
		rightType.Words[index], rightType.Words[length-i-1] = rightType.Words[length-i-1], rightType.Words[index]
	}
	index := game.random.Intn(len(game.levels[ctx.CurrentLevel].Types))
	ctx.RoundState.OddWord = game.levels[ctx.CurrentLevel].Types[ctx.RoundState.OddTypeID].Words[index]
	ctx.RoundState.CurrentWords = append(ctx.RoundState.MajorityWords, ctx.RoundState.OddWord)
	game.randomSort(ctx.RoundState.CurrentWords)

	var questionTemplate *dialoglib.CueTemplate
	if ctx.RoundState.IsFirstQuestion {
		questionTemplate = game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.FirstQuestionReply)
		ctx.RoundState.IsFirstQuestion = false
	} else {
		questionTemplate = game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.DefaultQuestionReply)
	}
	if err := dialog.SayTemplate(questionTemplate, nil); err != nil {
		return err
	}

	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) StateQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.QuestionCommands, game.extractor)
	if err != nil {
		return err
	}

	if len(matches) > 0 {
		if ctx.RoundState.WantsKnowAnswer {
			ctx.RoundState.WantsKnowAnswer = false
			switch matches[0].Name {
			case patterns.UserWantsToThinkIntent:
				wantThinkTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UserWantsToThinkReply)
				if err = dialog.SayTemplate(wantThinkTemplate, nil); err != nil {
					return err
				}
				return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
			case patterns.UserDoesntWantToThinkIntent:
				return game.SkipQuestion(log, ctx, request, meta, dialog)
			}
		}

		ctx.RoundState.WantsKnowAnswer = false
		switch matches[0].Name {
		case patterns.AnswerIntent:
			return game.ReactForUserAnswer(log, ctx, request, meta, dialog, matches)
		case patterns.GetAnswerIntent:
			return game.GetAnswer(log, ctx, request, meta, dialog)
		case patterns.RepeatQuestionIntent:
			return game.RepeatQuestion(log, ctx, request, meta, dialog)
		case patterns.GetHintIntent:
			return game.GetHint(log, ctx, request, meta, dialog)
		case patterns.UnsuccessfulGetHintIntent:
			badHintTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UnsuccessfulHintReply)
			return dialog.SayTemplate(badHintTemplate, nil)
		case patterns.MeaningIntent:
			id := matches[0].Variables["Word"][0].(string)
			return game.GetMeaning(log, ctx, request, meta, dialog, id)
		case patterns.InvalidAnswerIntent:
			invalidTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.CheatAnswerReply)
			if err = dialog.SayTemplate(invalidTemplate, nil); err != nil {
				return err
			}
			return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
		case patterns.ChangeLevelUpIntent:
			return game.ChangeLevelUp(log, ctx, request, meta, dialog)
		case patterns.ChangeLevelDownIntent:
			return game.ChangeLevelDown(log, ctx, request, meta, dialog)
		}
	}
	return game.Misunderstanding(log, ctx, request, meta, dialog)
}

func (game *Game) EndGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	endgameTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndGameReply)
	dialog.EndSession()
	return dialog.SayTemplate(endgameTemplate, struct {
		CountOfRightAnswers string
		CountOfQuestions    string
		Answer              string
	}{
		CountOfRightAnswers: strconv.Itoa(ctx.RightAnswers),
		CountOfQuestions:    strconv.Itoa(ctx.Answers),
		Answer:              chooseCase(ctx.RightAnswers),
	})
}

func (game *Game) Fallback(log sdk.Logger, dialog *dialoglib.Dialog) error {
	log.Debugf("Fallback")

	fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
	return dialog.SayTemplate(fallbackTemplate, nil)
}

func (game *Game) SkipQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.Answers++
	dontWantThinkTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UserDoesntWantToThinkReply)
	if err := dialog.SayTemplate(dontWantThinkTemplate, nil); err != nil {
		return err
	}
	if err := game.SayAnswer(log, ctx, request, meta, dialog); err != nil {
		return err
	}
	return game.AskQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) ReactForUserAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog,
	matches []sdk.Hypothesis) error {
	ctx.Answers++
	ctx.RoundState.IsEasy = false
	if len(matches[0].Variables["Word"]) > 0 {
		id := matches[0].Variables["Word"][0].(string)
		if ctx.RoundState.OddWord == id {
			return game.RightAnswer(log, ctx, request, meta, dialog)
		}
		for i := range ctx.RoundState.MajorityWords {
			if ctx.RoundState.MajorityWords[i] == id {
				return game.WrongAnswer(log, ctx, request, meta, dialog)
			}
		}
	}
	if len(matches[0].Variables["Number"]) > 0 {
		id := matches[0].Variables["Number"][0].(string)
		var number int
		if id == PenultimateWord {
			number = ctx.RoundState.CurrentCountOfMajorityWords
		} else if id == LastWord {
			number = ctx.RoundState.CurrentCountOfMajorityWords + 1
		} else {
			number = convertStringNumber(id)
		}
		if number > ctx.RoundState.CurrentCountOfMajorityWords+1 && ctx.RoundState.CurrentCountOfMajorityWords == ThreeHiddenWords {
			moreThanCountTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.IncorrectWordNumberReply)
			if err := dialog.SayTemplate(moreThanCountTemplate, nil); err != nil {
				return err
			}
			return game.AskUser(log, ctx, request, meta, dialog)
		}
		if ctx.RoundState.OddWord == ctx.RoundState.CurrentWords[number-1] {
			return game.RightAnswer(log, ctx, request, meta, dialog)
		}
		return game.WrongAnswer(log, ctx, request, meta, dialog)
	}
	notNowWordTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.InvalidAnswerReply)
	if err := dialog.SayTemplate(notNowWordTemplate, nil); err != nil {
		return err
	}
	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) RepeatQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	repeatTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.RepeatReply)
	if err := dialog.SayTemplate(repeatTemplate, nil); err != nil {
		return err
	}
	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) GetHint(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	hintTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.SuccessfulHintReply)
	if err := dialog.SayTemplate(hintTemplate, struct {
		Type string
	}{
		Type: game.levels[ctx.CurrentLevel].Types[ctx.RoundState.MajorityTypeID].PluralForm,
	}); err != nil {
		return err
	}
	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) GetAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if !ctx.RoundState.IsEasy {
		askAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UserDoesntKnowReply)
		ctx.RoundState.WantsKnowAnswer = true
		dialog.AddButtons(buttons.ChooseButtons...)
		return dialog.SayTemplate(askAnswerTemplate, nil)
	}
	answerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UserDoesntWantToThinkReply)
	if err := dialog.SayTemplate(answerTemplate, nil); err != nil {
		return err
	}
	if err := game.SayAnswer(log, ctx, request, meta, dialog); err != nil {
		return err
	}
	return game.AskQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) GetMeaning(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog,
	id string) error {
	word := game.words[id]
	type_ := word.Types[ctx.CurrentLevel+1]
	meaningTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.MeaningOfTheWordReply)
	if err := dialog.SayTemplate(meaningTemplate, struct {
		Text  string
		Voice string
		Type  string
	}{
		Text:  title(word.Word.Text),
		Voice: title(word.Word.Voice),
		Type:  type_,
	}); err != nil {
		return err
	}
	askUserTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.GuessOddWordReply)
	if err := dialog.SayTemplate(askUserTemplate, nil); err != nil {
		return err
	}
	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) ChangeLevelUp(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if ctx.CurrentLevel == LastLevel {
		unsuccessfulChangeTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UnsuccessfulLevelUpReply)
		if err := dialog.SayTemplate(unsuccessfulChangeTemplate, nil); err != nil {
			return err
		}
		return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
	}
	successfulChangeTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.SuccessfulLevelUpReply)
	if err := dialog.SayTemplate(successfulChangeTemplate, nil); err != nil {
		return err
	}
	ctx.CurrentLevel++
	return game.AskQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) ChangeLevelDown(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if ctx.CurrentLevel == FirstLevel {
		ctx.RoundState.IsEasy = true
		unsuccessfulChangeTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.UnsuccessfulLevelDownReply)
		if err := dialog.SayTemplate(unsuccessfulChangeTemplate, nil); err != nil {
			return err
		}
		return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
	}
	successfulChangeTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.SuccessfulLevelDownReply)
	if err := dialog.SayTemplate(successfulChangeTemplate, nil); err != nil {
		return err
	}
	ctx.CurrentLevel--
	return game.AskQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) SayAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	answerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.LoseExplanationReply)

	return dialog.SayTemplate(answerTemplate, struct {
		OddWord     dialoglib.Cue
		CorrectType string
		OtherType   string
	}{
		OddWord:     game.words[ctx.RoundState.OddWord].Word,
		CorrectType: game.levels[ctx.CurrentLevel].Types[ctx.RoundState.OddTypeID].Name,
		OtherType:   game.levels[ctx.CurrentLevel].Types[ctx.RoundState.MajorityTypeID].PluralForm,
	})
}

func (game *Game) SayCurrentQuestion(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	questionTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.CurrentQuestionReply)
	dialog.AddButtons(game.createButtons(ctx.RoundState.CurrentWords)...)
	words := make([]levels.Word, 0, len(ctx.RoundState.CurrentWords))
	for _, wordIndex := range ctx.RoundState.CurrentWords {
		words = append(words, game.words[wordIndex])
	}
	return dialog.SayTemplate(questionTemplate, struct {
		Words []levels.Word
	}{
		Words: words,
	})
}

func (game *Game) Misunderstanding(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	misunderstandingTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.MisUnderstandingReply)
	return dialog.SayTemplate(misunderstandingTemplate, nil)
}

func (game *Game) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, game.extractor)
	log.Debugf("len(matches): %d", len(matches))

	if err != nil {
		return false, err
	}
	if len(matches) > 0 {
		log.Debugf("matches[0].Name: %s", matches[0].Name)

		switch matches[0].Name {
		case patterns.StartGameIntent:
			ctx.IsFirstGame = true
			ctx.GameState = replies.StartGameState
			startTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
			return true, dialog.SayTemplate(startTemplate, struct {
				IsStation bool
			}{
				IsStation: dialoglib.IsYandexStation(meta),
			})
		case patterns.EndGameIntent:
			if ctx.GameState == replies.StartGameState {
				err = game.NoStartGame(log, ctx, request, meta, dialog)
			} else {
				err = game.EndGame(log, ctx, request, meta, dialog)
			}
			ctx.GameState = replies.EndGameState
			return true, err
		}
	}
	return false, nil
}

func (game *Game) RightAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.RightAnswers++
	if ctx.RoundState.CurrentCountOfMajorityWords == FourHiddenWords {
		if ctx.CurrentLevel == LastLevel {
			ctx.RoundState.CurrentCountOfMajorityWords = ThreeHiddenWords
			winTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.WinningReply)
			ctx.GameState = replies.StartGameState
			ctx.CurrentLevel = FirstLevel
			return dialog.SayTemplate(winTemplate, struct {
				CountOfRightAnswers int
				CountOfQuestions    int
				Answer              string
			}{
				CountOfRightAnswers: ctx.RightAnswers,
				CountOfQuestions:    ctx.Answers,
				Answer:              chooseCase(ctx.RightAnswers),
			})
		}
	} else {
		ctx.RoundState.CurrentCountOfMajorityWords++
	}
	successTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.RightAnswerReply)
	if err := dialog.SayTemplate(successTemplate, nil); err != nil {
		return err
	}
	explanationTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.SuccessExplanationReply)
	if err := dialog.SayTemplate(explanationTemplate, struct {
		Text        string
		Voice       string
		CorrectType string
		OtherType   string
	}{
		Text:        title(game.words[ctx.RoundState.OddWord].Word.Text),
		Voice:       title(game.words[ctx.RoundState.OddWord].Word.Voice),
		CorrectType: game.levels[ctx.CurrentLevel].Types[ctx.RoundState.OddTypeID].Name,
		OtherType:   game.levels[ctx.CurrentLevel].Types[ctx.RoundState.MajorityTypeID].PluralForm,
	}); err != nil {
		return err
	}
	return game.AskQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) WrongAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	wrongAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.WrongAnswerReply)
	if err := dialog.SayTemplate(wrongAnswerTemplate, nil); err != nil {
		return err
	}
	if err := game.SayAnswer(log, ctx, request, meta, dialog); err != nil {
		return err
	}
	if ctx.RoundState.CurrentCountOfMajorityWords == ThreeHiddenWords {
		if ctx.CurrentLevel != FirstLevel {
			ctx.CurrentLevel--
			ctx.RoundState.CurrentCountOfMajorityWords = FourHiddenWords
		}
	} else {
		ctx.RoundState.CurrentCountOfMajorityWords = ThreeHiddenWords
	}
	return game.AskQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) AskUser(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	askUserToSayAnswerTemplate := game.repliesManager.ChooseCueTemplate(replies.QuestionState, replies.InvalidAnswerReply)
	if err := dialog.SayTemplate(askUserToSayAnswerTemplate, nil); err != nil {
		return err
	}
	return game.SayCurrentQuestion(log, ctx, request, meta, dialog)
}

func (game *Game) NoStartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	noStartGameTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NoStartReply)
	if err := dialog.SayTemplate(noStartGameTemplate, nil); err != nil {
		return err
	}
	dialog.EndSession()
	ctx.GameState = replies.EndGameState
	return nil
}

func (game *Game) randomSort(words []string) {
	for i := 0; i < len(words); i++ {
		index := game.random.Intn(len(words)-i) + i
		words[i], words[index] = words[index], words[i]
	}
}

func (game *Game) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	dialog := &dialoglib.Dialog{}
	if ctx.IsNewSession() {
		ctx.GameState = replies.GeneralState
		ctx.Answers = 0
		ctx.RightAnswers = 0
		ctx.IsFirstGame = true
		ctx.RoundState.CurrentCountOfMajorityWords = ThreeHiddenWords
		if err := game.StateGeneral(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if ctx.GameState == replies.EndGameState {
		if err := game.Fallback(log, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}

	if isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog); err != nil {
		return nil, err
	} else if isGlobalCommand {
		return dialog.BuildResponse()
	}

	log.Debugf("new state: %s", ctx.GameState)

	var err error
	switch ctx.GameState {
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.StartGameState:
		err = game.StateStartGame(log, ctx, request, meta, dialog)
	case replies.QuestionState:
		err = game.StateQuestion(log, ctx, request, meta, dialog)
	}

	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}

func chooseCase(number int) string {
	n := number % 10
	if n == 1 {
		return "правильный ответ"
	}
	if n == 2 || n == 3 || n == 4 {
		return "правильных ответа"
	}
	return "правильных ответов"
}

func (game *Game) initExtractor() {
	entities := make(map[string]map[string][]string)

	entities["Word"] = make(map[string][]string)
	for i, word := range game.words {
		entities["Word"][i] = []string{strings.ReplaceAll(word.Word.Text, "ё", "е")}
	}

	entities["Number"] = make(map[string][]string)
	entities["Number"]["first"] = []string{"первое", "первый", "первая", "1 слово"}
	entities["Number"]["second"] = []string{"второе", "второй", "вторая", "2 слово"}
	entities["Number"]["third"] = []string{"третье", "третий", "третья", "3 слово"}
	entities["Number"]["fourth"] = []string{"четвертое", "четвертый", "четвертая", "4 слово"}
	entities["Number"]["fifth"] = []string{"пятое", "пятый", "пятая", "5 слово"}
	entities["Number"][PenultimateWord] = []string{"предпоследнее", "предпоследний", "предпоследняя"}
	entities["Number"][LastWord] = []string{"последнее", "последний", "последняя"}

	game.extractor = sdk.NewEntityExtractor(entities)
}

func convertStringNumber(number string) int {
	switch number {
	case "first":
		return 1
	case "second":
		return 2
	case "third":
		return 3
	case "fourth":
		return 4
	case "fifth":
		return 5
	}
	return -1
}

func (game *Game) createButtons(words []string) []sdk.Button {
	wordsButtons := make([]sdk.Button, len(words))
	for i := range words {
		wordsButtons[i] = sdk.Button{
			Title: game.words[words[i]].Word.Text,
			Hide:  true,
		}
	}
	return wordsButtons
}

func title(phrase string) string {
	tokens := strings.SplitN(phrase, " ", 2)
	tokens[0] = strings.Title(tokens[0])
	return strings.Join(tokens, " ")
}
