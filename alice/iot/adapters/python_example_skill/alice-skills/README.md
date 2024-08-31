# Примеры навыков Алисы

Чтобы вам было проще писать и запускать свои навыки, мы написали примеры и эту инструкцию.

### «Купи слона» на Python
Предлагает пользователю купить слона, пока он не согласится. Для создания веб-приложения используется Flask. 

[Исходный код]()
Код обработки запроса навыка на Python находится в файле `_имя папки_/elephant/elephant.py`.

### Попугай на node.js
Отвечает пользователю его же словами. Для запуска асинхронного сервиса используется пакет micro. 

[Исходный код]()
Код обработки запроса навыка на Node.js находится в файле `_имя папки_/parrot/index.js`, код конфигурации — в файле `_имя папки_/parrot/package.json`.

### Навык умного дома на Python
Пример провайдера с лампочками, розеткой и кондиционером. Навык реализован с допущением, что существует только один пользователь *alone_user*\*, отвечает на его запросы и отображает текущее состояние устройств провайдера на сайте. Для создания веб-приложения используется Flask и gunicorn, для интерфейса — css и react.js. 

[Исходный код]()
Код обработки запросов навыка умного дома на Python (`_имя папки_/smart_home`) находится в файле `_имя папки_/smart_home/smart_home_api.py`, код конфигурации — в файле `_имя папки_/smart_home/config.py`.

\*если вы хотите поддерживать большее количество пользователей, то используйте базу данных и настройте OAuth2 авторизацию. Подробнее про OAuth2 авторизацию и связку аккаунтов можно почитать в нашей документации: https://yandex.ru/dev/dialogs/alice/doc/auth/about-account-linking-docpage/

# Запуск всех навыков
Мы задокерезировали все навыки и для запуска всех приложений вместе используется docker-compose и Nginx.

## Подготовьте доменное имя
Доменное имя необходимо, чтобы зарегистрировать навык в Яндекс.Диалогах. Оно будет указывать на IP-адрес вашей виртуальной машины. 

## Создайте Виртуальную Машину
Вы можете создать Виртуальную Машину (compute cloud, virtual machine, instance) в любом сервисе, например, в [Яндекс.Облаке](https://cloud.yandex.ru/docs/compute/quickstart/quick-create-linux), [Amazon Lightsail](https://lightsail.aws.amazon.com/ls/docs/en_us/articles/how-to-create-amazon-lightsail-instance-virtual-private-server-vps), [Azure](https://docs.microsoft.com/en-us/azure/virtual-machines/linux/quick-create-portal) или [Google Cloud](https://cloud.google.com/compute/docs/instances/create-start-instance). При первой регистрации везде даются пробные периоды.

Рекомендуется создавать Виртуальную Машину **Linux**. Количество ресурсов вы можете выбрать любое, исходя из прогнозируемой нагрузки, но для функционального тестирования навыков хватит такой конфигурации:
* Гарантированная доля vCPU — 5%
* vCPU — 1
* RAM — 1.5 ГБ

Убедитесь, что у вашей Виртуальной Машины статический IP-адрес.

## Зайдите на Виртуальную Машину
При подключении к Виртуальной Машине по SSH ваша команда будет выглядеть примерно так:
`ssh -i /path/to/private/key login@12.34.56.78`, где
- `/path/to/private/key` — путь до вашего приватного ключа,
- `login` — имя пользователя (username) на вашей Виртуальной Машине,
- `12.34.56.78` — публичный IP-адрес вашей Виртуальной Машины.
 
Более подробно и другие способы тут:
- [Яндекс.Облако](https://cloud.yandex.ru/docs/compute/operations/vm-connect/ssh),
- [Amazon Lightsail](https://lightsail.aws.amazon.com/ls/docs/en_us/articles/lightsail-how-to-connect-to-your-instance-virtual-private-server),
- [Amazon Lightsail с PuTTY](https://lightsail.aws.amazon.com/ls/docs/en_us/articles/lightsail-how-to-set-up-putty-to-connect-using-ssh), 
- [Azure](https://docs.microsoft.com/en-us/azure/virtual-machines/linux/mac-create-ssh-keys),
- [Google Cloud](https://cloud.google.com/compute/docs/instances/connecting-to-instance),
- [Google Cloud](https://cloud.google.com/compute/docs/instances/connecting-advanced)

## Настройте Виртуальную Машину
После того, как вы зашли на Виртуальную Машину, обновите пакеты и зависимости, установите **docker** и **docker-compose** с помощью следующих команд:

```shell
sudo apt-get update
sudo apt install docker.io
sudo apt install docker-compose
sudo docker-compose build
sudo docker-compose up -d
```

В случае, если в вашем дистрибутиве отсутствуют данные пакеты, воспользуйтесь инструкциями на сайте:
https://docs.docker.com/engine/install/
https://docs.docker.com/compose/install/

## Скачайте и измените код навыков
Чтобы развернуть навыки, скачайте или склонируйте код [репозитория на GitHub]():
`git clone ...`
У вас должна получиться папка _имя папки_. 

Измените код чтобы он работал для вашего домена, для этого:
* Отредактируйте `nginx/nginx.conf`. В директиве `server_name` укажите имя вашего домена вместо example.ru.
* В `front/src/App.js` в константе `DEVICE_LIST_API_URL` измените example.ru на адрес вашего сайта.
* Добавте ssl сертификаты и приватный ключ в `nginx/ssl` в файлы _certificate.crt_ и _key.key_ соответсвенно. Для создания SSL сертификатов можете, например, использовать [cerbot](https://certbot.eff.org)
* Правильно настройте DNS:
    * Перейдите в список записей вашего домена.
    * Создайте или отредактируйте записи так, чтобы у домена было две A-записи с публичным IP-адресом виртуальной машины: для поддоменов «@» и «www».

Чтобы редактировать файлы из консоли, вы можете использовать команду `sudo nano /path/to/file`

## Запустите навыки
Находясь в папке _имя папки_, соберите образы навыков командой 
`sudo docker-compose build`
и запустите с помощью 
`sudo docker-compose up -d`

## Протестируйте
Теперь вы можете проверить работу навыков в [Яндекс.Диалогах](https://dialogs.yandex.ru/developer/).

Если вы хотите проверить навык «Купи слона», то в поле **Webhook URL** укажите https://example.ru/elephant, если Попугая — https://example.ru/parrot, если навык умного дома — https://example.ru/v1.0, нажмите кнопку **Сохранить** и перейдите на вкладку **Тестирование**.
