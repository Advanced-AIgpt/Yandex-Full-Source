### CIT tests (интеграционные тесты)

Запуск с поднятием/использованием локального uniproxy
```
$ py.test-3 pytest_uniclient.py
```

Запуск со стрельбой по удалённому uniproxy (testing)
```
$ py.test-3 -v pytest_qloud.py
```
или по production
```
$ py.test-3 -v pytest_qloud.py --qloud-env=stable
```


### Мониторинг тест (для тестирования функционала uniproxy+backends извне yandex-а: VOICESERV-1450)

Минимальный набор файлов для теста состоит из:
```
monitoring_test.py
uniclient.py
monitoring_vins_voice_session.json
monitoring_vins_text_session.json
```
\+ нужны патроны для теста (+ требуется python3 & tornado для запуска)

Минимальный пример патронов лежит в
```
$ cat monitoring_ammo_example.jsonl
{"name": "test school", "voice": "data/school.opus"}
{"name": "test composition name", "voice": "data/kuklakolduna.opus"}
{"name": "?", "text": "сколько время в москве"}
```
Поля
* name - описание патрона
* voice - путь до аудиофайла с речью (тест VINS.VoiceInput)
* text - текст с тестом (тест VINS.TextInput)
* normalized_recognition - строка(или список строк-версий) ожидаемые в результате ASR
* min_tts_size - минимальный полученный размер TTS данных (если получили меньше тест считается проваленным)
* max_tts_size - максимальный полученный размер TTS данных (если получили больше тест считается проваленным)
Пример запуска теста
```
$ python3 monitoring_test.py --test-ammo=monitoring_ammo_example.jsonl
```
(результаты пишуться в monitoring_report.json)

Более насыщенные (с большим кол-вом проверок) патроны можно нагенерировать из минимального комплекта
(используя данные полученные от uniproxy):
```
$ python3 monitoring_test.py --test-ammo=monitoring_ammo_example.jsonl --enriched-ammo=monitoring_ammo_example_.jsonl
...
$ cat monitoring_ammo_example_.jsonl
{"max_tts_size": 16113, "min_tts_size": 11910, "name": "test school", "normalized_recognition": "в нашей школе есть очень много спортивных секций", "voice": "data/school.opus"}
{"max_tts_size": 16113, "min_tts_size": 11910, "name": "test composition name", "normalized_recognition": "кукла колдуна", "voice": "data/kuklakolduna.opus"}
{"name": "?", "text": "сколько время в москве"}
```
(при _обогащении_ патронов ASR результаты не совпадающие с имеющимися добавляюся в список допустимых/ожидаемых,
диапазон ожидаемого размера TTS данных при необходимости расширяется, - это позволяет по быстрому подогнать патроны
если есть уверенность что fail-ы теста вызваны не ошибками на стороне uniproxy и его сервисов)

Формат рапорта с результатами прогона тестов можно посмотреть прогнав тест по примеру патронов с ошибками:
`$ python3 monitoring_test.py --test-ammo=monitoring_ammo_example_fails.jsonl`
```
$ cat monitoring_report.json
{
    "failed_sessions": [
        {
            "errors": [
                "bad norm. recognition for utt[0]: expected \"в нашей школе было очень много спортивных секций\" != \"в нашей школе есть очень много спортивных секций\""
            ],
            "name": "проверяем обнаружение несовпадения результата ASR",
            "sess_num": 0
        },
        {
            "errors": [
                "input stream data size to large"
            ],
            "name": "проверяем обнаружение превышения максимального размера TTS",
            "sess_num": 1
        },
        {
            "errors": [
                "input stream data size to small"
            ],
            "name": "fail on min size",
            "sess_num": 2
        }
    ],
    "sessions_processed": 3
}
```
Если ошибок не было, рапорт довольно лаконичен
```
$ cat monitoring_report.json
{
    "failed_sessions": [],
    "sessions_processed": 3
}
```

### Стресс тест (vins voice_input/text_input сценариев)

запускается скриптом stress_test.py, - пример запуска со стрельбой по testing-у:
```
python3 stress_test.py --tests-limit=500 --simultaneous-sessions=50 --test-session=stress_vins_voice_input.json --sess-result=stress_out.jsonl --audio-dir=stress_audio
```
в приведённом примере прогоняется 500 тестовых сессии проверки Vins.VoiceInput ответов (поддерживается 50 одновременных сессий)
аудиофайлы для отправки будут взяты из папки stress_audio, - подразумевается что там лежат audio-файлы в opus формате (и ничего больше!)
Скрипт описывающий тестовую сессию считывается из файла stress_vins_voice_input.json

В результате прогона генерируется JSONL файл stress_out.jsonl содержащий результаты прогона (по сессии на строку) - пример:
```
$ head -1 stress_out.jsonl
{"asr_result": 0.926703691482544, "audio": "bio2.opus", "errors": [], "num": 28, "start_session": 1527250705.7602472, "start_test": 1527250705.4898324,
"test_duration": 2.2418558597564697, "total_vins_response": 1.9708244800567627, "vins_response": 1.0441169738769531}
```

По этим результам можно прогнать скрипт-анализатор что-бы получить статистическую выжимку:
```
$ python3 stress_stat.py --sess-result=stress_out.jsonl --stat-result=stress.out && cat stress.out
{
    "asr_result_q50": 5.298,
    "asr_result_q80": 7.806,
    "asr_result_q90": 8.598,
    "asr_result_q95": 9.402,
    "asr_result_q99": 19.032,
    "errors": {
        "test reach timeout limit": 20
    },
    "input_sessions": 500,
    "ok_sessions": 480,
    "perc_ok_sessions": 96.0,
    "rps": 6.44,
    "rps_ok": 6.18,
    "test_duration_q50": 6.387,
    "test_duration_q80": 9.277,
    "test_duration_q90": 10.452,
    "test_duration_q95": 13.856,
    "test_duration_q99": 24.467,
    "used_sessions": 500,
    "vins_response_q50": 1.061,
    "vins_response_q80": 1.204,
    "vins_response_q90": 1.264,
    "vins_response_q95": 1.338,
    "vins_response_q99": 1.651
}
```

Расшифровка полей со статистикой:

* errors  - фатальные ошибки при выполнении тестовой сессии
* input_sessions  - кол-во сессий во входном файле
* used_sessions  - кол-во использованных при расчёте статистики сессий
    (может быть меньше input_sessions при использовании параметра --crop-tail=N, что удалит из выборки N сессий завершившихся позже всего
     применение данного параметра должно давать более корректный расчёт RPS-а, когда отбрасывается хвост прогона тестов, когда пул одновременных сессий
     занят не на 100%)
* ok_sessions  - количество тестовых сессий завершившихся успешно
* perc_ok_sessions  - процент тестовых сессий завершившихся успешно
* rps  - requests(sessions) per second
* rps_ok  - requests(sessions) per second (расчитанный только для успешных сессий)
* asr_result_*  - квантили времени распознавания (от начала тестовой сессии до ASR.Result) - рассчитывается только для успешных сессий
* vin_response_*  - квантили времени работы Vins-а (время между Vins.Reponse и ASR.Result) - рассчитывается только для успешных сессий
* test_duration_*  - квантили общего времени выполнения теста (сюда например попадает время установления websocket соединения) - рассчитывается по всем сессиям

Запуск тестирования Vins.TextInput запросов:
```
python3 stress_test.py --tests-limit=100 --simultaneous-sessions=10 --texts=texts.txt --test-session=stress_vins_text_input.json --sess-result=stress_out.jsonl
```

texts.txt  - файл c текстовыми данными для TextInput запроса (по запросу на строку)

Расчёт статистки по результатам тестирования Vins.TextInput запросов полностью аналогичен VoiceInput (изложено выше)


Для stress стрельбы по production нужно ещё указывать url uniproxy (по умолчанию используется url & key для тестинга):
--uniproxy=wss://voiceservices.yandex.net/uni.ws --auth-token=069b6659-984b-4c5f-880e-aaedcfd84102
