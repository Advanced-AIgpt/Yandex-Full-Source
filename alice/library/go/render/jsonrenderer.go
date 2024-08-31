package render

import (
	"context"
	"encoding/json"
	"net/http"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type JSONRenderer struct {
	Logger log.Logger
}

func (j *JSONRenderer) renderJSON(ctx context.Context, w http.ResponseWriter, code int, payload interface{}) {
	jsonPayload, err := json.Marshal(payload)
	if err != nil {
		ctxlog.Warn(ctx, j.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	ctxlog.Debugf(ctx, j.Logger, "Responding with: %s", string(jsonPayload))

	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(code)
	_, _ = w.Write(jsonPayload) // or use fmt.Fprintln(w, jsonPayload)
}

func (j *JSONRenderer) RenderJSON(ctx context.Context, w http.ResponseWriter, payload interface{}) {
	j.renderJSON(ctx, w, http.StatusOK, payload)
}

func (j *JSONRenderer) RenderJSONError(ctx context.Context, w http.ResponseWriter, code int, payload interface{}) {
	j.renderJSON(ctx, w, code, payload)
}
