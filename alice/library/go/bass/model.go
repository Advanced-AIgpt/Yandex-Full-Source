package libbass

const (
	TextPushEvent          PushEvent = "text_action"
	PhrasePushEvent        PushEvent = "phrase_action"
	SemanticFramePushEvent PushEvent = "mm_semantic_frame"
)

type PushEvent string

type PushPayload struct {
	Event        PushEvent   `json:"event"`
	Service      string      `json:"service"`
	ServiceData  interface{} `json:"service_data"`
	CallbackData string      `json:"callback_data"`
}
