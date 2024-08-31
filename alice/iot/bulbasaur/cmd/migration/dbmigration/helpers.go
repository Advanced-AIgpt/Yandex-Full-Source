package dbmigration

import (
	"context"
	"fmt"
	"os"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func createLogger() (logger log.Logger, cleanup func()) {
	encoderConfig := uzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uzap.DebugLevel)
	cleanup = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uzap.AddStacktrace(uzap.FatalLevel), uzap.AddCaller())
	return logger, cleanup
}

type Environment struct {
	Ctx     context.Context
	Logger  log.Logger
	Mdb     *Client
	cleanup func()
}

func (e *Environment) Close() {
	e.cleanup()
}

func AskMigration(migrationName string) Environment {
	logger, cleanup := createLogger()
	mdb := NewClientFromEnv(logger)
	msg := fmt.Sprintf("Do you want start migration %q for DB: %s?", migrationName, mdb.Description())

	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(1)
	}

	logger.WithName("migration")

	return Environment{
		Ctx:     context.Background(),
		Logger:  logger,
		Mdb:     mdb,
		cleanup: cleanup,
	}
}

func AskMigrationWithDescription(migrationName, migrationDescription string) Environment {
	logger, cleanup := createLogger()
	mdb := NewClientFromEnv(logger)
	msg := fmt.Sprintf("Do you want start migration %q for DB: %s?\n%s", migrationName, mdb.Description(), migrationDescription)

	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(1)
	}

	logger.WithName("migration")

	return Environment{
		Ctx:     context.Background(),
		Logger:  logger,
		Mdb:     mdb,
		cleanup: cleanup,
	}
}
