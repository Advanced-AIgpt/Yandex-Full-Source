package buttons

import (
	"strings"
	"text/template"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
)

var buttonTemplate = template.Must(template.New("button").Funcs(template.FuncMap{"Title": strings.Title}).Parse(
	"{{if eq .QuestionType \"Parts\"}}" +
		"У {{.Text}}" +
		"{{else}}" +
		"{{Title .Text}}" +
		"{{end}}",
),
)

func createButton(title, questionType string) (sdk.Button, error) {
	var text string
	var err error

	if text, err = dialoglib.ExecuteToString(buttonTemplate, struct {
		Text         string
		QuestionType string
	}{
		Text:         title,
		QuestionType: questionType,
	}); err != nil {
		return sdk.Button{}, err
	}

	return sdk.Button{
		Title: text,
		Hide:  true,
	}, nil
}

func CreateButtons(right, wrong, questionType string) ([]sdk.Button, error) {
	firstButton, err := createButton(right, questionType)
	if err != nil {
		return nil, err
	}

	secondButton, err := createButton(wrong, questionType)
	if err != nil {
		return nil, err
	}

	return []sdk.Button{
		firstButton, secondButton, RulesButton,
	}, nil
}

var YesButton = sdk.Button{
	Title: "Да",
	Hide:  true,
}

var RulesButton = sdk.Button{
	Title: "Правила",
	Hide:  true,
}

var TryAgainButton = sdk.Button{
	Title: "Ещё попытку",
	Hide:  true,
}

var NextQuestionButton = sdk.Button{
	Title: "Следующий вопрос",
	Hide:  true,
}

var PromoButtons = []sdk.Button{
	{
		Title: "«Найди лишнее»",
		Hide:  false,
		URL:   "https://dialogs.yandex.ru/store/skills/dd3f437b-vyberi-lishne",
	},
	{
		Title: "Продолжить игру",
		Hide:  true,
	},
}
