package testapi

import (
	"github.com/go-chi/chi/v5"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type skillInfoHandler struct {
	logger   log.Logger
	renderer render.Renderer
	dialogs  dialogs.Dialoger
}

func NewSkillInfoHandler(logger log.Logger, renderer render.Renderer, dialogs dialogs.Dialoger) http.Handler {
	return &skillInfoHandler{
		logger:   logger,
		renderer: renderer,
		dialogs:  dialogs,
	}
}

func (h *skillInfoHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	skillID := chi.URLParam(r, "skillId")
	skillInfo, err := h.dialogs.GetSkillInfo(r.Context(), skillID, "")
	if err != nil {
		errMessage := xerrors.Errorf("failed to get skill info from Dialogs API: %w", err)
		ctxlog.Warn(r.Context(), h.logger, errMessage.Error())
		http.Error(w, errMessage.Error(), http.StatusInternalServerError)
		return
	}

	response := map[string]string{
		"name":            skillInfo.Name,
		"applicationName": skillInfo.ApplicationName,
		"endpointUrl":     skillInfo.BackendURL,
	}
	h.renderer.RenderJSON(r.Context(), w, response)
}
