# Расчёт арабского WER

Скрипт возьмёт из файла in1.json тексты 1 и 2 и посчитает на них WER (доля ошибочных слов):
* wer raw - на сырых текстах
* wer normalized - на текстах, прогнанных через нормализатор
* wer patched - на пропатченных текстах с рекомендациями от Веры и некоторыми хаками

---

Без аркадийной сборки

требуются pip модули:
```
pylev
num2words
tashaphyne
```

Запускать:
```
make
```

Скопипасчен код:
* [расчёта метрик WER](https://a.yandex-team.ru/arc/trunk/arcadia/alice/analytics/wer/metrics_counter.py?rev=r9114337#L451)
* [нормализации арабского](https://a.yandex-team.ru/arc/trunk/arcadia/voicetech/asr/tools/arabic/normalizer/normalizer.py?rev=r9010334#L35)
