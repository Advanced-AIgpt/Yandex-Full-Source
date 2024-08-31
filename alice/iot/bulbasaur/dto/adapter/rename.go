package adapter

type RenameRequest struct {
	CustomData interface{} `json:"custom_data"`
	Name       string      `json:"name"`
}

type RenameResult struct {
	Success      bool      `json:"success"`
	RequestID    string    `json:"request_id"`
	ErrorCode    ErrorCode `json:"error_code,omitempty"`
	ErrorMessage string    `json:"error_message,omitempty"`
}

func (r *RenameResult) GetErrors() ErrorCodeCountMap {
	errorCodes := make(ErrorCodeCountMap)

	if r.ErrorCode != "" {
		errorCodes[r.ErrorCode] += 1
	}

	return errorCodes
}
