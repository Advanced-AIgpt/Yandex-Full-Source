package testapi

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type serviceTicketTVMHandler struct {
	logger   log.Logger
	renderer render.Renderer
	tvm      tvm.Client
}

func NewServiceTicketFromBlackBoxHandler(logger log.Logger, renderer render.Renderer, tvm tvm.Client) http.Handler {
	return &serviceTicketTVMHandler{
		logger:   logger,
		renderer: renderer,
		tvm:      tvm,
	}
}

func (h *serviceTicketTVMHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	dst := r.URL.Query().Get("dst")
	if len(dst) == 0 {
		http.Error(w, "dst cannot be empty", http.StatusBadRequest)
		return
	}
	ticket, err := h.tvm.GetServiceTicketForAlias(r.Context(), dst)
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}
	h.renderer.RenderJSON(r.Context(), w, map[string]string{"ticket": ticket})
}

type checkTvmSrvHandler struct {
	logger   log.Logger
	renderer render.Renderer
	tvm      tvm.Client
}

func NewCheckTvmSrvHandler(logger log.Logger, renderer render.Renderer, tvm tvm.Client) http.Handler {
	return &checkTvmSrvHandler{
		logger:   logger,
		renderer: renderer,
		tvm:      tvm,
	}
}

func (h *checkTvmSrvHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	ticket := r.Header.Get("X-Ya-Service-Ticket")
	if ticket == "" {
		err := fmt.Sprintf("Bad request: missing %s header", "X-Ya-Service-Ticket")
		ctxlog.Warn(r.Context(), h.logger, err)
		http.Error(w, err, http.StatusUnauthorized)
		return
	}

	_, err := h.tvm.CheckServiceTicket(r.Context(), ticket)
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	h.renderer.RenderJSON(r.Context(), w, "OK")
}
