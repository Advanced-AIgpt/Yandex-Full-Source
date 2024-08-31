package main

import (
	"context"
	"flag"
	"fmt"
	"golang.org/x/sync/errgroup"
	"gopkg.in/yaml.v2"
	"math/rand"
	"os"
	"os/signal"
	"runtime"
	"strings"
	"syscall"
	"time"

	"a.yandex-team.ru/alice/iot/steelix/config"
	steelix "a.yandex-team.ru/alice/iot/steelix/server"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/maxprocs"
)

var (
	configPath  string
	configDebug bool
)

func init() {
	flag.StringVar(&configPath, "config", "/etc/steelix.conf", "config folder")
	flag.StringVar(&configPath, "C", "/etc/steelix.conf", "config folder (shortcut)")
	flag.BoolVar(&configDebug, "debug", false, "show config, don't start server")
	flag.Parse()
}

func main() {
	if err := run(); err != nil {
		panic(err)
	}
}

func run() error {
	ctx := context.Background()

	// 1. Load config
	appConfig, err := config.Load(configPath)
	if err != nil {
		return xerrors.Errorf("failed to load config: %w", err)
	}

	if configDebug {
		yamlConfig, _ := yaml.Marshal(appConfig)
		fmt.Printf("config loaded successfully\n\n%s", yamlConfig)
		return nil
	}

	// 2. Set GOMAXPROCS
	setMaxProcs(appConfig.CloudType)

	// 3. Init logger
	logger, stop, err := initLogging(appConfig.Logging)
	if err != nil {
		return xerrors.Errorf("failed to init logger: %w", err)
	}
	defer stop()

	// 4. Start server
	logger.Info("Starting Steelix server")
	logger.Infof("adjust GOMAXPROCS to %d", runtime.GOMAXPROCS(0))
	rand.Seed(time.Now().UTC().UnixNano())

	app := steelix.Server{Config: appConfig, Logger: logger}
	if err = app.Init(ctx); err != nil {
		return xerrors.Errorf("failed to init app: %w", err)
	}

	wg, ctx := errgroup.WithContext(ctx)
	wg.Go(func() error {
		return app.Serve(ctx)
	})

	wg.Go(func() error {
		return waitSigtermSignal(ctx, logger)
	})

	if err = wg.Wait(); err != nil {
		return xerrors.Errorf("finish app with error: %w", err)
	}

	return nil
}

func setMaxProcs(cloudType string) {
	switch strings.ToLower(cloudType) {
	case "yp", "deploy":
		maxprocs.AdjustYP()
	case "yp_lite":
		maxprocs.AdjustYPLite()
	case "instancectl":
		maxprocs.AdjustInstancectl()
	default:
		maxprocs.AdjustAuto()
	}
}

func waitSigtermSignal(ctx context.Context, logger log.Logger) error {
	stopChan := make(chan os.Signal, 1)
	signal.Notify(stopChan, syscall.SIGTERM)
	select {
	case <-stopChan:
		logger.Infof("the application received SIGTERM signal")
		return xerrors.New("SIGTERM signal")
	case <-ctx.Done():
		return nil
	}
}
