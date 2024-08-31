package requestid

import (
	"context"

	"github.com/gofrs/uuid"
)

type ctxKeyRequestID int

const requestIDKey ctxKeyRequestID = 0

func New() string {
	id, err := uuid.NewV4()
	if err != nil {
		return ""
	}
	return id.String()
}

func WithRequestID(ctx context.Context, requestID string) context.Context {
	return context.WithValue(ctx, requestIDKey, requestID)
}

func GetRequestID(ctx context.Context) string {
	if ctx == nil {
		return ""
	}
	if reqID, ok := ctx.Value(requestIDKey).(string); ok {
		return reqID
	}
	return ""
}
