package main

import (
	"flag"
	"log"

	"a.yandex-team.ru/alice/amanda/internal/core"
	"a.yandex-team.ru/alice/amanda/internal/core/config"
)

func main() {
	configPathPtr := flag.String("c", "", "Path to config file")
	flag.Parse()
	cfg, err := config.Load(*configPathPtr)
	if err != nil {
		log.Panic(err)
	}
	if err := core.Serve(*cfg); err != nil {
		log.Panic(err)
	}
}
