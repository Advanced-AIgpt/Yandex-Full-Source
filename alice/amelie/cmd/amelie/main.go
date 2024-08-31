package main

import (
	"context"
	"flag"
	stdLog "log"
	"time"

	uberzap "go.uber.org/zap"

	"a.yandex-team.ru/alice/amelie/internal/config"
	"a.yandex-team.ru/alice/amelie/internal/server"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var (
	configPathPtr = flag.String("c", "", "path to config file")
)

func loadConfig(logger log.Logger) config.Config {
	flag.Parse()
	cfg, err := config.Load(*configPathPtr)
	if err != nil {
		logger.Fatal(err.Error())
	}
	return cfg
}

func initLogger() *zap.Logger {
	logger, err := zap.NewDeployLogger(log.DebugLevel, uberzap.AddStacktrace(uberzap.ErrorLevel), uberzap.AddCaller())
	if err != nil {
		stdLog.Fatal(err)
	}
	return logger
}

func initServer(logger log.Logger) *server.Server {
	cfg := loadConfig(logger)
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	app, err := server.New(ctx, cfg, initLogger())
	if err != nil {
		logger.Fatal(err.Error())
	}
	return app
}

func main() {
	logger := initLogger()
	defer func() {
		_ = logger.L.Sync()
	}()
	if err := initServer(logger).Serve(); err != nil {
		logger.Fatal(err.Error())
	}
}
