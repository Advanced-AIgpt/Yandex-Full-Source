package libconfita

import (
	"context"
	"encoding/base64"

	"github.com/heetch/confita/backend"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/yav"
)

var _ backend.Backend = &YaVaultBackend{}

type YaVaultBackend struct {
	yavClient yav.Client
	cache     map[string]string
}

func (b *YaVaultBackend) Get(_ context.Context, key string) ([]byte, error) {
	value, found := b.cache[key]
	if !found {
		return nil, backend.ErrNotFound
	}
	return []byte(value), nil
}

func (b *YaVaultBackend) Name() string {
	return "yav"
}

func NewYaVaultBackend(ctx context.Context, client yav.Client, secretID string) (*YaVaultBackend, error) {
	b := &YaVaultBackend{
		yavClient: client,
		cache:     map[string]string{},
	}
	response, err := client.GetVersion(ctx, secretID)
	if err != nil {
		return nil, xerrors.Errorf("unable to get %s secret version: %w", secretID, err)
	}
	if err := response.Err(); err != nil {
		return nil, err
	}
	for _, kv := range response.Version.Values {
		key, value, encoding := kv.Key, kv.Value, kv.Encoding
		if encoding != "" {
			switch encoding {
			case "base64":
				rawValue, err := base64.StdEncoding.DecodeString(value)
				if err != nil {
					return nil, xerrors.Errorf("unable to decode base64 value of key %v of secret %v: %w", key, secretID, err)
				}
				value = string(rawValue)
			default:
				return nil, xerrors.Errorf("unknown encoding %v for key %v of secret %v", encoding, key, secretID)
			}
		}
		b.cache[key] = value
	}
	return b, nil
}
