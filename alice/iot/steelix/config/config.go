package config

import (
	"context"
	"os"
	"path/filepath"
	"regexp"
	"strings"
	"time"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type (
	Config struct {
		CloudType      string `config:"CLOUD_TYPE,backend=env" yaml:"cloud_type"`
		AttachProfiler bool   `config:"WITH_PROFILER,backend=env" yaml:"attach_profiler"`
		HTTPServer     struct {
			ListenAddr      string        `yaml:"listen_addr" valid:"required"`
			ShutdownTimeout time.Duration `config:"SHUTDOWN_TIMEOUT" yaml:"shutdown_timeout"`
		} `yaml:"http_server"`
		Logging  Logging
		Solomon  Solomon
		TVM      TVM
		Blackbox Blackbox
		Dialogs  Dialogs
		Proxy    Proxy

		Logbroker Logbroker `yaml:"logbroker"`
	}

	Logging struct {
		DevMode  bool   `yaml:"dev_mode"`
		LogLevel string `config:"LOG_LEVEL,backend=env" yaml:"log_level" valid:"required"`
	}

	Solomon struct {
		Prefix      string
		Performance struct {
			Prefix          string        `valid:"required"`
			RefreshInterval time.Duration `yaml:"refresh_interval" valid:"greater=0"`
		}
		Router struct {
			Prefix string `valid:"required"`
		}
		Dialogs struct {
			Prefix string `valid:"required"`
		}
		Upstreams struct {
			Prefix  string `valid:"required"`
			TagName string `yaml:"tag_name" valid:"required"`
			Tags    struct {
				Default     string `valid:"required"`
				PaskillsB2B string `yaml:"paskills-b2b" valid:"required"`
				Iot         string `valid:"required"`
				Dialogovo   string `valid:"required"`
			}
		}
	}

	TVM struct {
		Port     uint16 `config:"TVM_PORT,backend=env" valid:"greater=0"`
		Token    string `config:"TVM_TOKEN,backend=env" valid:"required"`
		SrcAlias string `config:"TVM_CLIENT_NAME,backend=env" yaml:"src_alias" valid:"required"`
		Debug    bool   `config:"TVM_DEBUG,backend=env"`
	}

	Blackbox struct {
		TvmID uint32 `config:"BLACKBOX_TVM_ID,backend=env" yaml:"tvm_id" valid:"greater=0"`
		URL   string `config:"BLACKBOX_URL,backend=env" valid:"required"`
	}

	Dialogs struct {
		TvmAlias string `config:"DIALOGS_ALIAS,backend=env" yaml:"tvm_alias" valid:"required"`
		URL      string `config:"DIALOGS_URL,backend=env" valid:"required"`
	}

	Proxy struct {
		OAuth struct {
			ClientID    string `yaml:"client_id"`
			ClientIDB2B string `yaml:"client_id_b2b"`
		} `yaml:"oauth"`

		Upstream struct {
			Default struct {
				AuthType string `yaml:"auth_type"`
				TvmAlias string `config:"UPSTREAM_ALIAS_DEFAULT,backend=env" yaml:"tvm_alias"`
				URL      string `config:"UPSTREAM_URL_DEFAULT,backend=env"`
				Rewrites []Rewrite
			}
			PASkillsB2B struct {
				AuthType string `yaml:"auth_type"`
				TvmAlias string `config:"UPSTREAM_ALIAS_PASKILLS_B2B,backend=env" yaml:"tvm_alias"`
				URL      string `config:"UPSTREAM_URL_PASKILLS_B2B,backend=env"`
				Rewrites []Rewrite
			} `yaml:"paskills-b2b"`
			Bulbasaur struct {
				AuthType string `yaml:"auth_type"`
				TvmAlias string `config:"UPSTREAM_ALIAS_BULBASAUR,backend=env" yaml:"tvm_alias"`
				URL      string `config:"UPSTREAM_URL_BULBASAUR,backend=env"`
				Rewrites []Rewrite
			}
			IotAPI struct {
				AuthType string `yaml:"auth_type"`
				TvmAlias string `config:"UPSTREAM_ALIAS_BULBASAUR,backend=env" yaml:"tvm_alias"`
				URL      string `config:"UPSTREAM_URL_BULBASAUR,backend=env"`
				Rewrites []Rewrite
			} `yaml:"iot-api"`
			Dialogovo struct {
				AuthType string `yaml:"auth_type"`
				TvmAlias string `config:"UPSTREAM_ALIAS_DIALOGOVO,backend=env" yaml:"tvm_alias"`
				URL      string `config:"UPSTREAM_URL_DIALOGOVO,backend=env"`
				Rewrites []Rewrite
			}
		}
	}

	Rewrite struct {
		From string
		To   string
	}

	Logbroker struct {
		Enabled                bool          `config:"LOGBROKER_ENABLED" yaml:"enabled"`
		RequestsPercent        int           `config:"LOGBROKER_REQUESTS_PERCENT" yaml:"requests_percent"`
		PartitionCount         uint32        `config:"LOGBROKER_PARTITION_COUNT" yaml:"partition_count" valid:"greater=0"`
		AckTimeout             time.Duration `config:"LOGBROKER_ACK_TIMEOUT" yaml:"ack_timeout" valid:"greater=0"`
		CollectMetricsInterval time.Duration `config:"LOGBROKER_COLLECT_METRICS_INTERVAL" yaml:"collect_metrics_interval" valid:"greater=0"`
		TvmDest                uint32        `config:"LOGBROKER_TVM_DEST" yaml:"tvm_dest"`
		WriterTemplate         struct {
			Database       string        `config:"LOGBROKER_WRITER_TEMPLATE_DATABASE" yaml:"database"`
			Endpoint       string        `config:"LOGBROKER_WRITER_TEMPLATE_ENDPOINT" yaml:"endpoint"`
			Port           int           `config:"LOGBROKER_WRITER_TEMPLATE_PORT" yaml:"port"`
			MaxMemory      int           `config:"LOGBROKER_WRITER_TEMPLATE_MAX_MEMORY" yaml:"max_memory"`
			ClientTimeout  time.Duration `config:"LOGBROKER_WRITER_TEMPLATE_CLIENT_TIMEOUT" yaml:"client_timeout"`
			RetryOnFailure bool          `config:"LOGBROKER_WRITER_TEMPLATE_RETRY_ON_FAILURE" yaml:"retry_on_failure"`
			Topic          string        `config:"LOGBROKER_WRITER_TEMPLATE_TOPIC" yaml:"topic"`
		} `yaml:"writer_template"`
	}
)

func Load(basePath string) (Config, error) {
	var backends []backend.Backend

	// 1. Default config
	if defaultConfPath, err := filepath.Abs(filepath.Join(basePath, "default.yaml")); err == nil {
		backends = append(backends, file.NewBackend(defaultConfPath))
	} else {
		return Config{}, xerrors.Errorf("default config not found in folder %q: %w", basePath, err)
	}

	// 2. EnvType config overrides default config
	// NB: production.yaml will be ignored. Use default.yaml instead
	envType := strings.ToLower(os.Getenv("ENV_TYPE"))
	if ok, _ := regexp.Match("^[a-z-]+$", []byte(envType)); ok && envType != "production" {
		if envTypeConfPath, err := filepath.Abs(filepath.Join(basePath, envType+".yaml")); err == nil {
			backends = append(backends, file.NewBackend(envTypeConfPath))
		}
	}

	// 3. Env variables overrides EnvType-config and base config
	backends = append(backends, env.NewBackend())

	var cfg Config
	err := confita.NewLoader(backends...).
		Load(context.Background(), &cfg)

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
			return xerrors.New("value is required")
		} else {
			return nil
		}
	}))

	if err := valid.Struct(validationCtx, cfg); err != nil {
		return cfg, xerrors.Errorf("Config validation failed: %+v", err)
	}

	return cfg, err
}
