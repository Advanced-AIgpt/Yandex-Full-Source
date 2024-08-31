package experiments

import (
	"net/http"
)

// ManagerMiddleware is stored here because of our dear friend import cycle "db -> logging -> model -> middleware" ><
func ManagerMiddleware(factory IManagerFactory) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			*r = *r.WithContext(ContextWithManager(r.Context(), factory.NewManager()))
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
