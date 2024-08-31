# Алиса в бизнесе - API бизнес-логики

[![Build Status](https://drone.yandex-team.ru/api/badges/paskills/alice-in-business-api/status.svg)](https://drone.yandex-team.ru/paskills/alice-in-business-api)

### Основной стек

-   `TypeScript`
-   `PostgreSQL`
-   `Express`
-   `Sequlize`
-   `AVA` и `Supertest` для тестов

### Как поднять приложение локально
!!! На новых версиях Node.js какие-то шаги могут сломаться. Стоит установить версию `12.22.12`.

1. **Подготовка**
    1. Файл `/.env.example` → `/.env`
    2. Файл `/.tvm.dev.json.example` → `/.tvm.dev.json` дописываем [секрет](https://yav.yandex-team.ru/secret/sec-01dkew7k957nhbrvnthve0z1qw/explore/version/ver-01dkew7k9dd8esy85ve7zyvs9y) вместо ХХХ и [TVM_2018147](https://yav.yandex-team.ru/secret/sec-01em7526hpc3z21b4t126kgfg9/explore/version/ver-01f6pj0w74jmffcbstzynasjka) вместо YYY
    3. Выполняем подготовку для установки [пакета генерации PDF](https://github.com/tpisto/pdf-fill-form#installation)
    4. Устанавливаем пакеты и собираем TS
        ```bash
        npm i
        npm run build
        ```

2. **Разворачиваем окружение**

    1. Поднимаем Docker c postgresql
        ```bash
        brew install postgresql # если нет
        npm run dev:db:run
        ```
       > Если вы ранее не скачивали образ dbaas/minipgaas, [запросите](https://wiki.yandex-team.ru/docker-registry/#upravlenieroljami) viewer права на него.
    2. Накатываем Миграции
        ```bash
        npx sequelize db:migrate
        # npx sequelize db:migrate:undo откатывает последнюю
        ```
    3. Запускаем TVM-демон
        ```bash
        npm run dev:tvmtool
        ```
    4. Наливаем данные (для тестов это не требуется)
        ```bash
        npx sequelize db:seed:all
        ```

3. **Запускаем приложение**

    1. Следим за файлами, пересобираем по необходимости
        ```bash
        npm run dev:build
        ```
    2. Запуск приложения (API) на 7080 порту ([про magicdev](https://github.yandex-team.ru/toolbox/magicdev))
        ```bash
        npm run dev
        ```

### MiniPGaaS + DataGrip (JetBrains Database plugin)

1. `echo '127.0.0.1 minipgaas.localhost' > /etc/hosts`

2. Запускаем `npm run dev:db:run`. В папке `.minigpaas/ca` появится файл `ca.crt`

3. Настраиваем DataGrip:
    - `host=minipgaas.localhost port=12000 password=nacc6opq dbname=database user=minipgaas`
    - SSL CA: `.minigpaas/ca/ca.crt`

### Docker

Собирает образ под тегом текущего пользователя и пушит его

```bash
npm run docker # npm run docker:build && npm run docker:push
```

### Тесты

1. Готовим окружение (см. Как поднять локально > Разворачиваем окружение)

2. Запуск тестов (конфиг для тестов лежит в `/ava.config.js`)

    ```bash
    npm run test
    ```

### CI, Деплой

Конфиг для Drone лежит в `/.drone.yml`

-   На `git push ...` — запускает тесты
-   На `git push dev` — запускает тесты, собирает и пушит образ с тегом dev
-   На `npm version patch` — собирает версию, пушит в dev, запускает тесты, собирает и пушит образ с тегом соответствующей версии

Деплой выполняется в Qloud вручную: нужно зайти в [stable](https://qloud-ext.yandex-team.ru/projects/voice-int/paskills-int/stable) или [priemka](https://qloud-ext.yandex-team.ru/projects/voice-ext/paskills-int/stable), и обновить до актуальной версии

### CLI

Для запуска CLI на машинке

```bash
ssh ...
cd app
node cli
```

Команды

-   `users:create` — по логину создает пользователя, можно привязать к организации

-   `promocodes:add` — промокоды можно добавить через пробел

-   `organizations:bind` — привязывает пользователей к организациям

-   `organizations:create`

-   `devices:create`

-   `device:change`
