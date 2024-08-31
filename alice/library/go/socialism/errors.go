package socialism

type ErrorCode string

const (
	TokenNotFound ErrorCode = "TOKEN_NOT_FOUND"
)

func EC(ec ErrorCode) *ErrorCode { return &ec }

type TokenNotFoundError struct{}

func (t *TokenNotFoundError) Error() string {
	return string(TokenNotFound)
}
