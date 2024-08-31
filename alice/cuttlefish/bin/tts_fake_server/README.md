Пример реализации tts-server-а на основе alice/cuttlefish/library/tts/backend/base библиотеки.

Изначально библиотека включает в себя только часть про приём/отправку apphost сообщений, вместо реального распознавания в ней лежит пример в виде fake-реализации интерфейса, соот-но сервер тоже получается fake

Для конвертации бинарного лога (см. server/log/eventlog в читаемый формат использовать arcadia/voicetech/tools/evlogdump

Пример:

```
(cd ../../../../voicetech/tools/evlogdump && ya make) && ../../../../voicetech/tools/evlogdump/evlogdump eventlog
```
