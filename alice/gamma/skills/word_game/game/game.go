package game

import (
	"log"
	"math/rand"
	"sort"
	"strings"

	structpb "github.com/golang/protobuf/ptypes/struct"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/buttons"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/default_definitions"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/dictionary"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/replies"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/words"
)

const LettersForPerfectWord = 9
const WordsForStats = 3

func NumberOfWords(num int) string {
	if num%10 == 1 && num%100 != 11 {
		return " слово"
	} else if num%10 >= 2 && num%10 <= 4 && (num%100 < 10 || num%100 >= 20) {
		return " слова"
	}
	return " слов"
}

func CheckWord(ctx *Context, word string) (int, bool) {
	ans := -1
	for i, knownWord := range ctx.RoundState.Words {
		if word == strings.ToLower(knownWord) {
			ans = i
		}
	}
	if ans == -1 {
		return -1, false
	}
	return ans, true
}

func IsAnswer(ctx *Context, checkWord string) bool {
	for _, word := range ctx.RoundState.Words {
		if word == strings.ToLower(checkWord) {
			return true
		}
	}
	return false
}

func IsProperNoun(request *sdk.Request) bool {
	if request.Nlu == nil || request.Nlu.Entities == nil || len(request.Nlu.Entities) == 0 {
		return false
	}
	for _, nluType := range replies.ProperNLU {
		if strings.EqualFold(request.Nlu.Entities[0].Type, nluType) {
			return true
		}
	}
	return false
}

func CheckWordSet(mainWord, word []rune) (int, bool) {
	mapMainWord := make(map[rune]int)

	for _, r := range mainWord {
		mapMainWord[r]++
	}
	for i, r := range word {
		mapMainWord[r]--
		if mapMainWord[r] < 0 {
			return i, false
		}
	}
	return 0, true
}

type WordGame struct {
	RoundContent       []words.RoundContent
	Dictionary         []string
	DefaultDefinitions map[string]string
	repliesManager     replies.Manager
	random             rand.Rand
}

func NewWordGame(random rand.Rand) *WordGame {
	game := &WordGame{
		repliesManager: replies.Manager{RepliesManager: dialoglib.CreateRepliesManager(random, replies.CueTemplates)},
		random:         random,
	}
	var err error
	if game.RoundContent, err = words.GetRoundContent(); err != nil {
		log.Fatal(err)
	}
	if game.Dictionary, err = dictionary.GetDictionary(); err != nil {
		log.Fatal(err)
	}
	if game.DefaultDefinitions, err = defaultdefinitions.GetDefaultDefinitions(); err != nil {
		log.Fatal(err)
	}
	return game
}

func (game *WordGame) ResetContext(ctx *Context) {
	index, str, ok := ctx.RandomMainWord(game.random, game.RoundContent)
	if !ok {
		ctx.UsedMainWordsIndexes = map[int]bool{}
		ctx.LeftMainWords = len(game.RoundContent)
		index, str, _ = ctx.RandomMainWord(game.random, game.RoundContent)
	}
	ctx.LeftMainWords--
	ctx.UsedMainWordsIndexes[index] = true
	ctx.GameState = replies.AfterMainWordChangeState
	ctx.RoundState = RoundState{
		MainWord:         str,
		Words:            game.RoundContent[index].Words,
		WasInTip:         nil,
		UsedWordsIndexes: make(map[int]bool),
		GuessedWords:     0,
		LastWord:         strings.ToLower(game.RoundContent[index].MainWord.Text),
	}
}

func (game *WordGame) RestartGame(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog, generateIntent bool) error {
	game.ResetContext(ctx)
	dialog.AddButtons(buttons.DefaultGameButtons...)
	var RestartGameTemplate *dialoglib.CueTemplate
	if generateIntent {
		RestartGameTemplate = game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.RestartWithChangeIntentGameReply)
	} else {
		RestartGameTemplate = game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.RestartGameReply)
	}
	if err := dialog.SayTemplate(RestartGameTemplate, struct {
		MainWord dialoglib.Cue
	}{
		ctx.RoundState.MainWord,
	}); err != nil {
		return err
	}
	return nil
}

func (game *WordGame) MatchGlobalCommands(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) (bool, error) {
	matches, err := ctx.Match(request, patterns.GlobalCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return false, err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.DontUnderstandFromTipAndRulesIntent:
			if ctx.GameState == replies.TipState {
				return true, game.AboutTip(log, ctx, request, meta, dialog)
			} else {
				return true, game.GetRules(log, ctx, request, meta, dialog)
			}
		case patterns.RulesGameIntent:
			return true, game.GetRules(log, ctx, request, meta, dialog)
		case patterns.ChangeWordIntent:
			return true, game.RestartGame(log, ctx, request, meta, dialog, false)
		case patterns.GetDefinitionIntent:
			word := ctx.RoundState.LastWord
			if len(matches[0].Variables["Any"]) > 0 {
				word = matches[0].Variables["Any"][0].(*structpb.Value).GetStringValue()
			}
			return true, game.GetDefinition(word, log, ctx, request, meta, dialog)
		case patterns.UserChangeWordIntent:
			return true, game.UserChangeWord(log, ctx, request, meta, dialog)
		case patterns.ExtraTalkGood:
			return true, game.ExtraTalkGood(log, ctx, request, meta, dialog)
		case patterns.ExtraTalkAgree:
			return true, game.ExtraTalkAgree(log, ctx, request, meta, dialog)
		case patterns.ExtraTalkThink:
			return true, game.ExtraTalkThink(log, ctx, request, meta, dialog)
		case patterns.EndGameIntent:
			return true, game.EndGame(ctx, dialog)
		}
	}
	return false, nil
}

func (game *WordGame) StateGeneral(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	startGameTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.StartGameReply)
	if err := dialog.SayTemplate(startGameTemplate, struct {
		MainWord  dialoglib.Cue
		IsStation bool
	}{
		ctx.RoundState.MainWord,
		dialoglib.IsYandexStation(meta),
	}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *WordGame) GetRules(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	RulesGameTemplate := game.repliesManager.ChooseCueTemplate(replies.RulesState, replies.RulesGameReply)
	if err := dialog.SayTemplate(RulesGameTemplate, struct {
		IsStation bool
	}{
		dialoglib.IsYandexStation(meta),
	}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultContinueButtons...)
	ctx.GameState = replies.RulesState
	return nil
}

func (game *WordGame) ContinueGame(ctx *Context, dialog *dialoglib.Dialog) error {
	ContinueGameTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.MainWordGameReply)

	if err := dialog.SayTemplate(ContinueGameTemplate, struct {
		MainWord dialoglib.Cue
	}{
		ctx.RoundState.MainWord,
	}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	ctx.GameState = replies.StartGameState
	return nil
}

func (game *WordGame) MakeReply(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	if myReplyIndex, myReply, ok := ctx.RandomWord(game.random, ctx.RoundState.Words); ok {
		myWordTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.MyWordGameReply)
		if err := dialog.SayTemplate(myWordTemplate, struct {
			Reply string
		}{
			strings.ToUpper(myReply),
		}); err != nil {
			return err
		}
		ctx.RoundState.UsedWordsIndexes[myReplyIndex] = true
		ctx.RoundState.LastWord = ctx.RoundState.Words[myReplyIndex]
		if len(ctx.RoundState.UsedWordsIndexes) >= len(ctx.RoundState.Words) {
			GuessedAllTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.AliceAllWordsGuessedReply)
			if err := dialog.SayTemplate(GuessedAllTemplate, nil); err != nil {
				return err
			}
			dialog.AddButtons(buttons.DefaultContinueButtons...)
			ctx.GameState = replies.EndRoundState
		} else {
			mainWordTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.MainWordGameReply)
			if err := dialog.SayTemplate(mainWordTemplate, struct {
				MainWord dialoglib.Cue
			}{
				ctx.RoundState.MainWord,
			}); err != nil {
				return err
			}
		}
	} else {
		GuessedAllTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.UserAllWordsGuessedReply)
		if err := dialog.SayTemplate(GuessedAllTemplate, nil); err != nil {
			return err
		}
		dialog.AddButtons(buttons.DefaultContinueButtons...)
		ctx.GameState = replies.EndRoundState
	}

	return nil
}

func (game *WordGame) UserWord(word string, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	word = strings.ToLower(word)
	if IsProperNoun(request) {
		return game.NotCommonNoun(log, ctx, request, meta, dialog)
	}
	userWordIndex, isCorrect := CheckWord(ctx, word)
	var used bool
	if userWordIndex != -1 {
		used = ctx.RoundState.UsedWordsIndexes[userWordIndex]
	}
	if isCorrect {
		if !used {
			ctx.RoundState.UsedWordsIndexes[userWordIndex] = true
			ctx.RoundState.GuessedWords++
			if len([]rune(word)) >= LettersForPerfectWord {
				perfectWordTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.PerfectUserWordReply)
				if err := dialog.SayTemplate(perfectWordTemplate, nil); err != nil {
					return err
				}
			}
			if err := game.MakeReply(log, ctx, request, meta, dialog); err != nil {
				return err
			}
		} else {
			usedWordTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.UsedWordGameReply)
			if err := dialog.SayTemplate(usedWordTemplate, nil); err != nil {
				return err
			}
		}
	} else if strings.ToUpper(word) == ctx.RoundState.MainWord.Text {
		usingMainWordTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.UsingMainWordGamePeply)
		if err := dialog.SayTemplate(usingMainWordTemplate, nil); err != nil {
			return err
		}
	} else {
		letterIndex, isCorrectSub := CheckWordSet([]rune(strings.ToLower(ctx.RoundState.MainWord.Text)), []rune(word))
		if !isCorrectSub {
			letter := string(([]rune(word))[letterIndex])
			if "0" <= letter && letter <= "9" {
				return game.NotNoun(log, ctx, request, meta, dialog)
			}
			var wrongSubsequenceTemplate *dialoglib.CueTemplate
			if strings.Count(strings.ToLower(ctx.RoundState.MainWord.Text), letter) > 0 {
				wrongSubsequenceTemplate = game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.WrongSubsequenceReply)
			} else {
				wrongSubsequenceTemplate = game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.WrongLetterReply)
			}

			if err := dialog.SayTemplate(wrongSubsequenceTemplate, struct {
				Letter      dialoglib.Cue
				CurrentWord dialoglib.Cue
			}{
				Letter: dialoglib.Cue{
					Text:  letter,
					Voice: replies.LettersTTS[letter],
				},
				CurrentWord: ctx.RoundState.MainWord,
			}); err != nil {
				return err
			}
		} else {
			unknownWordTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.UnknownWordReply)
			if err := dialog.SayTemplate(unknownWordTemplate, nil); err != nil {
				return err
			}
		}
	}
	if ctx.GameState != replies.EndRoundState {
		dialog.AddButtons(buttons.DefaultGameButtons...)
	}
	return nil
}

func (game *WordGame) GetTip(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.TipState
	if ctx.RoundState.WasInTip == nil {
		ctx.RoundState.WasInTip = make(map[string]bool)
	}
	for i, word := range ctx.RoundState.Words {
		newWord := []rune(strings.ToLower(word))
		rand.Shuffle(len(newWord), func(i, j int) { newWord[i], newWord[j] = newWord[j], newWord[i] })
		sortedWord := make([]rune, len(newWord))
		copy(sortedWord, newWord)
		sort.Slice(sortedWord, func(i int, j int) bool { return sortedWord[i] < sortedWord[j] })
		_, used := ctx.RoundState.WasInTip[string(sortedWord)]
		if !ctx.RoundState.UsedWordsIndexes[i] && !used && !IsAnswer(ctx, string(newWord)) {
			ctx.RoundState.WasInTip[string(sortedWord)] = true
			TipGameTemplate := game.repliesManager.ChooseCueTemplate(replies.TipState, replies.TipReply)
			dialog.AddButtons(buttons.DefaultGameButtons...)
			return dialog.SayTemplate(TipGameTemplate, struct {
				Word string
			}{
				string(newWord),
			})
		}
	}
	dialog.AddButtons(buttons.NoTipsButtons...)
	NoTipsTemplate := game.repliesManager.ChooseCueTemplate(replies.TipState, replies.NoTipsReply)
	return dialog.SayTemplate(NoTipsTemplate, nil)
}

func (game *WordGame) NotCommonNoun(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	NotCommonNounGameTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NotCommonNounReply)
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return dialog.SayTemplate(NotCommonNounGameTemplate, nil)
}

func (game *WordGame) NotNoun(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	NotNounGameTemplate := game.repliesManager.ChooseCueTemplate(replies.StartGameState, replies.NotNounReply)
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return dialog.SayTemplate(NotNounGameTemplate, nil)
}

func (game *WordGame) StateStart(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.GameCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.NewTipIntent:
			return game.GetTip(log, ctx, request, meta, dialog)
		case patterns.ChangeWordFromStartIntent:
			return game.RestartGame(log, ctx, request, meta, dialog, false)
		case patterns.StartGameIntent:
			return game.RestartGame(log, ctx, request, meta, dialog, false)
		case patterns.UserWordIntent:
			ctx.GameState = replies.StartGameState
			return game.UserWord(matches[0].Variables["Any"][0].(*structpb.Value).GetStringValue(), log, ctx, request, meta, dialog)
		case patterns.SampleAnswerIntent:
			return game.GiveSampleAnswer(log, ctx, request, meta, dialog)
		}
	}
	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *WordGame) GiveSampleAnswer(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	TipGameTemplate := game.repliesManager.ChooseCueTemplate(replies.TipState, replies.BetterTipReply)
	if err := dialog.SayTemplate(TipGameTemplate, nil); err != nil {
		return err
	}
	return game.GetTip(log, ctx, request, meta, dialog)
}

func (game *WordGame) StateRules(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.ContinueGameCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesContinueGameIntent:
			return game.ContinueGame(ctx, dialog)
		case patterns.NoContinueGameIntent:
			return game.EndGame(ctx, dialog)
		}
	}
	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *WordGame) StateDefinition(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.ContinueGameCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesContinueGameIntent:
			return game.ContinueGame(ctx, dialog)
		case patterns.NoContinueGameIntent:
			return game.EndGame(ctx, dialog)
		}
	}
	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *WordGame) AfterMainWordChangeState(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.MainWordChangeState, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		return game.EndGame(ctx, dialog)
	}
	ctx.GameState = replies.StartGameState
	return game.StateStart(log, ctx, request, meta, dialog)
}

func (game *WordGame) StateUserSetMainWord(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.UserMainWordCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.UserMainWordIntent:
			var word string
			if len(matches[0].Variables["Any"]) > 0 {
				if IsProperNoun(request) {
					return game.NotCommonNoun(log, ctx, request, meta, dialog)
				}
				word = matches[0].Variables["Any"][0].(*structpb.Value).GetStringValue()
				letter := string(([]rune(word))[0])
				if "0" <= letter && letter <= "9" {
					return game.NotNoun(log, ctx, request, meta, dialog)
				}
			}
			return game.UserMainWordInit(word, log, ctx, request, meta, dialog)
		case patterns.UserDontKnow:
			return game.RestartGame(log, ctx, request, meta, dialog, false)
		case patterns.GenerateMainWord:
			return game.RestartGame(log, ctx, request, meta, dialog, true)
		}
	}
	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *WordGame) StateTip(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.TipCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.NextTipIntent:
			return game.GetTip(log, ctx, request, meta, dialog)
		case patterns.DontUnderstandIntent:
			return game.AboutTip(log, ctx, request, meta, dialog)
		}
	} else {
		ctx.GameState = replies.StartGameState
		return game.StateStart(log, ctx, request, meta, dialog)
	}
	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *WordGame) AboutTip(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	AboutTipTemplate := game.repliesManager.ChooseCueTemplate(replies.TipState, replies.AboutTipReply)
	if err := dialog.SayTemplate(AboutTipTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return nil
}

func (game *WordGame) GetDefinition(word string, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	word = strings.ToLower(word)
	var err error = nil
	if res, ok := game.DefaultDefinitions[strings.ToUpper(word)]; ok {
		DefinitionGameTemplate := game.repliesManager.ChooseCueTemplate(replies.DefinitionState, replies.DefinitionWikiReply)
		if err := dialog.SayTemplate(DefinitionGameTemplate, struct {
			Word        string
			Definitions string
		}{
			Word:        strings.ToUpper(word),
			Definitions: res,
		}); err != nil {
			return err
		}
		url := replies.WiktionaryURL + word
		dialog.AddButtons(buttons.NewContinueDefinitionButtons(replies.WiktionaryText, url)...)
	} else {
		DefinitionGameTemplate := game.repliesManager.ChooseCueTemplate(replies.DefinitionState, replies.DefinitionYandexReply)
		if err := dialog.SayTemplate(DefinitionGameTemplate, nil); err != nil {
			return err
		}
		url := replies.YandexURL + word
		dialog.AddButtons(buttons.NewContinueDefinitionButtons(replies.YandexText, url)...)
	}
	ContinueGameTemplate := game.repliesManager.ChooseCueTemplate(replies.DefinitionState, replies.DefinitionContinueReply)
	if err := dialog.SayTemplate(ContinueGameTemplate, nil); err != nil {
		return err
	}
	ctx.GameState = replies.DefinitionState
	return err
}

func (game *WordGame) UserChangeWord(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ctx.GameState = replies.UserSetMainWordState
	UserStateStartTemplate := game.repliesManager.ChooseCueTemplate(replies.UserSetMainWordState, replies.UserStateStartReply)
	if err := dialog.SayTemplate(UserStateStartTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.UserMainWordButtons...)
	return nil
}

func (game *WordGame) GetSmallWords(MainWord string) []string {
	var res []string
	for _, word := range game.Dictionary {
		if _, ok := CheckWordSet([]rune(MainWord), []rune(word)); ok && word != MainWord {
			res = append(res, word)
		}
	}
	rand.Shuffle(len(res), func(i, j int) {
		res[i], res[j] = res[j], res[i]
	})
	return res
}

func (game *WordGame) StateEndRound(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	matches, err := ctx.Match(request, patterns.ContinueGameCommands, &sdk.EmptyEntityExtractor)
	if err != nil {
		return err
	}
	if len(matches) > 0 {
		switch matches[0].Name {
		case patterns.YesContinueGameIntent:
			return game.RestartGame(log, ctx, request, meta, dialog, false)
		case patterns.NoContinueGameIntent:
			return game.EndGame(ctx, dialog)
		}
	}
	return game.Fallback(log, ctx, request, meta, dialog)
}

func (game *WordGame) UserMainWordInit(word string, log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	word = strings.ToUpper(word)
	smallWords := game.GetSmallWords(word)
	if len(smallWords) > 1 {
		game.ResetContext(ctx)
		ctx.RoundState.MainWord = dialoglib.Cue{Text: word, Voice: word}
		ctx.RoundState.Words = smallWords
		ctx.GameState = replies.AfterMainWordChangeState
		CorrectMainWordTemplate := game.repliesManager.ChooseCueTemplate(replies.UserSetMainWordState, replies.CorrectMainWordFromUserReply)
		if err := dialog.SayTemplate(CorrectMainWordTemplate, nil); err != nil {
			return err
		}
		if err := game.MakeReply(log, ctx, request, meta, dialog); err != nil {
			return err
		}
	} else {
		WrongMainWordTemplate := game.repliesManager.ChooseCueTemplate(replies.UserSetMainWordState, replies.WrongMainWordFromUserReply)
		if err := dialog.SayTemplate(WrongMainWordTemplate, struct {
			Word string
		}{
			Word: word,
		}); err != nil {
			return err
		}
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return nil
}

func (game *WordGame) ExtraTalkGood(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ExtraTalkTemplate := game.repliesManager.ChooseCueTemplate(replies.ExtraTalk, replies.ExtraTalkGood)
	if err := dialog.SayTemplate(ExtraTalkTemplate, struct{ Word dialoglib.Cue }{Word: ctx.RoundState.MainWord}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return nil
}

func (game *WordGame) ExtraTalkAgree(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ExtraTalkTemplate := game.repliesManager.ChooseCueTemplate(replies.ExtraTalk, replies.ExtraTalkAgree)
	if err := dialog.SayTemplate(ExtraTalkTemplate, struct{ Word dialoglib.Cue }{Word: ctx.RoundState.MainWord}); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return nil
}

func (game *WordGame) ExtraTalkThink(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	ExtraTalkTemplate := game.repliesManager.ChooseCueTemplate(replies.ExtraTalk, replies.ExtraTalkThink)
	if err := dialog.SayTemplate(ExtraTalkTemplate, nil); err != nil {
		return err
	}
	dialog.AddButtons(buttons.DefaultGameButtons...)
	return nil
}

func (game *WordGame) EndGame(ctx *Context, dialog *dialoglib.Dialog) error {
	if ctx.RoundState.GuessedWords >= WordsForStats {
		phrase := NumberOfWords(ctx.RoundState.GuessedWords)
		statsTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndGameStatsReply)
		if err := dialog.SayTemplate(statsTemplate, struct {
			NumberStats int
			WordStats   string
		}{
			ctx.RoundState.GuessedWords,
			phrase,
		}); err != nil {
			return err
		}
	}
	endGameTemplate := game.repliesManager.ChooseCueTemplate(replies.EndGameState, replies.EndGameReply)
	if err := dialog.SayTemplate(endGameTemplate, nil); err != nil {
		return err
	}
	dialog.EndSession()
	ctx.GameState = replies.EndGameState
	return nil
}

func (game *WordGame) Fallback(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta, dialog *dialoglib.Dialog) error {
	switch ctx.GameState {
	case replies.RulesState:
		fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
		if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
			return err
		}
	case replies.DefinitionState:
		fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
		if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
			return err
		}
	case replies.EndGameState:
		fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
		if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
			return err
		}
	case replies.UserSetMainWordState:
		dialog.AddButtons(buttons.UserMainWordButtons...)
		fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.UserSetMainWordState, replies.FallbackReply)
		if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
			return err
		}
	default:
		dialog.AddButtons(buttons.DefaultGameButtons...)
		fallbackTemplate := game.repliesManager.ChooseCueTemplate(replies.GeneralState, replies.FallbackReply)
		if err := dialog.SayTemplate(fallbackTemplate, nil); err != nil {
			return err
		}
	}
	return nil
}

func (game *WordGame) Handle(log sdk.Logger, ctx *Context, request *sdk.Request, meta *sdk.Meta) (*sdk.Response, error) {
	if request.Nlu == nil {
		log.Debug("There is no NLU in Handle")
	}
	dialog := &dialoglib.Dialog{}
	if ctx.IsNewSession() {
		game.ResetContext(ctx)
		ctx.GameState = replies.GeneralState
		if err := game.StateGeneral(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		}
		return dialog.BuildResponse()
	}
	if ctx.GameState != replies.EndGameState {
		if isGlobalCommand, err := game.MatchGlobalCommands(log, ctx, request, meta, dialog); err != nil {
			return nil, err
		} else if isGlobalCommand {
			return dialog.BuildResponse()
		}
	}
	var err error
	switch ctx.GameState {
	case replies.AfterMainWordChangeState:
		err = game.AfterMainWordChangeState(log, ctx, request, meta, dialog)
	case replies.TipState:
		err = game.StateTip(log, ctx, request, meta, dialog)
	case replies.GeneralState:
		err = game.StateGeneral(log, ctx, request, meta, dialog)
	case replies.StartGameState:
		err = game.StateStart(log, ctx, request, meta, dialog)
	case replies.RulesState:
		err = game.StateRules(log, ctx, request, meta, dialog)
	case replies.DefinitionState:
		err = game.StateDefinition(log, ctx, request, meta, dialog)
	case replies.UserSetMainWordState:
		err = game.StateUserSetMainWord(log, ctx, request, meta, dialog)
	case replies.EndRoundState:
		err = game.StateEndRound(log, ctx, request, meta, dialog)
	default:
		err = game.Fallback(log, ctx, request, meta, dialog)
	}
	if err != nil {
		return nil, err
	}
	return dialog.BuildResponse()
}
