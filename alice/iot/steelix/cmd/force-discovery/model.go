package main

import (
	"context"
	"fmt"
	"path/filepath"
	"sync"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/gofrs/uuid"
	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/env"
	"github.com/heetch/confita/backend/file"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	steelix "a.yandex-team.ru/alice/iot/steelix/client"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

type SteelixAuthType string

var (
	TVMSteelixAuth   SteelixAuthType = "tvm"
	OAuthSteelixAuth SteelixAuthType = "oauth"
)

type ForceDiscoveryClient struct {
	tvm             tvm.Client
	steelixClient   steelix.Client
	config          ForceDiscoveryConfig
	usersDataSource IDataSource
}

func (fdc *ForceDiscoveryClient) Init(logger *zap.Logger) {
	if fdc.config.SteelixAuthType == TVMSteelixAuth {
		fdc.InitTvmClient()
	}
	fdc.InitSteelixClient(logger)
}

func (fdc *ForceDiscoveryClient) InitTvmClient() {
	tvmPort := fdc.config.TVMConfig.Port
	token := fdc.config.TVMConfig.Token
	clientName := fdc.config.TVMConfig.ClientName

	var err error
	if fdc.tvm, err = tvmtool.NewClient(
		fmt.Sprintf("http://localhost:%d", tvmPort),
		tvmtool.WithAuthToken(token),
		tvmtool.WithSrc(clientName),
	); err != nil {
		panic(fmt.Sprintf("TVM client init failed: %s", err))
	}
}

func (fdc *ForceDiscoveryClient) InitSteelixClient(logger log.Logger) {
	client := steelix.Client{
		Endpoint: fdc.config.SteelixURL,
		Client:   resty.New(),
		Logger:   logger,
	}
	switch fdc.config.SteelixAuthType {
	case TVMSteelixAuth:
		client.AuthPolicy = &authpolicy.TVMWithClientServicePolicy{
			DstID:  fdc.config.SteelixTVMID,
			Client: fdc.tvm,
		}
	case OAuthSteelixAuth:
		client.AuthPolicy = &authpolicy.OAuthPolicy{
			Prefix: authpolicy.OAuthHeaderPrefix,
			Token:  fdc.config.OAuthConfig.Token,
		}
	default:
		panic(fmt.Sprintf("unknown steelix auth type: %s", fdc.config.SteelixAuthType))
	}
	fdc.steelixClient = client
}

func (fdc *ForceDiscoveryClient) ForceDiscoveryPerUser(ctx context.Context, userID string) error {
	var discoveryRequest callback.DiscoveryRequest
	// whether filter on device_type or not
	switch {
	case len(fdc.config.DeviceType) > 0:
		discoveryRequest = callback.DiscoveryRequest{
			Timestamp: timestamp.Now(),
			Payload: &callback.DiscoveryPayload{
				Filter: callback.DiscoveryDeviceTypeFilter{
					DeviceTypes: []string{string(fdc.config.DeviceType)},
				},
				FilterType:     callback.DiscoveryDeviceTypeFilterType,
				ExternalUserID: userID,
			},
		}
	default:
		discoveryRequest = callback.DiscoveryRequest{
			Timestamp: timestamp.Now(),
			Payload: &callback.DiscoveryPayload{
				ExternalUserID: userID,
			},
		}
	}

	logger.Infof("Trying to do request to steelix for user %s", userID)
	response, err := fdc.steelixClient.CallbackDiscovery(ctx, fdc.config.SkillID, discoveryRequest)
	if err != nil {
		return xerrors.Errorf("failed to call steelix: %w", err)
	}
	if response.Status == "error" {
		return xerrors.Errorf("failed to force discovery for user %s, error code %s, msg: %s", userID, response.ErrorCode, response.ErrorMessage)
	}
	logger.Infof("Successfully sent callback discovery request to steelix for user %s", userID)
	return nil
}

func (fdc *ForceDiscoveryClient) ForceDiscovery() error {
	logger.Infof("Starting force discovery for %s", fdc.config.SkillID)
	ctx := context.Background()

	usersCh := make(chan string)
	go func() {
		defer close(usersCh)
		defer func() {
			if r := recover(); r != nil {
				logger.Infof("Panic in populating force discovery workers with data: %v", r)
			}
		}()
		fdc.usersDataSource.StreamUsersID(ctx, usersCh)
	}()

	type discoveryResult struct {
		UserID string
		err    error
	}

	discoveryResultCh := make(chan discoveryResult)
	discoveryWorkers := fdc.config.RPS
	var wg sync.WaitGroup
	for i := 0; i < discoveryWorkers; i++ {
		wg.Add(1)
		go func(workerID int) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					logger.Warnf("panic in forcing discovery: %v", r)
				}
			}()
			for userID := range usersCh {
				requestID, _ := uuid.NewV4()
				reqCtx := requestid.WithRequestID(contexter.NoCancel(ctx), requestID.String())
				logger.Warnf("Discovering user %s devices for skill %s", userID, fdc.config.SkillID)
				err := fdc.ForceDiscoveryPerUser(reqCtx, userID)
				discoveryResultCh <- discoveryResult{
					UserID: userID,
					err:    err,
				}
				// to make more honest rps
				time.Sleep(time.Second)
			}
			logger.Warnf("worker `%d` has finished", workerID)
		}(i)
	}

	go func() {
		wg.Wait()
		close(discoveryResultCh)
	}()

	failedUsers := make([]string, 0)
	successUsersCount := 0

	for discoveryResult := range discoveryResultCh {
		if discoveryResult.err != nil {
			logger.Warnf("failed to discover user %s devices: %v", discoveryResult.UserID, discoveryResult.err)
			failedUsers = append(failedUsers, discoveryResult.UserID)
		} else {
			successUsersCount++
		}
	}

	logger.Warn("", log.Any("ForceDiscoveryResult", map[string]interface{}{
		"success_users_count": successUsersCount,
		"failed_users_count":  len(failedUsers),
		"failed_users":        failedUsers,
	}))

	return nil
}

func loadDataSource(basePath string, skillID string, dataSourceType DataSourceType, logger *zap.Logger) (IDataSource, error) {
	configPath, err := filepath.Abs(basePath)
	if err != nil {
		return nil, xerrors.Errorf("failed to get abs filepath: %w", err)
	}
	switch dataSourceType {
	case DBDataSourceType:
		var datasourceConfig struct {
			Parameters struct {
				Prefix   string `yaml:"prefix"`
				Endpoint string `yaml:"endpoint"`
				Token    string `config:"YDB_TOKEN,backend=env" yaml:"token"`
			} `yaml:"datasource_parameters"`
		}
		if err = confita.NewLoader(file.NewBackend(configPath), env.NewBackend()).Load(context.Background(), &datasourceConfig); err != nil {
			return nil, xerrors.Errorf("failed to load db datasource from path %s: %w", configPath, err)
		}
		credentials := ydb.AuthTokenCredentials{AuthToken: datasourceConfig.Parameters.Token}
		dbClient, err := ydbclient.NewYDBClient(context.Background(), logger, datasourceConfig.Parameters.Endpoint, datasourceConfig.Parameters.Prefix, credentials, false)
		if err != nil {
			return nil, err
		}
		dbds := &DBDataSource{
			DB:      dbClient,
			Prefix:  datasourceConfig.Parameters.Prefix,
			SkillID: skillID,
		}
		return dbds, nil
	case ConfigDataSourceType:
		var datasourceConfig struct {
			Parameters struct {
				UsersID []string `yaml:"users_id"`
			} `yaml:"datasource_parameters"`
		}
		if err = confita.NewLoader(file.NewBackend(configPath)).Load(context.Background(), &datasourceConfig); err != nil {
			return nil, xerrors.Errorf("failed to load config datasource from path %s: %w", configPath, err)
		}
		cds := &ConfigDataSource{
			UsersID: datasourceConfig.Parameters.UsersID,
			SkillID: skillID,
		}
		return cds, nil
	default:
		return nil, xerrors.Errorf("unknown datasource type in config: %q", dataSourceType)
	}
}

type ForceDiscoveryConfig struct {
	RPS            int              `yaml:"rps"`
	DataSourceType DataSourceType   `yaml:"datasource_type"`
	SteelixURL     string           `yaml:"steelix_url"`
	SteelixTVMID   uint32           `yaml:"steelix_tvm_id"`
	SkillID        string           `yaml:"skill_id"`
	DeviceType     model.DeviceType `yaml:"device_type"`
	UsersID        []string         `yaml:"users_id"`

	SteelixAuthType SteelixAuthType `yaml:"steelix_auth_type"`
	TVMConfig       *TVMConfig      `yaml:"tvm_config"`
	OAuthConfig     *OAuthConfig    `yaml:"oauth_config"`
}

func loadConfig(basePath string) (ForceDiscoveryConfig, error) {
	configPath, err := filepath.Abs(basePath)
	if err != nil {
		return ForceDiscoveryConfig{}, xerrors.Errorf("failed to get abs filepath: %w", err)
	}
	var cfg ForceDiscoveryConfig
	if err = confita.NewLoader(file.NewBackend(configPath)).Load(context.Background(), &cfg); err != nil {
		return ForceDiscoveryConfig{}, xerrors.Errorf("failed to load config from path %s: %w", configPath, err)
	}
	return cfg, err
}

type TVMConfig struct {
	Port       int    `yaml:"port"`
	Token      string `yaml:"token"`
	ClientName string `yaml:"client_name"`
}

type OAuthConfig struct {
	Token string `yaml:"token"`
}
