# Contribute guide или как быстро внести правки в документацию

Аркадия умеет собирать Markdown с расширением [Yandex Flavored Markdown](https://github.com/yandex-cloud/yfm-transform/#yandex-flavored-markdown) при помощи `ya make` и поддерживает автоматический [деплой на общий хостинг](https://docs.yandex-team.ru/docstools/deploy)


## Подготовка

Перейдите к [документации аркадии](https://docs.yandex-team.ru/devtools/intro/quick-start-guide), утановите **arc** и скачайте исходники



## Внесение правок
Проект документации мессенджера находится по адресу:

```
~/arcadia/docs/messenger/
```

Откройте проект в любом редакторе кода и творите

Подробнее про структуру оглавления тут: [https://docs.yandex-team.ru/docstools/settings/toc](https://docs.yandex-team.ru/docstools/settings/toc) 

Примеры синтаксиса можно найти тут: [https://docs.yandex-team.ru/docstools/examples](https://docs.yandex-team.ru/docstools/examples)


## Локальная сборка
Перед коммитом и отправкой пул реквеста можно собрать проект локально выполнив команду:
```
ya make
```
После успешной сборки в корне проекта появится файл `docs-messenger.tar.gz`

Если все ок, делаем коммит и отправляем пул реквест.


## Публикация документации в тестинг/прод

Откройте проект в аркадии [https://a.yandex-team.ru/arc_vcs/docs/messenger](https://a.yandex-team.ru/arc_vcs/docs/messenger)

В правом верхнем углу будет кнопка **Manage Docs** 

![Manage Docs](https://docs.yandex-team.ru/docs-assets/docstools/2299113-9097374/ru/images/widget.png)

При нажатии на кнопку открывается виджет, который позволяет выложить разные ревизии в [тестинг](https://testing.docs.yandex-team.ru/messenger/) или [прод](https://docs.yandex-team.ru/messenger/).


![Testing Prod](https://docs.yandex-team.ru/docs-assets/docstools/2299113-9097374/ru/images/widget_open.png)