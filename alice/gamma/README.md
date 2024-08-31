# Γ-skill-engine

Γ-skill-engine (spells _gamma skill engine_) — Engine and SDK for Alice, Yandex personal assistant

### Links
* [Wiki](https://wiki.yandex-team.ru/alice/skill-engine-and-sdk/)
* Instances:
    * Gamma skill engine testing (gskill-test.n.yandex-team.ru) 
[[nanny](https://nanny.yandex-team.ru/ui/#/services/catalog/gskill-test/general_info)]
    * Echo skill [[nanny]](https://nanny.yandex-team.ru/ui/#/services/catalog/echo-test/)

### How to build
```$bash
# Compiling protobuf
ya make sdk/api --add-result=go --add-result=py

# Compiling gamma server
ya make server

# Compiling echo skill
ya make skills/echo
```

### How to start
#### Gamma Server
```$bash
./gamma-server --port 8000 --sdk-port 8002 --config gamma.yaml
```

- `--port` — webhook listnening port
- `--config` — path to webhook config in [yaml format](#config-file-format)
- `--sdk-port` — grpc sdk listening port

##### Config File Format
```yaml
skills_client:
  skills:
  - id: echo  # Skill id
    addr: localhost:8001  # Skill address
webhook_server:
  storage:
    ydb:
      endpoint: "ydb-ru-prestable.yandex.net:2135" # Ydb endpoint
      dial_timeout: 1s  # Timeout to establish connection to ydb. Type: time.Duration
      request_timeout: 300ms  # Ydb per request timeout. Type: time.Duration
      database: "/ru-prestable/alice/test/gamma"  # Database name
      table_path_prefix: "/ru-prestable/alice/test/gamma/skills/"  # Path prefix for all per skill tables.
```


#### Echo skill
```$bash
./echo --port 8001 --api localhost:8002
```

- `--port` — skill grpc port
- `--api` — address of sdk grpc
