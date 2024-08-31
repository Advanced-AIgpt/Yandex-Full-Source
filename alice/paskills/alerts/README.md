Документация monitorado: https://a.yandex-team.ru/arc/trunk/arcadia/frontend/packages/monitorado

Посмотреть дифф после изменения `monitorado.yml`:

```bash
monitorado -c monitorado.yml diff
```

Применить изменения:

```bash
monitorado -c monitorado.yml exec
```

Сгенерировать сигналы для источников АПИ
```bash
npm i
# host бокса api
cd ./monitorado_api_sources
./updateApiSoures.js api.fs6yk5odeeaf54cn.sas.yp-c.yandex.net