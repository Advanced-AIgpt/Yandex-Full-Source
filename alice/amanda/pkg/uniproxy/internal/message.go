package internal

type Message struct {
	Event         *Event         `json:"event,omitempty"`
	StreamControl *StreamControl `json:"streamcontrol,omitempty"`
	Directive     *Event         `json:"directive,omitempty"`
}

type Event struct {
	Header  EventHeader `json:"header"`
	Payload interface{} `json:"payload"`
}

type Namespace string

type EventHeader struct {
	Namespace    Namespace `json:"namespace"`
	Name         string    `json:"name"`
	MessageID    string    `json:"messageId"`
	StreamID     *uint32   `json:"streamId,omitempty"`
	RefMessageID *string   `json:"refMessageId,omitempty"`
}

type Action int32
type Reason int32

const (
	CloseStream Action = 0
	ChunkStream Action = 1

	// See: https://tools.ietf.org/html/rfc7540#section-7
	NoError Reason = 0
)

type StreamControl struct {
	MessageID string `json:"messageId"`
	Action    Action `json:"action"`
	Reason    Reason `json:"reason"`
	StreamID  uint32 `json:"streamId"`
}
