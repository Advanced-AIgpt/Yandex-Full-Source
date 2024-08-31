package libconfita

import (
	"context"
	"path/filepath"
	"regexp"
	"strings"

	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/yav"
)

type BackendOption func([]backend.Backend) ([]backend.Backend, error)

func AddDefaultFileBackend(basePath string) BackendOption {
	return func(backends []backend.Backend) ([]backend.Backend, error) {
		defaultConfPath, err := filepath.Abs(filepath.Join(basePath, "default.yaml"))
		if err != nil {
			return nil, xerrors.Errorf("default config not found in folder %q: %w", basePath, err)
		}
		backends = append(backends, file.NewBackend(defaultConfPath))
		return backends, nil
	}
}

func AddEnvFileBackend(envType, basePath string) BackendOption {
	return func(backends []backend.Backend) ([]backend.Backend, error) {
		envType := strings.ToLower(envType)
		if ok, _ := regexp.Match("^[a-z-]+$", []byte(envType)); ok && envType != "production" {
			envTypeConfPath, err := filepath.Abs(filepath.Join(basePath, envType+".yaml"))
			if err != nil {
				return nil, xerrors.Errorf("env %s config not found in folder %q: %w", envType, basePath, err)
			}
			backends = append(backends, file.NewBackend(envTypeConfPath))
		}
		return backends, nil
	}
}

func AddYaVaultBackend(yavClient yav.Client, secretID string) BackendOption {
	return func(backends []backend.Backend) ([]backend.Backend, error) {
		yavBackend, err := NewYaVaultBackend(context.Background(), yavClient, secretID)
		if err != nil {
			return nil, xerrors.Errorf("unable to create confita yav backend: %w", err)
		}
		backends = append(backends, yavBackend)
		return backends, nil
	}
}

func AddEnvBackend() BackendOption {
	return func(backends []backend.Backend) ([]backend.Backend, error) {
		backends = append(backends, env.NewBackend())
		return backends, nil
	}
}
