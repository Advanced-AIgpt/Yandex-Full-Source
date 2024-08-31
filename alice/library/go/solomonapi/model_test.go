package solomonapi

import (
	"encoding/json"
	"github.com/stretchr/testify/assert"
	"testing"

	"a.yandex-team.ru/library/go/ptr"
)

func TestParseFloatWithNaN(t *testing.T) {
	raw := `[41.0,49.0,47.0,"NaN",44.0]`
	var actual []FloatWithNaN
	err := json.Unmarshal([]byte(raw), &actual)
	if assert.NoError(t, err) {
		expected := []FloatWithNaN{
			{Val: ptr.Float64(41.0)},
			{Val: ptr.Float64(49.0)},
			{Val: ptr.Float64(47.0)},
			{Val: nil},
			{Val: ptr.Float64(44.0)},
		}
		assert.Equal(t, expected, actual)
	}
}
