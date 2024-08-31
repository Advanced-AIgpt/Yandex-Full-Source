package servicehost

import (
	"context"
)

type ctxServiceKey string

func WithServiceHostURL(ctx context.Context, serviceKey string, hostURL string) context.Context {
	return context.WithValue(ctx, ctxServiceKey(serviceKey), hostURL)
}

func GetServiceHostURL(ctx context.Context, serviceKey string) (string, bool) {
	if host, ok := ctx.Value(ctxServiceKey(serviceKey)).(string); ok {
		return host, true
	}
	return "", false
}
