# Middlewares

### TvmServiceTicketGuard

Данный middleware может быть использован как на весь роутер, так и на конкретную ручку API.
`TvmServiceTicketGuard` разрешает доступ к API только запросам, которые подписаны валидным TVM-тикетом, выпущенным для доступа к вашему сервису.

#### Пример использования


```go
func (s *Server) InitRouter() {
	router := chi.NewRouter()

	//System routes
	router.Use(TvmServiceTicketGuard(s.tvm))
	router.Get("/my_secret_api_method", s.secretHandler)
	s.Router = router
}
```

Или

```go
func (s *Server) InitRouter() {
	router := chi.NewRouter()

	//System routes
	router.With(Use(TvmServiceTicketGuard(s.tvm))).Get("/my_secret_api_method", s.secretHandler)
	s.Router = router
}
```
