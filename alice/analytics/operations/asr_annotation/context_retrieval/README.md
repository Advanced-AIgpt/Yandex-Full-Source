# Разметка sidespeech-запросов с контекстом

## Мотивация

Во время DSAT ошибок ASR в Алисе (начало 2022-го) мы выяснили, что значимая часть проблем заключатся в аннотировании. Причём наибольшее число ошибок заключается в неправильном определении того, обращён ли запрос в умное устройство или нет. К сожалению, имея только аудио, зачастую сложно определить кто адресат запроса. У нас появилась гипотеза, что информация о соседних запросах поможет улучшить качество аннотирования.

В данной папке находятся ad-hoc скрипты для сбора данных для разметки и простой HTTP-сервер для аннотации.

## Сбор данных

> **Замечание**: в YQL-запросах и командах запуска зашиты конкретные пути и даты. Для сбора своей разметки нужно их поменять.

- извлекаем нужные id из wonderlogs (чтоб не бороться с тяжёлыми строками и глубокими схемами wonderlogs в последующих запросах): `queries/extract_ids_from_wonderlogs.yql`
- извлекаем валидные id из предыдущей выжимки и сортируем для быстрого поиска: `queries/extract_valid_request_ids_from_wonderlogs.yql`
- вы должны выбрать `request_id`, которые хотите разметить (`_message_id` в терминологии wonderlogs)
- получаем для выбранных `request_id` те id, что нам понадобятся из wonderlogs: `queries/select_interesting_ids_from_wonderlogs.yql`
- построение таблички с "сессиями" - сортируем валидные строки по device-id и времени запроса: `queries/make_sessions.yql`
- выбираем только те "сессии", которые в итоге будем размечать: `queries/select_interesting_sessions.yql`
- находим соседние запросы для целевых: `./select_only_close_requests //home/voice/kolesov93/VOICE-6763/attempt3/interesting_requests //home/voice/kolesov93/VOICE-6763/attempt3/sessions //home/voice/kolesov93/VOICE-6763/attempt3/message_ids_for_requests_and_contexts //home/voice/kolesov93/VOICE-6763/attempt3/context_mappings`
- выбираем из wonderlogs запросы, которые понадобятся для разметки: `./filter_wonderlogs //home/voice/kolesov93/VOICE-6763/attempt3/message_ids_for_requests_and_contexts //home/alice/wonder/logs/2022-01-{24..30} //home/voice/kolesov93/VOICE-6763/attempt3/selected_wonderlogs`
- жмём чанки: `ya tool yt merge --mode ordered --proxy hahn --src //home/voice/kolesov93/VOICE-6763/attempt3/selected_wonderlogs --dst //home/voice/kolesov93/VOICE-6763/attempt3/selected_wonderlogs --spec '{combine_chunks=true;data_size_per_job=986488597}'`
- парсим из wonderlogs информацию, которая нам понадобится: `./parse_wonderlogs //home/voice/kolesov93/VOICE-6763/attempt3/interesting_requests //home/voice/kolesov93/VOICE-6763/attempt3/selected_wonderlogs //home/voice/kolesov93/VOICE-6763/attempt3/parsed_wonderlogs`
- готовим html-ки: `./prepare_htmls //home/voice/kolesov93/VOICE-6763/attempt3/context_mappings //home/voice/kolesov93/VOICE-6763/attempt3/parsed_wonderlogs b`
- собираем индекс html: `find /home/kolesov93/work/arcadia/voicetech/asr/experiments/VOICE-6763-sidespeech-dsat/prepare_htmls/b -name "*.html" > ids`

Какую-то информацию можно найти в тикете: https://st.yandex-team.ru/VOICE-6763

## Поднятие сервера

Перед поднятием сервера нужно создать в его рабочей директории папки `markup` и `backup`.

В папке `markup` будут содержаться json-ы с разметкой в духе:
```json
{
    "21": {
        "result": "sidespeech",
            "comment": [
                "Разговор между собой."
            ]
    },
    "22": {
        "result": "sidespeech",
        "comment": [
            "Судя по контексту, это телевизор."
        ]
    }
}
```

В папке `backup` будет содержаться история изменений этих json-файлов. 

Поднимаем сервер такой командой: `./server --port 8182 --ipv6`
