package config

import (
	"encoding/json"
	"fmt"
	"os"
	"reflect"

	"go.uber.org/zap"
	"gopkg.in/yaml.v2"
)

type Environment string
type ServerType string

const (
	LongPoll ServerType = "longpoll"
	Webhook  ServerType = "webhook"
)

type Config struct {
	Mongo    Mongo       `yaml:"mongo" json:"-"`
	Staff    Staff       `yaml:"staff" json:"-"`
	Telegram Telegram    `yaml:"telegram" json:"-"`
	Sentry   Sentry      `yaml:"sentry" json:"-"`
	Passport Passport    `yaml:"passport" json:"-"`
	Zora     Zora        `yaml:"zora" json:"-"`
	S3       S3          `yaml:"s3" json:"-"`
	Zap      *zap.Config `yaml:"zap" json:"-"`
	Server   Server      `yaml:"server" json:"server"`
	Surface  *Surface    `yaml:"surface" json:"-"`
	Auth     Auth        `yaml:"auth" json:"-"`
}

// String returns json string representation of config without sensitive data
func (c Config) String() string {
	data, _ := json.Marshal(c)
	return string(data)
}

func (c *Config) Validate() error {
	// TODO: validate
	return nil
}

type Server struct {
	Environment    Environment `yaml:"environment" json:"environment"`
	Port           int         `yaml:"port" json:"port"`
	SolomonPort    int         `yaml:"solomon_port" json:"solomon_port"`
	Type           ServerType  `yaml:"type" json:"type"`
	FQDN           string      `yaml:"fqdn" json:"fqdn"`
	MaxConnections int         `yaml:"max_connections" json:"max_connections"`
}

type S3 struct {
	AccessKey string `yaml:"access_key" env:"S3_ACCESS_KEY"`
	SecretKey string `yaml:"secret_key" env:"S3_SECRET_KEY"`
}

type Zora struct {
	Source   string `yaml:"source" env:"ZORA_SOURCE"`
	TVM      string `yaml:"tvm" env:"ZORA_TVM"`
	TVMAlias string `yaml:"tvm_alias" env:"TVM_ALIAS"`
}

type Passport struct {
	ClientID     string `yaml:"client_id" env:"PASSPORT_CLIENT_ID"`
	ClientSecret string `yaml:"client_secret" env:"PASSPORT_CLIENT_SECRET"`
}

type Mongo struct {
	Username   string `yaml:"username" env:"MONGO_USERNAME"`
	Password   string `yaml:"password" env:"MONGO_PASSWORD"`
	Hosts      string `yaml:"hosts" env:"MONGO_HOSTS"`
	DBName     string `yaml:"db_name" env:"MONGO_DB_NAME"`
	ReplicaSet string `yaml:"replica_set" env:"MONGO_REPLICA_SET"`
}

type Staff struct {
	OAuthToken string `yaml:"oauth_token" env:"STAFF_OAUTH_TOKEN"`
}

type Telegram struct {
	Token string `yaml:"token" env:"TELEGRAM_TOKEN"`
}

type Sentry struct {
	DSN string `yaml:"dsn" env:"SENTRY_DSN"`
}

type Surface struct {
	AppID             *string  `yaml:"app_id"`
	UniproxyURL       *string  `yaml:"uniproxy_url"`
	VinsURL           *string  `yaml:"vins_url"`
	Language          *string  `yaml:"language"`
	Experiments       []string `yaml:"experiments"`
	QueryParams       []string `yaml:"query_params"`
	SupportedFeatures []string `yaml:"supported_features"`
	VoiceSession      *bool    `yaml:"voice_session"`
}

type Auth struct {
	AllowNonYandexoid bool `yaml:"allow_non_yandexoid"`
}

func Load(filePath string) (*Config, error) {
	config := new(Config)
	file, err := os.Open(filePath)
	if err != nil {
		return nil, err
	}
	defer func() {
		_ = file.Close()
	}()
	decoder := yaml.NewDecoder(file)
	if err := decoder.Decode(config); err != nil {
		return nil, err
	}
	if err := loadENVs(config); err != nil {
		return nil, err
	}
	return config, nil
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
