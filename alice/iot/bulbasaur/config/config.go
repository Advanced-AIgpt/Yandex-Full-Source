package config

import (
	"context"
	"time"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"

	"a.yandex-team.ru/alice/library/go/binder"
	libconfita "a.yandex-team.ru/alice/library/go/confita"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type YdbAuthType string

var (
	TVMYdbAuthType   YdbAuthType = "tvm"
	OAuthYdbAuthType YdbAuthType = "oauth"
)

type SolomonSender string

var (
	PushSolomonSender         SolomonSender = "push"  // send directly to solomon api
	BatchSolomonSender        SolomonSender = "batch" // batch before pushing to solomon
	UnifiedAgentSolomonSender SolomonSender = "unified-agent"
)

type (
	Config struct {
		CloudType         string `config:"CLOUD_TYPE" yaml:"cloud_type"`
		ListenAddr        string `yaml:"listen_addr" valid:"required"`
		ApphostListenAddr string `yaml:"apphost_listen_addr" valid:"required"`
		AttachProfiler    bool   `config:"WITH_PROFILER" yaml:"attach_profiler"`

		TVM                TVM                `yaml:"tvm"`
		Blackbox           Blackbox           `yaml:"blackbox"`
		YDB                YDB                `yaml:"ydb"`
		Repository         Repository         `yaml:"repository"`
		Dialogs            Dialogs            `yaml:"dialogs"`
		Socialism          Socialism          `yaml:"socialism"`
		Bass               Bass               `yaml:"bass"`
		Begemot            Begemot            `yaml:"begemot"` // todo: fix configs
		Xiva               Xiva               `yaml:"xiva"`
		Sup                Sup                `yaml:"sup"`
		Timemachine        Timemachine        `yaml:"timemachine"`
		Megamind           Megamind           `yaml:"megamind"`
		Takeout            Takeout            `yaml:"takeout"`
		Steelix            Steelix            `yaml:"steelix"`
		ActionController   ActionController   `yaml:"action_controller"`
		HistoryController  HistoryController  `yaml:"history_controller"`
		ScenarioController ScenarioController `yaml:"scenario_controller"`

		Notificator Notificator `yaml:"notificator"`
		Quasar      Quasar      `yaml:"quasar"`
		Oauth       OAuth       `yaml:"oauth"`

		Adapters struct {
			QuasarAdapter  QuasarAdapter  `yaml:"quasar"`
			TuyaAdapter    TuyaAdapter    `yaml:"tuya"`
			SberAdapter    SberAdapter    `yaml:"sber"`
			PhilipsAdapter PhilipsAdapter `yaml:"philips"`
			XiaomiAdapter  XiaomiAdapter  `yaml:"xiaomi"`
			CloudFunctions CloudFunctions `yaml:"cloud_functions"`
		} `yaml:"adapters"`

		CSRF           CSRF           `yaml:"csrf"`
		IotApp         IotApp         `yaml:"iot_app"`
		Crypter        Crypter        `yaml:"crypter"`
		Setrace        Setrace        `yaml:"setrace"`
		Memento        Memento        `yaml:"memento"`
		Geosuggest     Geosuggest     `yaml:"geosuggest"`
		Datasync       Datasync       `yaml:"datasync"`
		StressHandlers StressHandlers `yaml:"stress_handlers"`
		UserService    UserService    `yaml:"user_service"`
	}

	IotApp struct {
		ClientIDs []string `config:"IOT_APP_CLIENT_IDS" yaml:"client_ids"`
	}

	TVM struct {
		Port     uint16 `config:"TVM_PORT" yaml:"port" valid:"greater=0"`
		Token    string `config:"TVM_TOKEN" yaml:"token" valid:"required"`
		SrcAlias string `config:"TVM_CLIENT_NAME" yaml:"src_alias" valid:"required"`
		Debug    bool   `config:"TVM_DEBUG"`
	}

	Blackbox struct {
		TvmID uint32 `config:"BLACKBOX_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
		URL   string `config:"BLACKBOX_URL" valid:"required"`
	}

	YDB struct {
		Prefix          string      `config:"YDB_PREFIX" yaml:"prefix" valid:"required"`
		Endpoint        string      `config:"YDB_ENDPOINT" yaml:"endpoint" valid:"required"`
		PreferLocalDB   bool        `config:"YDB_PREFER_LOCAL_DC" yaml:"prefer_local_dc"`
		BalancingMethod string      `config:"YDB_CLIENT_BALANCING_METHOD" yaml:"client_balancing_method"`
		MinSessionCount uint32      `config:"YDB_SESSION_COUNT" yaml:"min_session_count"`
		MaxSessionCount uint32      `config:"YDB_SESSION_COUNT" yaml:"max_session_count"`
		AuthType        YdbAuthType `config:"YDB_AUTH_TYPE" yaml:"auth_type" valid:"required"`
		Token           string      `config:"YDB_TOKEN" yaml:"token"`
		Debug           bool        `config:"YDB_DEBUG"`
	}

	Repository struct {
		IgnoreCache bool `config:"REPOSITORY_IGNORE_CACHE" yaml:"ignore_cache"`
	}

	CSRF struct {
		TokenKey string `config:"CSRF_TOKEN_KEY" valid:"required"`
	}

	Dialogs struct {
		TVMAlias string `config:"DIALOGS_TVM_ALIAS" yaml:"tvm_alias" valid:"required"`
		URL      string `config:"DIALOGS_URL" yaml:"url" valid:"required"`
	}

	Socialism struct {
		URL string `config:"SOCIALISM_URL" yaml:"url" valid:"required"`
	}

	Bass struct {
		URL string `config:"BASS_URL" yaml:"url" valid:"required"`
	}

	Begemot struct {
		URL string `config:"BEGEMOT_URL" yaml:"url" valid:"required"`
	}

	QuasarAdapter struct {
		Endpoint string `config:"QUASAR_ADAPTER_URL" yaml:"endpoint" valid:"required"`
		TVMAlias string `config:"QUASAR_ADAPTER_TVM_ALIAS" yaml:"tvm_alias" valid:"required"`
	}

	TuyaAdapter struct {
		Endpoint string `config:"TUYA_ADAPTER_URL" yaml:"endpoint" valid:"required"`
		TVMAlias string `config:"TUYA_ADAPTER_TVM_ALIAS" yaml:"tvm_alias" valid:"required"`
	}

	SberAdapter struct {
		Endpoint string `config:"SBER_ADAPTER_URL" yaml:"endpoint" valid:"required"`
		TVMAlias string `config:"SBER_ADAPTER_TVM_ALIAS" yaml:"tvm_alias" valid:"required"`
	}

	PhilipsAdapter struct {
		Endpoint string `config:"PHILIPS_ADAPTER_URL" yaml:"endpoint" valid:"required"`
	}

	XiaomiAdapter struct {
		Endpoint string `config:"XIAOMI_ADAPTER_URL" yaml:"endpoint" valid:"required"`
	}

	CloudFunctions struct {
		TVMAlias string `config:"CLOUD_FUNCTIONS_TVM_ALIAS" yaml:"tvm_alias" valid:"required"`
	}

	Crypter struct {
		Key string `config:"CRYPTER_KEY" valid:"required"`
	}

	Xiva struct {
		URL                string `config:"XIVA_URL" yaml:"url" valid:"required"`
		SendToken          string `config:"XIVA_SEND_TOKEN" valid:"required"`
		SubscribeToken     string `config:"XIVA_SUBSCRIBE_TOKEN" valid:"required"`
		MaxIdleConnections int    `config:"XIVA_MAX_IDLE_CONNECTIONS" yaml:"max_idle_connections" valid:"greater=0"`
	}

	Sup struct {
		URL          string `config:"SUP_URL" yaml:"url" valid:"required"`
		Token        string `config:"SUP_TOKEN" valid:"required"`
		BulbasaurURL string `config:"SUP_BULBASAUR_SRCRWR_URL" yaml:"bulbasaur_srcrwr_url"`
	}

	Timemachine struct {
		URL   string `config:"TIME_MACHINE_URL" yaml:"url" valid:"required"`
		TVMID uint32 `config:"TIME_MACHINE_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	Megamind struct {
		Bulbasaur struct {
			URL         string `config:"BULBASAUR_URL" yaml:"url" valid:"required"`
			TVMClientID uint32 `config:"TVM_CLIENT_ID" yaml:"tvm_client_id" valid:"greater=0"`
		}
		Vulpix Vulpix
	}

	Vulpix struct {
		YDB struct {
			Prefix          string      `config:"VULPIX_YDB_PREFIX" yaml:"prefix" valid:"required"`
			Endpoint        string      `config:"VULPIX_YDB_ENDPOINT" yaml:"endpoint" valid:"required"`
			MinSessionCount uint32      `config:"VULPIX_YDB_SESSION_COUNT" yaml:"min_session_count"`
			MaxSessionCount uint32      `config:"VULPIX_YDB_SESSION_COUNT" yaml:"max_session_count"`
			PreferLocalDB   bool        `config:"VULPIX_YDB_PREFER_LOCAL_DC" yaml:"prefer_local_dc"`
			BalancingMethod string      `config:"VULPIX_YDB_CLIENT_BALANCING_METHOD" yaml:"client_balancing_method"`
			AuthType        YdbAuthType `config:"VULPIX_YDB_AUTH_TYPE" yaml:"auth_type" valid:"required"`
			Token           string      `config:"VULPIX_YDB_TOKEN" yaml:"token"`
			Debug           bool        `config:"VULPIX_YDB_DEBUG"`
		} `yaml:"ydb"`
		Steelix struct {
			URL   string `config:"VULPIX_STEELIX_URL" yaml:"url" valid:"required"`
			TVMID uint32 `config:"VULPIX_STEELIX_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
		} `yaml:"steelix"`
		Tuya struct {
			URL   string `config:"VULPIX_TUYA_URL" yaml:"url" valid:"required"`
			TVMID uint32 `config:"VULPIX_TUYA_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
		} `yaml:"tuya"`
		Notificator struct {
			URL   string `config:"VULPIX_NOTIFICATOR_URL" yaml:"url" valid:"required"`
			TVMID uint32 `config:"VULPIX_NOTIFICATOR_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
		} `yaml:"notificator"`
	}

	ActionController struct {
		RetryPolicy struct {
			Type       string `config:"ACTION_RETRY_POLICY_TYPE" yaml:"type" valid:"required"`
			LatencyMs  uint64 `config:"ACTION_RETRY_POLICY_LATENCY_MS" yaml:"latency_ms"`
			RetryCount int    `config:"ACTION_RETRY_POLICY_RETRY_COUNT" yaml:"retry_count"`
		} `yaml:"retry_policy"`
	}

	HistoryController struct {
		YDB struct {
			Prefix          string      `config:"HISTORY_YDB_PREFIX" yaml:"prefix" valid:"required"`
			Endpoint        string      `config:"HISTORY_YDB_ENDPOINT" yaml:"endpoint" valid:"required"`
			PreferLocalDB   bool        `config:"HISTORY_YDB_PREFER_LOCAL_DC" yaml:"prefer_local_dc"`
			BalancingMethod string      `config:"HISTORY_YDB_CLIENT_BALANCING_METHOD" yaml:"client_balancing_method"`
			MinSessionCount uint32      `config:"HISTORY_YDB_SESSION_COUNT" yaml:"min_session_count"`
			MaxSessionCount uint32      `config:"HISTORY_YDB_SESSION_COUNT" yaml:"max_session_count"`
			AuthType        YdbAuthType `config:"HISTORY_YDB_AUTH_TYPE" yaml:"auth_type" valid:"required"`
			Token           string      `config:"HISTORY_YDB_TOKEN" yaml:"token"`
			Debug           bool        `config:"HISTORY_YDB_DEBUG" yaml:"debug"`
		} `yaml:"ydb"`
		Solomon struct {
			Project       string        `config:"HISTORY_SOLOMON_PROJECT" yaml:"project" valid:"required"`
			ServicePrefix string        `config:"HISTORY_SOLOMON_SERVICE_PREFIX" yaml:"service_prefix" valid:"required"`
			Cluster       string        `config:"HISTORY_SOLOMON_CLUSTER" yaml:"cluster" valid:"required"`
			Token         string        `config:"HISTORY_SOLOMON_OAUTH_TOKEN" yaml:"token"`
			SenderType    SolomonSender `config:"HISTORY_SOLOMON_SENDER_TYPE" yaml:"sender_type" valid:"required"`
			Batch         struct {
				Limit           uint          `config:"HISTORY_SOLOMON_BATCH_LIMIT" yaml:"limit" valid:"greater=0"`
				SendInterval    time.Duration `config:"HISTORY_SOLOMON_BATCH_SEND_INTERVAL" yaml:"send_interval"`
				Buffer          uint          `config:"HISTORY_SOLOMON_BATCH_BUFFER" yaml:"buffer"`
				CallbackTimeout time.Duration `config:"HISTORY_SOLOMON_BATCH_CALLBACK_TIMEOUT" yaml:"callback_timeout"`
				ShutdownTimeout time.Duration `config:"HISTORY_SOLOMON_BATCH_SHUTDOWN_TIMEOUT" yaml:"shutdown_timeout"`
			} `yaml:"batch"`
			UnifiedAgent struct {
				BaseURL string `config:"HISTORY_SOLOMON_UNIFIED_AGENT_BASE_URL" yaml:"base_url" valid:"required"`
			} `yaml:"unified_agent"`
		} `yaml:"solomon"`
	}
	ScenarioController struct {
		Jitter struct { // jitter adds some lag to timestamp for smoothing out peak load
			Enabled     bool          `config:"SCENARIO_JITTER_ENABLED" yaml:"enabled"`
			LeftBorder  time.Duration `config:"SCENARIO_JITTER_LEFT_BORDER" yaml:"left_border"`   // left jitter border (including)
			RightBorder time.Duration `config:"SCENARIO_JITTER_RIGHT_BORDER" yaml:"right_border"` // right jitter border (excluding)
		} `yaml:"jitter"`
	}

	Takeout struct {
		TVMID uint32 `config:"TAKEOUT_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	Steelix struct {
		TVMID uint32 `config:"STEELIX_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	Setrace struct {
		Filepath string `config:"SETRACE_FILEPATH" yaml:"filepath"`
		Enabled  bool   `config:"SETRACE_ENABLED" yaml:"enabled"`
	}

	WidgetScenarios struct {
		ClientID string `config:"WIDGET_SCENARIOS_CLIENT_ID" yaml:"client_id" valid:"required"`
	}

	WidgetCallableSpeakers struct {
		ClientID string `config:"WIDGET_CALLABLE_SPEAKERS_CLIENT_ID" yaml:"client_id" valid:"required"`
	}

	Memento struct {
		URL   string `config:"MEMENTO_URL" yaml:"url" valid:"required"`
		TVMID uint32 `config:"MEMENTO_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	Geosuggest struct {
		URL      string `config:"GEOSUGGEST_URL" yaml:"url" valid:"required"`
		ClientID string `config:"GEOSUGGEST_CLIENT_ID" yaml:"client_id" valid:"required"`
	}

	Datasync struct {
		URL   string `config:"DATASYNC_URL" yaml:"url" valid:"required"`
		TVMID uint32 `config:"DATASYNC_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	Notificator struct {
		URL   string `config:"BULBASAUR_NOTIFICATOR_URL" yaml:"url" valid:"required"`
		TVMID uint32 `config:"BULBASAUR_NOTIFICATOR_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	Quasar struct {
		URL   string `config:"QUASAR_URL" yaml:"url" valid:"required"`
		TVMID uint32 `config:"QUASAR_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
	}

	OAuth struct {
		URL                 string `config:"OAUTH_URL" yaml:"url" valid:"required"`
		TVMID               uint32 `config:"OAUTH_TVM_ID" yaml:"tvm_id" valid:"greater=0"`
		Consumer            string `config:"OAUTH_CONSUMER" yaml:"consumer" valid:"required"`
		YandexIOXTokenCreds struct {
			ClientID     string `config:"YANDEX_IO_X_TOKEN_CLIENT_ID" yaml:"client_id"`
			ClientSecret string `config:"YANDEX_IO_X_TOKEN_CLIENT_SECRET" yaml:"client_secret"`
		} `yaml:"yandex_io_x_token_creds"`
		YandexIOAuthCreds struct {
			ClientID     string `config:"YANDEX_IO_OAUTH_CLIENT_ID" yaml:"client_id"`
			ClientSecret string `config:"YANDEX_IO_OAUTH_CLIENT_SECRET" yaml:"client_secret"`
		} `yaml:"yandex_io_oauth_creds"`
	}

	StressHandlers struct {
		Enable bool `config:"STRESS_HANDLERS_ENABLE" yaml:"enable"`
	}

	UserService struct {
		ReqIDCache struct {
			MaxSize int64 `config:"REQID_CACHE_MAX_SIZE" yaml:"max_size"`
		} `yaml:"reqid_cache"`
	}
)

func Load(ctx context.Context, options ...libconfita.BackendOption) (Config, error) {
	var (
		backends []backend.Backend
		err      error
	)
	for _, o := range options {
		backends, err = o(backends)
		if err != nil {
			return Config{}, err
		}
	}

	var cfg Config
	err = confita.NewLoader(backends...).Load(ctx, &cfg)
	if err != nil {
		return Config{}, err
	}

	if err := valid.Struct(binder.DefaultValidationContext, cfg); err != nil {
		return cfg, xerrors.Errorf("config validation failed: %+v", err)
	}

	return cfg, err
}
