package tools

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestPrettyJSONMessage(t *testing.T) {
	assert.Equal(t, " wololo !", PrettyJSONMessage("", "  ", []byte(` wololo !`)))
	expected := `{
  "1": "2",
  "3": "4"
}`
	assert.Equal(t, expected, PrettyJSONMessage("", "  ", []byte(`{"1": "2", "3": "4"}`)))
}
