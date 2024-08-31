package sdk

import (
	"net"

	lru "github.com/hashicorp/golang-lru"
	"google.golang.org/grpc"

	"a.yandex-team.ru/alice/gamma/sdk/api"
	"a.yandex-team.ru/alice/gamma/server/log"
)

type Server struct {
	compiledPatternsCache *lru.Cache
}

type ServerConfig struct {
	CacheSize int `yaml:"cache_size"`
}

func StartSdkServer(cfg *ServerConfig, addr string) error {
	if cfg == nil {
		cfg = &ServerConfig{}
	}

	listener, err := net.Listen("tcp", addr)
	if err != nil {
		return err
	}

	grpcServer := grpc.NewServer()
	cache, err := lru.New(cfg.CacheSize)
	log.Debugf("Initialized patterns cache, size %d", cfg.CacheSize)

	if err != nil {
		return err
	}
	api.RegisterSdkServer(
		grpcServer,
		&Server{
			compiledPatternsCache: cache,
		},
	)

	log.Infof("Starting sdk server on %s", addr)
	return grpcServer.Serve(listener)
}
