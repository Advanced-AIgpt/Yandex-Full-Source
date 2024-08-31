package main

import (
	"fmt"
	"os"

	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"

	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/library/go/core/xerrors"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

func initLogging() (*zap.Logger, func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	asyncCore := asynczap.NewCore(
		encoder,
		zapcore.AddSync(os.Stdout),
		uberzap.WarnLevel,
		asynczap.Options{})
	stop := func() {
		asyncCore.Stop()
	}

	logger = zap.NewWithCore(asyncCore, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())
	return logger, stop
}

func main() {
	logger, stop := initLogging()
	defer stop()

	config, err := loadConfig(os.Getenv("MIGRATION_CONFIG_PATH"))
	if err != nil {
		panic(xerrors.Errorf("failed to read config: %w", err))
	}
	datasource, err := loadDataSource(os.Getenv("MIGRATION_CONFIG_PATH"), config.SkillID, config.DataSourceType, logger)
	if err != nil {
		panic(xerrors.Errorf("failed to read config: %w", err))
	}

	msg := fmt.Sprintf("Do you really want to force discovery with skill %s, steelix %s and rps %d?", config.SkillID, config.SteelixURL, config.RPS)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	mc := ForceDiscoveryClient{
		config:          config,
		usersDataSource: datasource,
	}
	mc.Init(logger)

	if err := mc.ForceDiscovery(); err != nil {
		logger.Fatal(err.Error())
	}
}
