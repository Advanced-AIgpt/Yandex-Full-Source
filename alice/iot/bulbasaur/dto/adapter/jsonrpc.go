package adapter

import "encoding/json"

type RequestType string

const (
	Unlink    RequestType = "unlink"
	Discovery RequestType = "discovery"
	Query     RequestType = "query"
	Action    RequestType = "action"
)

type JSONRPCHeaders struct {
	Authorization string `json:"authorization"`
	RequestID     string `json:"request_id"`
}

type JSONRPCRequest struct {
	Headers     JSONRPCHeaders `json:"headers"`
	RequestType RequestType    `json:"request_type"`
	Payload     interface{}    `json:"payload,omitempty"` // StatesRequest or ActionRequestPayload
}

type rawRequest struct {
	Headers     JSONRPCHeaders  `json:"headers"`
	RequestType RequestType     `json:"request_type"`
	Payload     json.RawMessage `json:"payload,omitempty"`
}

func (r *JSONRPCRequest) UnmarshalJSON(b []byte) error {
	var rRaw rawRequest

	if err := json.Unmarshal(b, &rRaw); err != nil {
		return err
	}

	r.Headers = rRaw.Headers
	r.RequestType = rRaw.RequestType

	switch rRaw.RequestType {
	case Action:
		var payload ActionRequestPayload
		if err := json.Unmarshal(rRaw.Payload, &payload); err != nil {
			return err
		}
		r.Payload = payload
	case Query:
		var payload StatesRequest
		if err := json.Unmarshal(rRaw.Payload, &payload); err != nil {
			return err
		}
		r.Payload = payload
	}
	return nil
}
