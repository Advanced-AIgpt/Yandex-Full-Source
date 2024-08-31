package adapter

type InfraredHubRemotesResponse struct {
	RequestID    string    `json:"request_id"`
	Remotes      []string  // infrared remotes ids
	ErrorCode    ErrorCode `json:"error_code,omitempty"`
	ErrorMessage string    `json:"error_message,omitempty"`
}

func (i *InfraredHubRemotesResponse) GetErrors() ErrorCodeCountMap {
	errorCodes := make(ErrorCodeCountMap)

	if i.ErrorCode != "" {
		errorCodes[i.ErrorCode] += 1
	}

	return errorCodes
}
