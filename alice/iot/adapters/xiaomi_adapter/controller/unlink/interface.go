package unlink

import (
	"context"
)

type IController interface {
	Unlink(ctx context.Context, token string, userID uint64) error
}
