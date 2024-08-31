[log]
level = "INFO"
filePath = "/var/log/traefik/debug.log"

[accessLog]
filePath = "/var/log/traefik/access.log"
bufferingSize = 100
[accessLog.filters]
statusCodes = ["500-599"]

[wss]
protocol = "http"

[entryPoints.http]
address = "[::]:8000"
[entryPoints.auth_api]
address = "[::]:8099"

[api]
dashboard = true

[providers.redis]
endpoints = ["{{ redis_endpoint }}"]
rootKey = "{{ env }}/proxy/traefik"
password = "{{ redis_password }}"

[providers.file]
directory = "/srv/traefik.conf"

[metrics]
[metrics.prometheus]

[ping]
entryPoint = "auth_api"
