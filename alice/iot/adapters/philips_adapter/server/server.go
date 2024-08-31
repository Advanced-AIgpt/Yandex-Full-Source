package philips

import (
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/alice/iot/adapters/philips_adapter/philips"
	"a.yandex-team.ru/alice/library/go/middleware"
	r "a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"github.com/go-chi/chi/v5"
)

type Render struct {
	*r.JSONRenderer
}

type Server struct {
	Router       *chi.Mux
	Logger       log.Logger
	render       *Render
	provider     *philips.Provider
	tokenHandler func(*http.Request) (string, error)
	metrics      *solomon.Registry
}

func (s *Server) Init() {
	s.render = &Render{JSONRenderer: &r.JSONRenderer{Logger: s.Logger}}
	s.metrics = solomon.NewRegistry(solomon.NewRegistryOpts())
	s.InitRouter()
	s.InitProvider()
}

func (s *Server) InitProvider() {
	s.provider = &philips.Provider{Logger: s.Logger}
	s.provider.Init(s.metrics)
}

func (s *Server) InitRouter() {
	router := chi.NewRouter()

	router.Use(
		middleware.Recoverer(s.Logger),
		requestid.Middleware(s.Logger),
		middleware.RequestLoggingMiddleware(s.Logger, middleware.IgnoredLogURLPaths...),
	)

	s.tokenHandler = func(req *http.Request) (string, error) {
		authHeader := req.Header.Get("Authorization")
		return strings.Replace(authHeader, "Bearer ", "", 1), nil
	}

	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })

	metricsHandler := httppuller.NewHandler(s.metrics)
	router.Get("/solomon", metricsHandler.ServeHTTP)

	router.Route("/v1.0", func(r chi.Router) {
		r.Head("/", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })
		r.Route("/user", func(r chi.Router) {
			r.Route("/devices", func(r chi.Router) {
				r.Get("/", s.DiscoveryHandler)
				r.Post("/action", s.ActionHandler)
				r.Post("/query", s.QueryHandler)
			})
			r.Post("/unlink", s.UnlinkHandler)
		})
	})

	s.Router = router
}
