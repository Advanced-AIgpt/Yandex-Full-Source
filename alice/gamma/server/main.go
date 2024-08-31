package main

import (
	"flag"
	"fmt"
	stdLog "log"
	"net/http"
	"os"
	"strings"

	"github.com/labstack/echo/v4"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/alice/gamma/server/log"
	"a.yandex-team.ru/alice/gamma/server/sdk"
	"a.yandex-team.ru/alice/gamma/server/webhook"
)

type Config struct {
	WebhookServer *webhook.ServerConfig `yaml:"webhook_server"`
	SdkServer     *sdk.ServerConfig     `yaml:"sdk_server"`
	LogConfig     *log.Config           `yaml:"log"`
}

func loadConfig(filePath string) (*Config, error) {
	file, err := os.Open(filePath)
	if err != nil {
		return nil, err
	}
	defer func() {
		_ = file.Close()
	}()

	var config Config

	decoder := yaml.NewDecoder(file)

	if err := decoder.Decode(&config); err != nil {
		return nil, err
	}
	return &config, nil
}

func CheckStatus(ctx echo.Context) error {
	return ctx.String(http.StatusOK, "Ok")
}

type JSONBinder struct{}

func (*JSONBinder) Bind(i interface{}, ctx echo.Context) (err error) {
	req := ctx.Request()
	if !strings.HasPrefix(req.Header.Get(echo.HeaderContentType), echo.MIMEApplicationJSON) {
		return echo.ErrUnsupportedMediaType
	}
	return new(echo.DefaultBinder).Bind(i, ctx)
}

func main() {
	portPtr := flag.Int("port", 8000, "Listening port")
	sdkPortPtr := flag.Int("sdk-port", 8002, "Sdk grpc listening port")
	configPathPtr := flag.String("config", "gamma.yaml", "Path to config file")
	flag.Parse()

	config, err := loadConfig(*configPathPtr)
	if err != nil {
		stdLog.Fatal(err)
	}
	log.Setup(config.LogConfig)
	defer func() {
		_ = log.Sync()
	}()

	client := webhook.CreateServer(config.WebhookServer)

	go func() {
		log.Fatalf("SDK Server Error: %v", sdk.StartSdkServer(config.SdkServer, fmt.Sprintf(":%d", *sdkPortPtr)))
	}()

	server := echo.New()
	server.Server.ReadTimeout = config.WebhookServer.EchoReadTimeout
	server.Server.WriteTimeout = config.WebhookServer.EchoWriteTimeout

	server.Binder = &JSONBinder{}

	server.POST("/skill/:skill", client.ServeSkill, client.WithSensors("webhook/serveSkill", "responseTimesMillis"))
	server.POST("/dialog/test", client.GetTestSkill, client.WithSensors("webhook/getTestSkill", "responseTimesMillis"))
	server.GET("/metrics", client.ServeMetrics)
	server.GET("/ping", CheckStatus)

	log.Infof("Starting webhook listening on port %d", *portPtr)
	log.Fatal(
		server.Start(fmt.Sprintf(":%d", *portPtr)),
	)
}
