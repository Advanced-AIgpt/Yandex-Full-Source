package config

import (
	"context"
	"testing"

	"github.com/heetch/confita/backend"
	"github.com/stretchr/testify/assert"

	libconfita "a.yandex-team.ru/alice/library/go/confita"
)

func NewSecretValuesMockBackend() backend.Backend {
	return backend.Func("mock", func(_ context.Context, key string) ([]byte, error) {
		secretValuesMock := map[string]string{
			"CRYPTER_KEY":                     "<mock-value>",
			"CSRF_TOKEN_KEY":                  "<mock-value>",
			"PUSH_CLIENT_TVM_SECRET":          "<mock-value>",
			"SUP_TOKEN":                       "<mock-value>",
			"TVM_SECRET":                      "<mock-value>",
			"TVM_TOKEN":                       "<mock-value>",
			"WIDGET_CLIENT_ID":                "<mock-value>",
			"XIVA_SEND_TOKEN":                 "<mock-value>",
			"XIVA_SUBSCRIBE_TOKEN":            "<mock-value>",
			"YDB_TOKEN":                       "<mock-value>",
			"YANDEX_IO_X_TOKEN_CLIENT_ID":     "<mock-value",
			"YANDEX_IO_X_TOKEN_CLIENT_SECRET": "<mock-value",
			"YANDEX_IO_OAUTH_CLIENT_ID":       "<mock-value",
			"YANDEX_IO_OAUTH_CLIENT_SECRET":   "<mock-value",
		}

		if val, ok := secretValuesMock[key]; ok {
			return []byte(val), nil
		}
		return nil, backend.ErrNotFound
	})
}

func AddSecretValuesMockBackend() libconfita.BackendOption {
	return func(backends []backend.Backend) ([]backend.Backend, error) {
		backends = append(backends, NewSecretValuesMockBackend())
		return backends, nil
	}
}

func TestDefaultConfig(t *testing.T) {
	options := []libconfita.BackendOption{
		AddSecretValuesMockBackend(),
		libconfita.AddDefaultFileBackend("./"),
	}
	_, err := Load(context.Background(), options...)
	assert.NoError(t, err)
}
