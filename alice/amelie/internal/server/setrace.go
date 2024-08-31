package server

import (
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"context"
	"fmt"
	"net/url"
	"os"
	"syscall"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/amelie/internal/config"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"
	"a.yandex-team.ru/library/go/core/log/zap/logrotate"
)

var (
	boxID   = os.Getenv("DEPLOY_BOX_ID")
	podFQDN = os.Getenv("DEPLOY_POD_PERSISTENT_FQDN")
)

func getHostName() string {
	if len(boxID) > 0 && len(podFQDN) > 0 {
		return fmt.Sprintf("%s.%s", boxID, podFQDN)
	}
	name, err := os.Hostname()
	if err != nil {
		return "unknown"
	}
	return name
}

func (s *Server) newSetraceCore() (core zapcore.Core, stop func(), err error) {
	var environment string
	level := uberzap.DebugLevel
	switch s.cfg.Env.Type { // https://wiki.yandex-team.ru/partner/w/errorbooster/#environment
	case config.Stable:
		environment = "production"
	//	environment = "pre_production"
	case config.Testing:
		environment = "testing"
	default:
		environment = "development"
	}
	encoder := setrace.NewSetraceEncoder(environment, s.cfg.Env.Service, getHostName(), zapcore.EncoderConfig{})
	if len(s.cfg.Logger.Setrace.LogsPath) == 0 {
		return core, stop, fmt.Errorf("invalid logs path: %s", s.cfg.Logger.Setrace.LogsPath)
	}
	u, err := url.ParseRequestURI(s.cfg.Logger.Setrace.LogsPath)
	if err != nil {
		return core, stop, fmt.Errorf("can't parse filepath as uri: %w", err)
	}
	logrotateSink, err := logrotate.NewLogrotateSink(u, syscall.SIGUSR1)
	if err != nil {
		return core, stop, fmt.Errorf("can't create logrotate sink: %w", err)
	}
	asyncCore := asynczap.NewCore(encoder, zapcore.Lock(logrotateSink), level, asynczap.Options{})
	setraceCore := setrace.NewEntryFilteringCore(asyncCore, setrace.FilterSetraceEntry)
	stop = func() { asyncCore.Stop() }
	return setraceCore, stop, nil
}

func (s *Server) newYTCore() (core zapcore.Core, stop func(), err error) {
	var encoder zapcore.Encoder
	var level zapcore.Level
	switch s.cfg.Env.Type {
	case config.Stable, config.Testing:
		encoderConfig := uberzap.NewProductionEncoderConfig()
		encoderConfig.EncodeTime = zaplogger.EpochMicrosTimeEncoder
		encoder = zaplogger.NewDatetimeEncoderWrapper(zapcore.NewJSONEncoder(encoderConfig))
		level = uberzap.InfoLevel
	default:
		encoderConfig := uberzap.NewDevelopmentEncoderConfig()
		encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
		encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
		encoder = zaplogger.NewDatetimeEncoderWrapper(zapcore.NewConsoleEncoder(encoderConfig))
		level = uberzap.DebugLevel
	}
	if len(s.cfg.Logger.Setrace.YTLogsPath) == 0 {
		return core, stop, fmt.Errorf("invalid logs path: %s", s.cfg.Logger.Setrace.YTLogsPath)
	}
	u, err := url.ParseRequestURI(s.cfg.Logger.Setrace.YTLogsPath)
	if err != nil {
		return core, stop, fmt.Errorf("can't parse filepath as uri: %w", err)
	}
	logrotateSink, err := logrotate.NewLogrotateSink(u, syscall.SIGUSR2)
	if err != nil {
		return core, stop, fmt.Errorf("can't create logrotate sink: %w", err)
	}
	asyncCore := asynczap.NewCore(encoder, zapcore.Lock(logrotateSink), level, asynczap.Options{})
	stop = func() { asyncCore.Stop() }
	return asyncCore.With([]zapcore.Field{
		uberzap.String("host", getHostName()),
		uberzap.String("env", string(s.cfg.Env.Type)),
		uberzap.String("service", s.cfg.Env.Service),
		uberzap.Namespace("@fields"),
	}), stop, nil
}

func (s *Server) initSetraceLogger(ctx context.Context) error {
	setraceCore, setraceStop, err := s.newSetraceCore()
	if err != nil {
		return fmt.Errorf("setrace logger init error: %w", err)
	}
	ytCore, ytStop, err := s.newYTCore()
	if err != nil {
		return fmt.Errorf("setrace logger init error: %w", err)
	}
	var opts []uberzap.Option
	if s.cfg.Env.Type == config.Dev {
		opts = append(opts, uberzap.AddStacktrace(uberzap.ErrorLevel), uberzap.AddCaller())
	}
	s.logger = zap.NewWithCore(zapcore.NewTee(setraceCore, ytCore), opts...)
	s.addShutdownDelegate(func(ctx context.Context) {
		s.serviceLogger.Info("Stopping setrace logger")
		setraceStop()
		ytStop()
	})
	return nil
}
