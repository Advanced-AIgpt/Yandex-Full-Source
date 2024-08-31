# Поэтапная раскладка релиза

Релиз заливается поэтапной раскладкой в сторы, чтобы избежать раскатки проблемной версии на всю аудиторию. Раскладка выполняется дежурным публикатором ([график дежурств](https://abc.yandex-team.ru/services/beruapps/duty/?role=2511)).

## Тикет публикации

{% include [Тикет публикации](_release_distribution/publication-ticket.md) %}

## Инструкция для дежурного публикатора

1. Опубликовать новую версию на 10% в Google Play Market (apk уже загружен и ждет публикации)
2. Нажать в тикете публикации кнопку **Опубликовал**
3. Следить за отчетами в telegram или тикете публикации (пристально следить за крешфри)
4. При достижении аудитории в несколько тысяч активных пользователей с допустимым крешфри проверить отзывы в Google Play Market
5. Если с отзывами все хорошо, опубликовать версию на 25%
6. Следить за отчетами в telegram или тикете публикации (пристально следить за крешфри)
7. При достижении аудитории в 20.000 активных пользователей с допустимым крешфри проверить отзывы в Google Play Market
8. Если с отзывами все хорошо, опубликовать версию на 100%
9. Опубликовать версию в Huawei Apps Gallery через кубик в релизном пайплайне
10. Нажать в тикете публикации кнопку **Раскатил на 100%**

### Если что-то идет не так

1. Приостанавливаем раскатку в Google Play Market
2. Проверяем, есть ли проблема (можно попросить помощи у дежурного разработчика)
3. Если проблема подтверждена,
просим дежурного разработчика и QA запустить хотфикс и нажимает в релизном тикете кнопку **Остановил раскатку**
4. Иначе продолжаем раскатку в Google Play Market

## Google Play Market (основная площадка)

День | Раскатка | Реальная аудитория (% от DAU)
:--- | ---: | ---:
1 | 5% | 0%
2 | 10% | 2,5%
3 | 100% | 5%
4 | 100% | 50%
5 | 100% | 75%

### На случай возникновения проблем

[Ссылка на заявку](https://support.google.com/googleplay/android-developer/contact/publishing)
Фио указываем свое
Email - свой, в котором авторизуетесь в гугл плей
Developer Name - Yandex Apps
Developer Account Id - 9141303443900639327
App Name - Беру
App Package Name - ru.beru.android
Выбрать пункт "I submitted a new app or app update but it is not live" или другой подходящий

Текст рыба письма:
```
We published new version (2.33.2029) of application to 5% and to 100% for beta and alpha tester 21 april 2020. Today  (22 april 2020) we could not see on our metrics this version. And also we found only 2.32.2024 version on Google Play.

We observed the similar issue in previous versions.

Please help with this situation how to publish new version of application.
```

## Huawei Apps Gallery

### Автоматический деплой

1. Дождаться раскатки версии в Google Play Market на 100%
2. Запустить шаг в пайплайне релиза
3. Удостовериться, что шаг успешно завершен. Иначе приступить к ручному деплою (см. ниже)

### Ручной деплой

#### Способ 1. Получить доступ к общему аккаунту

1. Подписываемся на yahuaweistore@yandex-team.ru
2. Открываем [https://developer.Huawei.com/consumer/ru](https://developer.Huawei.com/consumer/ru)
3. Логинимся [https://yav.yandex-team.ru/secret/sec-01e2qtbnvjpzw2qhtyntwjdrg1](https://yav.yandex-team.ru/secret/sec-01e2qtbnvjpzw2qhtyntwjdrg1)
4. Вводим код авторизации полученный на рассылку yahuaweistore@yandex-team.ru
5. Качаем новую сборку к себе на комп. (Пайплайн -> Джоба "Сборка" -> Teamcity -> Artifacts -> base -> release -> market-base-arm64-v8a-release-signed.apk)
6. Жмём Console -> Huawei Apps Gallery -> My Apps -> Беру -> Version/Upgrade -> Manage Packages -> Upload
7. Выбираем скачанную сборку. Apps Gallery иногда тупит и не показывает прогресс загрузки. Надо закрыть окно загрузки попробовать ещё раз
8. Выбираем Release type "Full release"
9. Выбираем Release schedule "Upon approval"
10. Копируем release notes в поле
11. Submit -> Not now -> Ok.
12. Если всё сделано правильно, то слева в меню появляется новая версия со статусом "Under review"
13. В случае проблем идти к Карина Надеева

#### Способ 2. Собственный аккаунт

1. Регистрируемся [https://developer.Huawei.com/consumer/ru](https://developer.Huawei.com/consumer/ru) на свою почту @yandex-team.ru (при регистрации указать страну Швейцария)
2. Далее попросить Юрия Гришина (либо любого другого у кого есть доступ в huawei) чтобы добавил твой аккаунт в корпоративный
3. Далее все как в Способе 1 с пункта 5
