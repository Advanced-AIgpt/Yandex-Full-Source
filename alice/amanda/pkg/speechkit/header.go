package speechkit

type Header struct {
	RequestID      string  `json:"request_id"`
	PrevReqID      string  `json:"prev_req_id,omitempty"`
	SequenceNumber *uint32 `json:"sequence_number,omitempty"`
	DialogID       string  `json:"dialog_id,omitempty"`
}
