package middleware

import (
	"context"
	"math/rand"
	"net/http"
)

type partitionRequestsCtxKey int

const (
	partitionCtxKey partitionRequestsCtxKey = 0
)

type PartitionLabel string

type SplitPartitionConfig struct {
	Enabled bool
	Percent int // percent of requests must be labeled with partition label
	Label   PartitionLabel
}

// SplitRequestsMiddleware split requests by percent and marks them with label
func SplitRequestsMiddleware(config SplitPartitionConfig) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if !config.Enabled || config.Percent == 0 {
				next.ServeHTTP(w, r)
				return
			}

			requestPercent := rand.Intn(100) // mark request my label
			if requestPercent <= config.Percent {
				ctx := r.Context()
				ctx = context.WithValue(ctx, partitionCtxKey, config.Label)
				*r = *r.WithContext(ctx)
			}
			next.ServeHTTP(w, r)
		})
	}
}

func GetRequestPartitionLabel(ctx context.Context) PartitionLabel {
	if label, ok := ctx.Value(partitionCtxKey).(PartitionLabel); ok {
		return label
	}
	return ""
}
