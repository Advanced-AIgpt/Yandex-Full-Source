package main

import (
	"context"
	"os"
	"path/filepath"
	"regexp"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"
)

type (
	Config struct {
		Logging Logging

		Yt Yt

		Database  Database
		TVM       TVM
		Dialogs   Dialogs
		Socialism Socialism
		Tuya      Tuya
		Sber      Sber
		Xiaomi    Xiaomi
	}

	Yt struct {
		Cluster string `config:"YT_CLUSTER,backend=env" yaml:"cluster" valid:"required"`
		Input   string `config:"YT_INPUT_TABLE,backend=env" yaml:"input" valid:"required"`
	}

	Logging struct {
		Level string `config:"LOG_LEVEL,backend=env" yaml:"level" valid:"required"`
	}

	Database struct {
		YdbEndpoint string `config:"YDB_ENDPOINT,backend=env" yaml:"endpoint" valid:"required"`
		YdbPrefix   string `config:"YDB_PREFIX,backend=env" yaml:"prefix" valid:"required"`
		YdbToken    string `config:"YDB_TOKEN,backend=env" yaml:"token" valid:"required"`
	}

	TVM struct {
		Port     uint16 `config:"TVM_PORT,backend=env" valid:"greater=0"`
		Token    string `config:"TVMTOOL_LOCAL_AUTHTOKEN,backend=env" valid:"required"`
		SrcAlias string `config:"TVM_CLIENT_NAME,backend=env" yaml:"src_alias" valid:"required"`
	}

	Dialogs struct {
		TvmAlias string `config:"DIALOGS_ALIAS,backend=env" yaml:"tvm_alias" valid:"required"`
		URL      string `config:"DIALOGS_URL,backend=env" valid:"required"`
	}

	Socialism struct {
		URL string `config:"SOCIALISM_URL,backend=env" valid:"required"`
	}

	Tuya struct {
		AdapterURL string `config:"TUYA_ADAPTER_URL,backend=env" yaml:"adapter_url" valid:"required"`
		TVMAlias   string `config:"TUYA_ADAPTER_TVM_ALIAS,backend=env" yaml:"tvm_alias" valid:"required"`
	}

	Sber struct {
		AdapterURL string `config:"SBER_ADAPTER_URL,backend=env" yaml:"adapter_url" valid:"required"`
		TVMAlias   string `config:"SBER_ADAPTER_TVM_ALIAS,backend=env" yaml:"tvm_alias" valid:"required"`
	}

	Xiaomi struct {
		AdapterURL string `config:"XIAOMI_ADAPTER_URL,backend=env" yaml:"adapter_url" valid:"required"`
	}
)

func Load(basePath string) (Config, error) {
	var backends []backend.Backend

	// 1. Default config
	if defaultConfPath, err := filepath.Abs(filepath.Join(basePath, "config.yaml")); err == nil {
		backends = append(backends, file.NewBackend(defaultConfPath))
	} else {
		return Config{}, xerrors.Errorf("default config not found in folder %q: %w", basePath, err)
	}

	// 2. EnvType config overrides default config
	envType := strings.ToLower(os.Getenv("ENV_TYPE"))
	if ok, _ := regexp.Match("^[a-z-]+$", []byte(envType)); ok && envType != "production" {
		if envTypeConfPath, err := filepath.Abs(filepath.Join(basePath, envType+".yaml")); err == nil {
			backends = append(backends, file.NewBackend(envTypeConfPath))
		}
	}

	// 3. Env variables overrides EnvType-config and base config
	backends = append(backends, env.NewBackend())

	var cfg Config
	err := confita.NewLoader(backends...).Load(context.Background(), &cfg)
	if err != nil {
		return cfg, err
	}

	// 4. Validate config
	validationCtx := valid.NewValidationCtx()
	validationCtx.Add("equal", valid.Equal)
	validationCtx.Add("notEq", valid.NotEqual)
	validationCtx.Add("lesser", valid.Lesser)
	validationCtx.Add("greater", valid.Greater)
	validationCtx.Add("min", valid.Min)
	validationCtx.Add("max", valid.Max)
	validationCtx.Add("required", valid.WrapValidator(func(s string) error {
		if len(s) == 0 {
			return xerrors.New("value is required. ")
		} else {
			return nil
		}
	}))

	if err := valid.Struct(validationCtx, cfg); err != nil {
		return cfg, xerrors.Errorf("Config validation failed: %+v", err)
	}

	return cfg, err
}
