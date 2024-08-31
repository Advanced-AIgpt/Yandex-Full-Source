package config

import (
	"fmt"
	"gopkg.in/yaml.v2"
	"net/url"
	"os"
	"reflect"

	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/valid"
)

const (
	InMemoryDBKind DBKind = "inmemory"
	MongoDBKind    DBKind = "mongo_db"
	DiskDBKind     DBKind = "disk"
)

const (
	Webhook     UpdaterKind = "webhook"
	LongPolling UpdaterKind = "long_polling"
)

const (
	Stable  EnvType = "stable"
	Testing EnvType = "testing"
	Dev     EnvType = "dev"
)

type (
	Config struct {
		InternalServer Server      `yaml:"internal_server" json:"internal_server"`
		ExternalServer Server      `yaml:"external_server" json:"external_server"`
		DB             DB          `yaml:"db" json:"db"`
		Telegram       Telegram    `yaml:"telegram" json:"telegram"`
		Passport       Passport    `yaml:"passport" json:"passport"`
		Staff          Staff       `yaml:"staff" json:"staff"`
		Logger         Logger      `yaml:"logger" json:"logger"`
		Env            Env         `yaml:"env" json:"env"`
		RateLimiter    RateLimiter `yaml:"rate_limiter" json:"rate_limiter"`
	}

	DBKind      string
	UpdaterKind string
	EnvType     string

	Server struct {
		Address string `yaml:"address" json:"address" valid:"required"`
	}

	DB struct {
		Kind    DBKind  `yaml:"kind" json:"kind" valid:"required"`
		MongoDB MongoDB `yaml:"mongo_db" json:"mongo_db"`
	}

	MongoDB struct {
		Username   string `yaml:"username" env:"MONGO_DB_USERNAME" json:"username"`
		Password   string `yaml:"password" env:"MONGO_DB_PASSWORD" json:"-"`
		Hosts      string `yaml:"hosts" env:"MONGO_DB_HOSTS" json:"hosts"`
		DBName     string `yaml:"db_name" env:"MONGO_DB_NAME" json:"db_name"`
		ReplicaSet string `yaml:"replica_set" env:"MONGO_DB_REPLICA_SET" json:"replica_set"`
	}

	Telegram struct {
		Token   string  `yaml:"token" env:"TELEGRAM_TOKEN" json:"-" valid:"required"`
		Updater Updater `yaml:"updater" json:"updater"`
	}

	Updater struct {
		Kind      UpdaterKind `yaml:"kind" json:"kind" valid:"required"`
		Host      string      `yaml:"host" json:"host" valid:"required"`
		BasePath  string      `yaml:"base_path" json:"base_path" valid:"required"`
		TokenHash string      `yaml:"-" json:"-"`
		URL       *url.URL    `yaml:"-" json:"-"`
	}

	Passport struct {
		ClientID     string `yaml:"client_id" json:"client_id" env:"PASSPORT_CLIENT_ID" valid:"required"`
		ClientSecret string `yaml:"client_secret" json:"-" env:"PASSPORT_CLIENT_SECRET" valid:"required"`
	}
	Staff struct {
		Token string `yaml:"token" json:"-" env:"STAFF_TOKEN" valid:"required"`
	}
	Logger struct {
		Setrace Setrace `yaml:"setrace" json:"setrace"`
	}
	Setrace struct {
		Enabled    bool   `yaml:"enabled" json:"enabled"`
		LogsPath   string `yaml:"logs_path" json:"logs_path"`
		YTLogsPath string `yaml:"yt_logs_path" json:"yt_logs_path"`
	}
	Env struct {
		Service string  `yaml:"service" json:"service" valid:"required"`
		Type    EnvType `yaml:"type" json:"type" valid:"required"`
	}
	RateLimiter struct {
		MaxRPS uint `yaml:"max_rps" json:"max_rps"`
		MaxRPM uint `yaml:"max_rpm" json:"max_rpm"`
	}
)

func Load(filePath string) (Config, error) {
	var cfg Config
	file, err := os.Open(filePath)
	if err != nil {
		return Config{}, err
	}
	defer func() {
		_ = file.Close()
	}()
	decoder := yaml.NewDecoder(file)
	if err := decoder.Decode(&cfg); err != nil {
		return Config{}, err
	}
	if err := loadENVs(&cfg); err != nil {
		return Config{}, err
	}
	if err := valid.Struct(binder.DefaultValidationContext, cfg); err != nil {
		return Config{}, fmt.Errorf("invalid config: %+v", err)
	}
	return cfg, nil
}

func loadENVsReflect(v reflect.Value) error {
	t := v.Type()
	for i := 0; i < v.NumField(); i++ {
		fv := v.Field(i)
		ft := t.Field(i)
		envKey, ok := ft.Tag.Lookup("env")
		if !ok {
			if fv.Kind() == reflect.Struct {
				if err := loadENVsReflect(fv); err != nil {
					return err
				}
			}
			continue
		}
		if fv.Kind() != reflect.String {
			return fmt.Errorf("%s has env tag, but it's not a string", ft.Name)
		}
		if envVal, ok := os.LookupEnv(envKey); ok {
			fv.SetString(envVal)
		}
	}
	return nil
}

func loadENVs(config *Config) error {
	return loadENVsReflect(reflect.ValueOf(config).Elem())
}
