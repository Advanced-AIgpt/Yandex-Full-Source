[http.routers.api]
entrypoints = ["auth_api"]
service = "api@internal"
middlewares = ["apiAuth"]
rule = "PathPrefix(`/api`) || PathPrefix(`/dashboard`)"

[http.middlewares.apiAuth.basicAuth]
users = [
    "jupyterhub:{{ traefik_hashed_password }}",
]
