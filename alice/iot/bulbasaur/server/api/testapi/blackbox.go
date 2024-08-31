package testapi

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/library/go/userctx"
)

type userUIDFromBlackBoxHandler struct {
	renderer render.Renderer
}

func NewUserUIDFromBlackBoxHandler(renderer render.Renderer) http.Handler {
	return &userUIDFromBlackBoxHandler{
		renderer: renderer,
	}
}

func (h *userUIDFromBlackBoxHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	user, err := userctx.GetUser(r.Context())
	if err != nil {
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	response := map[uint64]string{user.ID: user.Login}
	h.renderer.RenderJSON(r.Context(), w, response)
}

type userTicketFromBlackBoxHandler struct {
	renderer render.Renderer
}

func NewUserTicketFromBlackBoxHandler(renderer render.Renderer) http.Handler {
	return &userTicketFromBlackBoxHandler{
		renderer: renderer,
	}
}

func (h *userTicketFromBlackBoxHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	userTicket := userctx.GetUserTicket(r.Context())
	if len(userTicket) == 0 {
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	h.renderer.RenderJSON(r.Context(), w, map[string]string{"ticket": userTicket})
}
