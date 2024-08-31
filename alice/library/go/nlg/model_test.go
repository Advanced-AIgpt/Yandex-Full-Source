package libnlg

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestNLG_Clone(t *testing.T) {
	ok := NLG{
		NewAsset("Сплюнь через правое плечо 5 раз", "Найдешь под подушкой айфон"),
	}
	x := ok.Clone()
	assert.Equal(t, ok, x)
}
