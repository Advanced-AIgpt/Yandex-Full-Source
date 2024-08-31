# Определение запросов от роботов

В XScript реализована проверка того, является ли зашедший на страницу пользователь роботом.

Считается, что запрос сделан роботом, если HTTP-заголовок `Accept` пустой, и/или HTTP-заголовок `User-agent` содержит подстроку из заданного [списка](http://wiki.yandex-team.ru/passport/MDAforBOTs). Список подстрок `User-agent`, принадлежащих Роботам, распространяется в пакетах [xscript-botlist](packages.md#xscript-botlist) и [xscript-multiple-botlist](packages.md#xscript-multiple-botlist) и подключается к [конфигурационному файлу](../appendices/config-params.md#bot) XScript.

В случае, если запрос пришел от робота, не выполняется парсинг куки Session_id, обращение к Черному Ящику и перенаправление пользователя на Яндекс.Паспорт.

Верстальщик может получить информацию о том, был ли запрос сделан роботом, с помощью переменной [bot](../appendices/protocol-arg.md#bot) из ProtocolArg.

