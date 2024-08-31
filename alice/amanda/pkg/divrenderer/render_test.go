package divrenderer

import (
	"encoding/json"
	"testing"
)

var cardsTests = []struct {
	name     string
	card     *Card
	expected string
}{
	{
		name:     "TextCard",
		card:     NewTextCard("it's a text on the card"),
		expected: `{"text":"it's a text on the card","type":"simple_text"}`,
	},
	{
		name:     "TextWithButtonCard_EmptyButtons",
		card:     NewTextWithButtonsCard("it's a text on the card"),
		expected: `{"buttons":[],"text":"it's a text on the card","type":"text_with_button"}`,
	},
	{
		name:     "TextWithButtonCard",
		card:     NewTextWithButtonsCard("it's a text on the card", map[string]interface{}{"type": "action"}),
		expected: `{"buttons":[{"type":"action"}],"text":"it's a text on the card","type":"text_with_button"}`,
	},
	{
		name:     "DivCard",
		card:     NewDivCard(map[string]interface{}{"type": "action"}),
		expected: `{"body":{"type":"action"},"text":"...","type":"div_card"}`,
	},
}

func prepJSON(raw string) string {
	data := &map[string]interface{}{}
	_ = json.Unmarshal([]byte(raw), data)
	binary, _ := json.Marshal(data)
	return string(binary)
}

func TestCards(t *testing.T) {
	for _, tt := range cardsTests {
		t.Run(tt.name, func(t *testing.T) {
			act, err := json.Marshal(tt.card.content)
			if err != nil {
				t.Error("unable to marshal card", err)
			}
			expected := prepJSON(tt.expected)
			actual := string(act)
			if expected != actual {
				t.Errorf("%s != %s", actual, expected)
			}
		})
	}
}
