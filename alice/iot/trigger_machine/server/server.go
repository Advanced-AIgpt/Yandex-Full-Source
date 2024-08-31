package triggermachine

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/trigger_machine/config"
	"a.yandex-team.ru/library/go/core/log"
	"github.com/go-chi/chi/v5"
)

type Server struct {
	Config config.Config
	Logger log.Logger
	Router *chi.Mux
}

func (s *Server) Init() {
	s.Logger.Info("Initializing Trigger Machine...")

	s.InitRouter()

	s.Logger.Info("Trigger Machine was successfully initialized")
}

func (s *Server) InitRouter() {
	router := chi.NewRouter()

	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })

	s.Router = router
}
