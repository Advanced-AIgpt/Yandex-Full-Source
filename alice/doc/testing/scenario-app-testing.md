# Отладка в приложениях и на устройствах

Для отладки сценария можно использовать:

* Телеграм-бота [@AmandaJohnson](https://t.me/AmandaJohnsonBot), который умеет эмулировать устройства с Алисой и поисковое приложение.
* Бету поискового приложения.
* Десктопный Яндекс.Браузер для Windows.
* Колонки, часы и остальные устройства — с помощью [Космодрома](#devices).


## Флаги экспериментов {#exp-flags}
На поведение бэкенда можно влиять снаружи, выставляя флаги экспериментов. В разных приложениях это делается по разному, смотри примеры ниже.



### Для отладки еще не выпущенного в прод сценария {#new-scenario}

Чтобы включить сценарий только для тестирования, используйте один из следующих флагов:
1. `mm_enable_protocol_scenario=<имя сценария>` — включает сценарий в мегамайнде (аналог `enabled: True` в [конфиге](../megamind/config.md)), чтобы тестировать в том числе ранжирование сценария в общем поле.
2. `mm_scenario=<имя сценария>` — выключить все остальные сценарии для
   этого клиента (как будто в Алисе существует только сценарий `<имя
   сценария>`). Полезно, чтобы целенаправленно проверять ответы
   нужного сценария, не отвлекаясь на ранжирование. Этот флаг не
   рекомендуется использовать нигде кроме как при разработке.


## Настройка {#setup}

### Amanda Johnson {#amanda-johnson}

1. Добавьте свой Telegram-username в анкету на Стаффе.
2. [Начните диалог с @AmandaJohnson](https://t.me/AmandaJohnsonBot).

Чтобы включить сценарий, установите флаг эксперимента с помощью команды `/setexp mm_enable_protocol_scenario=<имя сценария>`, например `/setexp mm_enable_protocol_scenario=BestScenario`.

Подробная информация о настройке представлена на [отдельной странице Amanda Johnson](amanda.md).


### Поисковое приложение {#yandex-app}

Чтобы проверить сценарий в поисковом приложении:

1. Установите бету Поискового приложения [iOS](https://beta.m.soft.yandex.ru/description?app=bro_search&platform_shortcut=iphoneos&branch=master-canary) / [Android](https://beta.m.soft.yandex.ru/description?app=yandex&platform_shortcut=android&branch=dev).
2. Укажите адрес нужного сервера Мегамайнда в настройках:
   * **Android**: **Панель отладки → Dialog → VINS URL**
   * **iOS**: вкладка турбо-аппов → блок **Отладка** → **Панель отладки** → блок **Assistant** → **Settings** → **Vins URL**

   Укажите адрес Мегамайнда в следующем формате:
   ```
   http://vins.hamster.alice.yandex.net.speechkit/app/pa/?srcrwr=MEGAMIND_ALIAS:<адрес_dev-сервера>:<номер_порта>, номер_порта = порт + 3
   ```
   Например, если вы запустили мегамайнд на порту 7007:
   ```
   http://vins.hamster.alice.yandex.net/speechkit/app/pa/?srcrwr=MEGAMIND_ALIAS:cowsay-test.man.yp-c.yandex.net:7010
   ```

3. Укажите один из [флагов эксперимента](#new-scenario) для того чтобы всегда был активирован ваш сценарий:

   * **Android**: панель отладки -> experiments flags -> vins experiments
   * **iOS**: вкладка турбо-аппов → блок **Отладка** → **Панель отладки** → блок **Assistant** → **Settings** → **Experiment List**

   Впишите в поле `mm_enable_protocol_scenario=<имя сценария>`, например `mm_enable_protocol_scenario=ps_hackathon_1`.

4. Перезапустите Поисковое приложение.


### Десктопный Яндекс.Браузер {#desktop-ybrowser}

Только для Windows:

1. Закройте браузер с помощью **Ctrl+Shift+Q**.
1. `cd "%LOCALAPPDATA%\Yandex\YandexBrowser\Application"`
1. Запустите Браузер с двумя параметрами:
   ```
   --speechkit_vins_url="http://vins.hamster.alice.yandex.net.speechkit/app/pa/?srcrwr=MEGAMIND_ALIAS:<адрес_dev-сервера>:<номер_порта>", номер_порта = порт + 3
   ```
   и `--force-fieldtrial-params`, один из вариантов (TODO чем отличаются варианты?):
        * `--force-fieldtrial-params="als.1:vins_experiments/[\"mm_enable_protocol_scenario%3D<имя сценария в конфиге>\"]`
        * `--force-fieldtrial-params="als.1:vins_experiments/[\"mm_scenario%3D<имя сценария в конфиге>\"]`

Например, если Мегамайнд запущен на порту 7007:
```
browser.exe --speechkit_vins_url="http://vins.hamster.alice.yandex.net/speechkit/app/pa/?srcrwr=MEGAMIND_ALIAS:cowsay-test.man.yp-c.yandex.net:7010" --force-fieldtrial-params="als.1:vins_experiments/[\"mm_enable_protocol_scenario%3Dalice.cowsay\"]
```


### Колонки, часы и прочие устройства {#devices}

Тестировать сценарии и Алису в целом на колонках, часах и других несмартфонных устройствах помогает Космодром — интерфейс для управления конфигурациями и прошивками устройств.

Чтобы получить доступ в [UI Космодрома](https://quasmodrom.quasar.yandex-team.ru/admin/), нужно иметь роль IDM на управление отдельным устройством или группой устройств. О том, как выбрать нужный вариант и запросить роль, читайте на [вики Космодрома](https://wiki.yandex-team.ru/alicetesting/Quasmodrom/#kakpodkljuchitkolonkuvkosmodrom).


#### Как задать флаги экспериментов {#flags}

Флаги экспериментов для устройств те же, что и для [приложений](#exp-flags), задавать их нужно в [конфигурации устройства](https://wiki.yandex-team.ru/alicetesting/Quasmodrom/#kakpomenjatkonfigi), в массиве `experiments`.

Например, чтобы включить на устройстве эксперимент `mm_enable_protocol_scenario=<имя сценария>` (аналог `enabled: True` в [конфиге](../megamind/config.md)), нужно дописать его в массив:

```json
...
"experiments": ["mm_enable_protocol_scenario=<имя сценария>", <...>],
...
```

Чтобы убедиться, что флаги доехали до устройства вместе с конфигурацией, найдите запрос к устройству в [SETrace](../setrace/index.md ) и поищите слово «experiments» на странице запроса к Мегамайнду:

![megamind-setrace-ctrl-f](../images/experiments.png)
