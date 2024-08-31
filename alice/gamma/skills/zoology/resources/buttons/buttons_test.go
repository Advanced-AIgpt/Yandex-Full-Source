package buttons

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

func TestCreateButtons(t *testing.T) {
	t.Run("no parts", func(t *testing.T) {
		expectedButtons := []sdk.Button{
			{
				Title: "Волк",
				Hide:  true,
			},
			{
				Title: "Лев",
				Hide:  true,
			},
			{
				Title: "Правила",
				Hide:  true,
			},
		}
		buttons, err := CreateButtons("волк", "лев", "junk")
		assert.Equal(t, nil, err)
		assert.Equal(t, expectedButtons, buttons)
	})

	t.Run("parts", func(t *testing.T) {
		expectedButtons := []sdk.Button{
			{
				Title: "У волка",
				Hide:  true,
			},
			{
				Title: "У льва",
				Hide:  true,
			},
			{
				Title: "Правила",
				Hide:  true,
			},
		}
		buttons, err := CreateButtons("волка", "льва", "Parts")
		assert.Equal(t, nil, err)
		assert.Equal(t, expectedButtons, buttons)
	})
}
