package questions

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/library/go/core/resource"

	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/patterns"
)

type Question struct {
	Type      string        `json:"questionType"`
	Fact      dialoglib.Cue `json:"fact"`
	YesAnswer dialoglib.Cue `json:"yes"`
	NoAnswer  dialoglib.Cue `json:"no"`
	URL       string        `json:"url"`
	IsTruth   bool          `json:"correct"`
}

func (question *Question) IsCorrect(answer string) bool {
	if question.IsTruth {
		return answer == patterns.TruthAnswerIntent
	} else {
		return answer == patterns.LiesAnswerIntent
	}
}

func GetQuestions() (questions []Question, err error) {
	if err = json.Unmarshal(resource.Get("facts.json"), &questions); err != nil {
		return nil, xerrors.Errorf("Resource is an invalid json resource")
	}
	return questions, nil
}
