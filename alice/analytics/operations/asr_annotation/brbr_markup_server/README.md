# Разметка сложных срезов с контекстом

## Поднятие сервера

Перед поднятием сервера в папке `server` нужно создать папки `markup` и `backup`: \
`mkdir server/markup server/backup`

Готовим html-ки: \
`cd prepare_htmls` \
`ya make -r` \
`./prepare_htmls //home/voice/truepg/brbr/grep_context/context_mappings //home/voice/truepg/brbr/grep_context/parsed_wonderlogs b`

Cобираем индекс html: \
`cd ../server` \
`find ../prepare_htmls/b -name "*.html" > brbr_ids`

Поднимаем сервер такой командой: \
`ya make ya.make` \
`./server --port 8182 --ipv6 brbr_ids`

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
        "result": "brbr",
        "comment": [
            "Название исполнителя на английском не разобрать."
        ]
    }
}
```
В папке `backup` будет содержаться история изменений этих json-файлов. 

