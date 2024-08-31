package divrenderer

// Card is Div2Html card object which contains SpeechKit card
type Card struct {
	content map[string]interface{}
}

// Dialog is Div2Html message container
type Dialog struct {
	messages []map[string]interface{}
}

// NewTextCard creates new SpeechKit's simple_text object with provided text
func NewTextCard(text string) *Card {
	return &Card{content: map[string]interface{}{
		"type": "simple_text",
		"text": text,
	}}
}

// NewTextWithButtonsCard creates new SpeechKit's text_with_button object with provided text and buttons (in json representation)
func NewTextWithButtonsCard(text string, buttons ...map[string]interface{}) *Card {
	if buttons == nil {
		buttons = make([]map[string]interface{}, 0)
	}
	return &Card{content: map[string]interface{}{
		"type":    "text_with_button",
		"text":    text,
		"buttons": buttons,
	}}
}

// NewDivCard creates new Card with SpeechKit's div_card object with provided body (in json representation)
func NewDivCard(body map[string]interface{}) *Card {
	return &Card{content: map[string]interface{}{
		"type": "div_card",
		"text": "...",
		"body": body,
	}}
}

// NewDialog creates new Dialog in Div2Html format
func NewDialog() *Dialog {
	return &Dialog{}
}

// AddUserMessage adds user message to the dialog
func (dialog *Dialog) AddUserMessage(text string) *Dialog {
	return dialog.addMessage("user", struct {
		Text string `json:"text"`
	}{
		Text: text,
	})
}

// AddAliceMessage adds Alice message to the dialog
func (dialog *Dialog) AddAliceMessage(cards ...*Card) *Dialog {
	message := make([]map[string]interface{}, 0, len(cards))
	for _, card := range cards {
		message = append(message, card.content)
	}
	return dialog.addMessage("assistant", struct {
		Cards []map[string]interface{} `json:"cards"`
	}{
		Cards: message,
	})
}

func (dialog *Dialog) addMessage(from string, msg interface{}) *Dialog {
	dialog.messages = append(dialog.messages, map[string]interface{}{
		"message": msg,
		"type":    from,
	})
	return dialog
}
