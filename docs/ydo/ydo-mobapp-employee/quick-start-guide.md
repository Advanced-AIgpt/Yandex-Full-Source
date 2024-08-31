# Как запускать

### Требования
1. node@12.18.1
2. npm@6.14.5

### Быстрый старт
1. Установить и настроить нужные версии node и npm
2. [Установить arc и счекаутить репозиторий](https://doc.yandex-team.ru/arc/setup/arc/install.html)
3. Перейти в монорепу `arcadia/frontend/`
4. Выполнить команду `npm ci`
5. Перейти в директорию сервиса `arcadia/frontend/services/ydo-mobapp-employee`
6. Выполнить команду `npm ci`
7. `npm build`
8. `npm start`
9. Сервер разработки доступен по адресу `https://local.yandex.ru:3443/`. При открытии в браузере необходимо перейти в режим мобильного отображения.

## Установка зависимостей
* `npm ci`
* `npm run deps` - быстрая установка зависимостей

## Сборка
`npm build`

## Запуск приложения
- `npm run start` и `npm run start:public` (только тач)
- `npm run start:all` и `npm run start:all:public` (обе платформы)

## Запуск Storybook
- `npm run storybook:start` запуск локального StoryBook

## Запуск Hermione
- Для тестов, использующих StoryBook, перед запуском Гермионы необходимо его собрать `npm run storybook:build`
- `npm run hermione:gui` запуск Гермионы с графическим интерфейсом