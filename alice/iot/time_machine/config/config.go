package config

import (
	"context"
	"os"
	"path/filepath"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type (
	Config struct {
		TVM      TVM      `yaml:"tvm"`
		Worker   Worker   `yaml:"worker"`
		HTTP     Server   `yaml:"server"`
		PGBroker PGBroker `yaml:"pg_broker"`
	}

	TVM struct {
		Port        int    `yaml:"port"`
		ServiceName string `yaml:"service_name"`
		Token       string `yaml:"token" config:"TVM_TOKEN,backend=env"`
	}

	PGBroker struct {
		Hosts    []string `yaml:"hosts"`
		Port     int      `yaml:"port"`
		DB       string   `yaml:"db"`
		User     string   `yaml:"user" config:"POSTGRES_USER,backend=env"`
		Password string   `yaml:"password" config:"POSTGRES_PASSWORD,backend=env"`
	}

	Worker struct {
		ProcessNum    int `yaml:"process_num"`
		TaskChunkSize int `yaml:"task_chunk_size"`
	}

	Server struct {
		AllowedTvmClientIDs []int `yaml:"allowed_tvm_clients"`
	}
)

func Load(basePath string, envType string) (Config, error) {
	var backends []backend.Backend

	// 1. Default config
	if defaultConfPath, err := filepath.Abs(filepath.Join(basePath, "default.yaml")); err == nil {
		backends = append(backends, file.NewBackend(defaultConfPath))
	} else {
		return Config{}, xerrors.Errorf("default config not found in folder %q: %w", basePath, err)
	}

	// 2. EnvType config overrides default config
	if envTypeConfPath, err := filepath.Abs(filepath.Join(basePath, envType+".yaml")); err == nil {
		info, err := os.Stat(envTypeConfPath)
		if err == nil && info.IsDir() {
			return Config{}, err
		}
		if !os.IsNotExist(err) {
			backends = append(backends, file.NewBackend(envTypeConfPath))
		}
	}

	// 3. Env variables overrides EnvType-config and base config
	backends = append(backends, env.NewBackend())

	var cfg Config
	err := confita.NewLoader(backends...).Load(context.Background(), &cfg)

	return cfg, err
}
