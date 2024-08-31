package dialogs

type EndpointValidationRequest struct {
	EndpointURL string `json:"endpoint_url" valid:"required"`
}

type EndpointValidationResponse struct {
	Status    EndpointStatus    `json:"status"`
	RequestID string            `json:"request_id"`
	Result    map[string]string `json:"result,omitempty"`
}

// TODO: Delete old format request and response

type EndpointRouteValidationResponse struct {
	Status         EndpointStatus `json:"status"`
	PathURL        string         `json:"url"`
	HTTPStatusCode int            `json:"http_code"`
	HTTPMethod     string         `json:"http_method"`
}

type EndpointValidationRequestNew struct {
	EndpointURL string `json:"endpoint_url" valid:"required"`
	Public      bool   `json:"public"`
	Trusted     bool   `json:"trusted"`
}

type EndpointValidationResponseNew struct {
	Status        string                          `json:"status"`
	RequestID     string                          `json:"request_id"`
	Availability  EndpointRouteValidationResponse `json:"availability"`
	UserUnlink    EndpointRouteValidationResponse `json:"user_unlink"`
	Devices       EndpointRouteValidationResponse `json:"devices"`
	DevicesQuery  EndpointRouteValidationResponse `json:"devices_query"`
	DevicesAction EndpointRouteValidationResponse `json:"devices_action"`
}
