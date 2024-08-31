package main

import (
	"context"
	"flag"
	"fmt"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/config"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/server"
	libconfita "a.yandex-team.ru/alice/library/go/confita"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/yandex/yav/httpyav"
)

var (
	configPath string
)

type yavConfig struct {
	secretID string
	token    string
}

func init() {
	flag.StringVar(&configPath, "config", "/etc/bulbasaur.conf", "config folder")
	flag.StringVar(&configPath, "C", "/etc/bulbasaur.conf", "config folder (shortcut)")

	flag.Parse()
}

func loadConfig(ctx context.Context, configPath, envType string, yavConfig yavConfig) (config.Config, error) {
	yavClient, err := httpyav.NewClient(httpyav.WithLogger(zaplogger.NewNop()), httpyav.WithOAuthToken(yavConfig.token))
	if err != nil {
		return config.Config{}, err
	}
	options := []libconfita.BackendOption{
		libconfita.AddDefaultFileBackend(configPath),
		libconfita.AddEnvFileBackend(envType, configPath),
		libconfita.AddYaVaultBackend(yavClient, yavConfig.secretID),
		libconfita.AddEnvBackend(),
	}
	return config.Load(ctx, options...)
}

func main() {
	cfgCtx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()

	yavSecretID := os.Getenv("YAV_SECRET_ID")
	yavToken := os.Getenv("YAV_TOKEN")
	envType := "LOCAL"

	appConfig, err := loadConfig(cfgCtx, configPath, envType, yavConfig{secretID: yavSecretID, token: yavToken})
	if err != nil {
		fmt.Printf("Failed to initialize server: %v\n", err)
		return
	}

	logger := &nop.Logger{}
	s := &bulbasaur.Server{
		Config: appConfig,
		Logger: logger,
	}

	s.Init()
	if uiHandlers, err := getUIHandles(s); err != nil {
		fmt.Println(err)
	} else {
		printAsPythonList(uiHandlers)
		fmt.Printf("Handlers written: %d\n", len(uiHandlers))
	}
}

func printAsPythonList(handlers []HTTPHandler) {
	fmt.Println("[")
	for _, handler := range handlers {
		fmt.Printf("HttpHandler(path=\"%s\", method=\"%s\"),\n", handler.Path, handler.Method)
	}
	fmt.Println("]")
}

type HTTPHandler struct {
	Path   string `json:"path"`
	Method string `json:"method"`
}

func getUIHandles(server *bulbasaur.Server) ([]HTTPHandler, error) {
	var uiHandlers []HTTPHandler
	var counter int
	walker := func(method string, route string, handler http.Handler, _ ...func(http.Handler) http.Handler) error {
		if !strings.HasPrefix(route, "/m/") {
			return nil
		}
		uiHandlers = append(uiHandlers, HTTPHandler{
			Path:   route,
			Method: method,
		})
		counter++
		return nil
	}

	err := chi.Walk(server.Router, walker)
	fmt.Printf("Total: %d\n", counter)

	return uiHandlers, err
}
