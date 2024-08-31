Утилита для склеивания нескольких YT-таблиц в одну.
В первую очередь предназначена для маленьких таблиц с резолюциями модераторов, для больших таблиц нужно будет дописать запуск склеивания на YT.

# Пример запуска

```
ya make
export YT_TOKEN=...
java -cp yt_merger.jar ru.yandex.alice.paskills.yt_merger.YtMerger --input-dir //home/paskills/moderation/stable/chat_responses --output-table //home/paskills/moderation/stable/chat_responses_archive/2020-11-30 --do-not-delete-input --force --max-table-name 2019-10-07T20
```

Полный список аргументов доступен по `--help`.
