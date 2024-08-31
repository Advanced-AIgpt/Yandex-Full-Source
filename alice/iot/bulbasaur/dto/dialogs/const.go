package dialogs

type EndpointStatus string

const (
	StatusBadRequest      EndpointStatus = "BAD_REQUEST"
	StatusValidationError EndpointStatus = "VALIDATION_ERROR"
)

// new endpoint validation statuses
const (
	StatusOK                    EndpointStatus = "OK"
	StatusTimeout               EndpointStatus = "TIMEOUT"
	StatusNotFound              EndpointStatus = "NOT_FOUND"
	StatusProviderInternalError EndpointStatus = "PROVIDER_INTERNAL_ERROR"
	StatusSSLCheckFailed        EndpointStatus = "SSL_CHECK_FAILED"
)
