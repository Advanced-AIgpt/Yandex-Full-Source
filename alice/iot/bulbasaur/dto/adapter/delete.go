package adapter

type DeleteRequest struct {
	CustomData interface{} `json:"custom_data,omitempty"`
}

// TODO: migrate to delete devices handler and change model to per device ErrorCode (remove Success)
type DeleteResult struct {
	Success      bool      `json:"success"`
	RequestID    string    `json:"request_id"`
	ErrorCode    ErrorCode `json:"error_code,omitempty"`
	ErrorMessage string    `json:"error_message,omitempty"`
}

func (d *DeleteResult) GetErrors() ErrorCodeCountMap {
	errorCodes := make(ErrorCodeCountMap)

	if d.ErrorCode != "" {
		errorCodes[d.ErrorCode] += 1
	}

	return errorCodes
}
