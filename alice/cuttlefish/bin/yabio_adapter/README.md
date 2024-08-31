Для конвертации бинарного лога (см. server/log/eventlog в default_config.json и asr_adapter.json) в читаемый формат использовать arcadia/voicetech/tools/evlogdump

Пример:

```
(cd ../../../../voicetech/tools/evlogdump && ya make) && ../../../../voicetech/tools/evlogdump/evlogdump eventlog
```

Для конвертации бинарного RT лога (см. server/rtlog в json конфиге) в читаемый формат использовать arcadia/alice/rtlog/evlogdump

Пример:

```
(cd ../../../rtlog/evlogdump && ya make) && ../../../rtlog/evlogdump/evlogdump rtlog
```