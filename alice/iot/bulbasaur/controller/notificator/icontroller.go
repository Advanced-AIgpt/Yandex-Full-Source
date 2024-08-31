package notificator

import (
	"context"

	matrixpb "a.yandex-team.ru/alice/protos/api/matrix"
)

type IController interface {
	// SendTypedSemanticFrame sends any TSF-convertible struct to speaker
	SendTypedSemanticFrame(ctx context.Context, userID uint64, deviceID string, frame TSF, options ...Option) error

	// SendSpeechkitDirective sends any SK directive to speaker
	SendSpeechkitDirective(ctx context.Context, userID uint64, deviceID string, directive SpeechkitDirective) error

	// IsDeviceOnline performs a quick lookup of speaker connectivity in notificator
	IsDeviceOnline(ctx context.Context, userID uint64, deviceID string) bool

	// OnlineDeviceIDs returns all currently online speaker IDs
	OnlineDeviceIDs(ctx context.Context, userID uint64) ([]string, error)
}

type Option func(delivery *matrixpb.TDelivery)

var SetTTLOption = func(ttl int) Option {
	return func(delivery *matrixpb.TDelivery) {
		delivery.Ttl = uint32(ttl)
	}
}
