package dialoglib

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestCompileAndExecute(t *testing.T) {
	args := map[string]interface{}{
		"TextNumber": 42,
		"TtsNumber":  70,
		"Name":       "Corvo Attano",
	}

	cues := []CueTemplate{
		{
			Text:         "Число в тексте {{.TextNumber}}",
			Voice:        "Число в голосе {{.TtsNumber}}",
			templateName: "testTemplate.1",
		},
		{
			Text:         "Имя {{.Name}}",
			templateName: "testTemplate.2",
		},
	}
	expectedCues := []*Cue{
		{
			Text:  "Число в тексте 42",
			Voice: "Число в голосе 70",
		},
		{
			Text: "Имя Corvo Attano",
		},
	}

	for i := range cues {
		assert.NoError(t, cues[i].Compile())
		resultCue, err := cues[i].Execute(args)
		assert.NoError(t, err)
		assert.Equal(t, expectedCues[i], resultCue)
	}
}
