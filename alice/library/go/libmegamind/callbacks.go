package libmegamind

import (
	"encoding/json"

	structpb "github.com/golang/protobuf/ptypes/struct"
)

type CallbackFrameAction struct {
	FrameSlug       string
	FrameName       string
	Phrases         []string
	CallbackName    CallbackName
	CallbackPayload *structpb.Struct
}

type CallbackName string

type Callback struct {
	Name    CallbackName
	Payload json.RawMessage
}
