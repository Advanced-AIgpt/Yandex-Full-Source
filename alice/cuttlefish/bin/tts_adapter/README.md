Для конвертации бинарного лога (см. server/log/eventlog в default_config.json и tts_adapter.json) в читаемый формат использовать arcadia/voicetech/tools/evlogdump

Пример:

```
(cd ../../../../voicetech/tools/evlogdump && ya make) && ../../../../voicetech/tools/evlogdump/evlogdump eventlog
```
