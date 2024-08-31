package tuya

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func Test_NormalizeCustomButtons(t *testing.T) {
	buttons := IRCustomButtons{
		{
			Key:  "1",
			Name: "Назад ",
		},
		{
			Key:  "2",
			Name: "    Вперед",
		},
		{
			Key:  "3",
			Name: "Посере    дине",
		},
	}

	normalizedButtons := IRCustomButtons{
		{
			Key:  "1",
			Name: "Назад",
		},
		{
			Key:  "2",
			Name: "Вперед",
		},
		{
			Key:  "3",
			Name: "Посере дине",
		},
	}

	buttons.Normalize()
	assert.Equal(t, normalizedButtons, buttons)
}
