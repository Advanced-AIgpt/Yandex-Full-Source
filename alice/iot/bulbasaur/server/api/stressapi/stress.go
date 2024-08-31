package stressapi

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/stress"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// Handlers is a temporary api handler for solomon batching stress test
type Handlers struct {
	logger            log.Logger
	renderer          render.Renderer
	historyController history.IController
}

func NewHandlers(logger log.Logger, renderer render.Renderer, historyController history.IController) *Handlers {
	return &Handlers{
		logger:            logger,
		renderer:          renderer,
		historyController: historyController,
	}
}

func (h *Handlers) CallbackHistorySolomonHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Error reading body: %v", err)
		h.renderer.RenderCallbackError(r.Context(), w, err)
		return
	}
	var req stress.UpdateSolomonStateRequest
	if err := json.Unmarshal(body, &req); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "failed to unmarshal body: %v", err)
		h.renderer.RenderCallbackError(r.Context(), w, err)
		return
	}

	deviceProperties := map[string]model.Properties{
		req.DeviceID: {
			model.MakePropertyByType(model.FloatPropertyType).
				WithState(model.FloatPropertyState{
					Instance: req.Instance,
					Value:    req.Value,
				}).
				WithLastUpdated(req.Timestamp),
		},
	}
	if err := h.historyController.PushMetricsToSolomon(r.Context(), deviceProperties); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "failed to send metrics to solomon: %v", err)
		h.renderer.RenderCallbackError(r.Context(), w, err)
		return
	}

	h.renderer.RenderCallbackOk(r.Context(), w)
}
