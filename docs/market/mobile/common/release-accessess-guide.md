## Перед дежурством необходимо проверить наличие доступов в соответствующие сервисы

- AppMetrica

Необходимо иметь доступы к 
**AppMetrica/Доступ к приложению сервиса Яндекса id 2780002**
**AppMetrica/Доступ к приложению сервиса Яндекса id 1389598**

{% note tip %}

Проверить наличие или запросить доступы можно через [IDM](https://idm.yandex-team.ru/)

{% endnote %}

{% cut "Пример" %}

![alt text](https://jing.yandex-team.ru/files/kurenkovap1/Pasted%20Graphic.png)

{% endcut %}

{% list tabs %}

- iOS

  - App Store Сonnect

  {% note tip %}

  Проверить есть ли доступ можно зайдя на [AppStoreConnect](https://appstoreconnect.apple.com) в раздел **users and access** нажать поиск, ввести свою фамилию. 
  Роль для работы с релизом - **AppManager**

  {% endnote %}

  {% note warning %}

  Доступ лучше запрашивать на AppleID зареганный на корповую учетку.
  
  {% endnote %}

    1. Создаем в [очереди](https://st.yandex-team.ru/APPMARKET) таск с названием типа "Выдать права в applestore".
    2. В теле пишем запрос с обоснованием

    app: Яндекс.Маркет: здесь покупают
    appleId: **Ваш apple id**
    роль: Менеджер приложения/app manager

    тикет должен сам заассайниться на нужного исполнителя, либо можно самому повесить на **@energen**

    [пример тикета](https://st.yandex-team.ru/APPMARKET-1535)

- Android

  В разработке

{% endlist %}