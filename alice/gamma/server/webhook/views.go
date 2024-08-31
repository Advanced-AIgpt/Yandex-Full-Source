package webhook

import (
	"context"
	"net/http"
	"os"
	"path"
	"time"

	"github.com/labstack/echo/v4"
	"go.uber.org/zap"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/gamma/metrics"
	"a.yandex-team.ru/alice/gamma/metrics/generic"
	"a.yandex-team.ru/alice/gamma/metrics/solomon"
	"a.yandex-team.ru/alice/gamma/server/log"
	"a.yandex-team.ru/alice/gamma/server/skills"
	"a.yandex-team.ru/alice/gamma/server/storage"
	webhookApi "a.yandex-team.ru/alice/gamma/server/webhook/api"
	"a.yandex-team.ru/alice/gamma/server/webhook/api/admin"
	"a.yandex-team.ru/alice/gamma/server/webhook/handlers"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

var (
	defaultTimerConfig = generic.TimerConfig{
		Unit: time.Millisecond,
		Bounds: []int64{
			10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 175, 200, 225, 250, 300, 500, 1000,
		}}
	defaultHistConfig = generic.HistConfig{
		Bounds: []float64{
			10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 175, 200, 225, 250, 300, 500, 1000,
		}}
)

func (server *Server) ServeSkill(ctx echo.Context) (err error) {
	serveSkillRegistry := server.baseRegistry.WithPrefix("webhook/serveSkill")

	var request webhookApi.Request
	if err = ctx.Bind(&request); err != nil {
		serveSkillRegistry.Counter("response_400").Inc()
		log.Warnf("Malformed request: %+v", err)
		return ctx.String(http.StatusBadRequest, "Bad request")
	}

	loggingContext := log.LoggingContext{
		Context: ctx.Request().Context(),
		Logger: log.With(
			zap.String("uuid", request.Session.UserID),
			zap.String("session_id", request.Session.SessionID),
			zap.String("skill_id", request.Session.SkillID),
			zap.Int64("message_id", request.Session.MessageID),
		),
	}

	skillID := ctx.Param("skill")
	if _, ok := request.Meta.Experiments["test_gamma_skill"]; ok { // keep experiment flag for logging & etc
		loggingContext.Logger.Infof("Request for skill %s from test bot: %+v", skillID, request)
	} else {
		loggingContext.Logger.Infof("Request for skill %s: %+v", skillID, request)
	}

	response, err := server.skillHandler.Handle(loggingContext, skillID, &request)
	if err != nil {
		if xerrors.Is(err, skills.NotFoundError) {
			serveSkillRegistry.Counter("response_404").Inc()
			loggingContext.Logger.Warn("Skill not found %s", skillID)
			return ctx.String(http.StatusNotFound, "Not found")
		}
		serveSkillRegistry.Counter("response_500").Inc()
		loggingContext.Logger.Errorf("Skill handle failed: %+v", err)
		return ctx.String(http.StatusInternalServerError, "Skill is not responding")
	}
	serveSkillRegistry.Counter("response_200").Inc()
	loggingContext.Logger.Infof("Response from skill %s: %+v", skillID, response)
	return ctx.JSON(http.StatusOK, response)
}

func (server *Server) GetTestSkill(ctx echo.Context) (err error) {
	getTestSkillRegistry := server.baseRegistry.WithPrefix("webhook/getTestSkill")

	var requests []admin.Request
	if err = ctx.Bind(&requests); err != nil {
		getTestSkillRegistry.Counter("response_400").Inc()
		log.Warnf("Malformed json: %+v", err)
		return ctx.String(http.StatusBadRequest, "Bad request")
	}
	getTestSkillRegistry.Counter("response_200").Inc()
	log.Debugf("Got test skill requests %+v", requests)
	response := server.skillAdmin.GetTestSkill(ctx.Request().Context(), requests, func(skillId string) string {
		return "http://" + path.Join(ctx.Request().Host, "skill", skillId)
	})
	return ctx.JSON(http.StatusOK, response)
}

func (server *Server) ServeMetrics(ctx echo.Context) (err error) {
	log.Info("Solomon metrics pull")
	response, err := solomon.Encode(server.metricsStorage)
	if err != nil {
		log.Error(err)
		return ctx.String(http.StatusInternalServerError, "Solomon metrics encoding error")
	}
	return ctx.JSON(http.StatusOK, response)
}

type Server struct {
	metricsStorage metrics.Storage
	baseRegistry   *metrics.InMemoryRegistry
	skillAdmin     *handlers.SkillAdmin
	skillHandler   *handlers.SkillHandler
}

func (server *Server) WithSensors(registryPrefix, timerName string) func(echo.HandlerFunc) echo.HandlerFunc {
	handlerRegistry := server.baseRegistry.WithPrefix(registryPrefix)
	return func(call echo.HandlerFunc) echo.HandlerFunc {
		return func(c echo.Context) (err error) {
			handlerRegistry.Timer(timerName).Observe(func() {
				err = call(c)
			})
			server.baseRegistry.Rate("requests").Add(1)
			handlerRegistry.Rate("requests").Add(1)
			if err != nil {
				c.Error(err)
			}
			return nil
		}
	}
}

type ServerConfig struct {
	Skills  []skills.Info `yaml:"skills"`
	Storage struct {
		Ydb struct {
			Endpoint        string        `yaml:"endpoint"`
			DialTimeout     time.Duration `yaml:"dial_timeout"`
			RequestTimeout  time.Duration `yaml:"request_timeout"`
			Database        string        `yaml:"database"`
			TablePathPrefix string        `yaml:"table_path_prefix"`
		} `yaml:"ydb"`
	} `yaml:"storage"`
	EchoReadTimeout  time.Duration   `yaml:"echo_read_timeout"`
	EchoWriteTimeout time.Duration   `yaml:"echo_write_timeout"`
	Metrics          *metrics.Config `yaml:"metrics"`
}

func CreateServer(cfg *ServerConfig) *Server {
	if cfg == nil {
		cfg = &ServerConfig{}
		cfg.Metrics.SetDefaultTimerConfig(defaultTimerConfig)
		cfg.Metrics.SetDefaultHistConfig(defaultHistConfig)
	}
	skillsMap := make(map[string]skills.Info)
	for _, skill_ := range cfg.Skills {
		skillsMap[skill_.ID] = skill_
	}

	metricsStorage, baseRegistry := metrics.BaseRegistry("gamma/server", nil, cfg.Metrics)
	ydbConfig := new(ydb.DriverConfig)

	ydbConfig.Database = cfg.Storage.Ydb.Database
	ydbConfig.RequestTimeout = cfg.Storage.Ydb.RequestTimeout
	ydbConfig.Credentials = ydb.AuthTokenCredentials{
		AuthToken: os.Getenv("GAMMA_YDB_TOKEN"),
	}

	storageProvider := storage.YdbStorageFactory{
		Endpoint:        cfg.Storage.Ydb.Endpoint,
		DialTimeout:     cfg.Storage.Ydb.DialTimeout,
		TablePathPrefix: cfg.Storage.Ydb.TablePathPrefix,
		Context:         context.Background(),
		Config:          ydbConfig,
	}
	storage_, err := storage.CreateStorage(&storageProvider)

	if err != nil {
		log.Fatal("Failed starting server: ", err)
	}

	providerFactory := skills.InMemoryProviderFactory{SkillsMap: skillsMap}
	provider_, err := providerFactory.CreateProvider()
	if err != nil {
		log.Fatal("Failed starting server: ", err)
	}

	return &Server{
		metricsStorage: metricsStorage,
		baseRegistry:   baseRegistry,
		skillAdmin:     handlers.CreateAdmin(provider_),
		skillHandler:   handlers.CreateHandler(provider_, storage_),
	}
}
