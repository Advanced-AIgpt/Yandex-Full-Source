package main

import (
	"a.yandex-team.ru/library/go/maxprocs"
	"fmt"
	"github.com/go-chi/chi/v5"
	"io"
	"net/http"
	"strconv"
	"time"
)

func main() {
	maxprocs.AdjustYP()

	router := chi.NewRouter()

	router.Get("/ping", pingHandler)

	router.Route("/paskills", func(r chi.Router) {
		r.Route("/public/v1", func(r chi.Router) {
			r.Get("/status", bigJSONHandler)
			r.Route("/skills/{skillId}", func(r chi.Router) {
				r.Route("/images", func(r chi.Router) {
					r.Get("/", bigJSONHandler)
					r.Post("/", jsonHandler)
				})
				r.Route("/sounds", func(r chi.Router) {
					r.Get("/", bigJSONHandler)
					r.Post("/", jsonHandler)
				})
			})
		})
		r.HandleFunc("/*", errorHandler)
	})

	router.Route("/bulbasaur", func(r chi.Router) {
		r.Route("/api/v1/", func(r chi.Router) {
			r.Post("/skills/{skillId}/callback", jsonHandler)
		})
		r.HandleFunc("/*", errorHandler)
	})

	_ = http.ListenAndServe(":8080", router)
}

func pingHandler(w http.ResponseWriter, r *http.Request) {
	_, _ = fmt.Fprint(w, "OK")
}

func mirrorHandler(w http.ResponseWriter, r *http.Request) {
	sleep(r.URL.Query().Get("delay"))

	defer func() { _ = r.Body.Close() }()
	w.Header().Set("Content-Type", r.Header.Get("Content-Type"))
	w.WriteHeader(http.StatusOK)
	_, _ = io.Copy(w, r.Body)
}

func jsonHandler(w http.ResponseWriter, r *http.Request) {
	sleep(r.URL.Query().Get("delay"))

	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(http.StatusOK)
	_, _ = fmt.Fprint(w, `{"result":"OK"}`)
}

func bigJSONHandler(w http.ResponseWriter, r *http.Request) {
	sleep(r.URL.Query().Get("delay"))

	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(http.StatusOK)
	_, _ = fmt.Fprint(w, `{"status":"ok","request_id":"adfab7fa-556d-4696-b68a-a2cab7252c1c","rooms":[{"id":"f2a450c5-5d2a-4f0a-bb7a-758b682b3d5d","name":"Кабинет","devices":[{"id":"956be9c7-b5f2-4558-919a-04a10bb30f8c","name":"Кондиционер","type":"devices.types.thermostat.ac","capabilities":[{"retrievable":true,"type":"devices.capabilities.on_off","state":null,"parameters":{}}],"groups":[],"skill_id":"T"},{"id":"ca3981e5-474d-4caa-ba24-332d2a549ee1","name":"Лампа сяоми","type":"devices.types.light","capabilities":[{"retrievable":true,"type":"devices.capabilities.on_off","state":{"instance":"on","value":true},"parameters":{}}],"groups":[],"skill_id":"ad26f8c2-fc31-4928-a653-d829fda7e6c2"},{"id":"e3a0b5e9-272e-48f2-9dbc-6cadc4b15366","name":"Лампа филипс","type":"devices.types.light","capabilities":[{"retrievable":true,"type":"devices.capabilities.on_off","state":{"instance":"on","value":true},"parameters":{}}],"groups":[],"skill_id":"4a8cbce2-61d3-4e58-9f7b-6b30371d265c"},{"id":"bd3b10bb-2a50-49ac-ace0-7d4695756328","name":"Пульт","type":"devices.types.hub","capabilities":[],"groups":[],"skill_id":"T"}]}],"groups":[],"unconfigured_devices":[]}`)
}

func errorHandler(w http.ResponseWriter, r *http.Request) {
	sleep(r.URL.Query().Get("delay"))

	code, _ := strconv.Atoi(r.URL.Query().Get("code"))
	if code == 0 {
		code = http.StatusBadRequest
	}

	w.WriteHeader(code)
	_, _ = fmt.Fprint(w, http.StatusText(code))
}

func sleep(delay string) {
	delayInt, _ := strconv.ParseInt(delay, 10, 64)

	if delayInt > 0 {
		time.Sleep(time.Duration(delayInt) * time.Millisecond)
	}
}
