Для конвертации бинарного лога eventlog в читаемый формат использовать arcadia/voicetech/tools/evlogdump

Пример:

```
(cd ../../../../voicetech/tools/evlogdump && ya make) && ../../../../voicetech/tools/evlogdump/evlogdump cuttlefish.evlog
```

Для конвертации бинарного RT лога (см. server/rtlog в json конфиге) в читаемый формат использовать arcadia/alice/rtlog/evlogdump

Пример:

```
(cd ../../../rtlog/evlogdump && ya make) && ../../../rtlog/evlogdump/evlogdump cuttlefish.rtlog