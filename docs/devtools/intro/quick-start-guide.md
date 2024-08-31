# Быстрый старт

Если вы только начинаете работать, данный раздел поможет быстро установить все инструменты, которые понадобятся вам в повседневной разработке, и загрузить исходные коды из репозитория. Нам потребуется установить несколько инструментов:

* **Arc** - система контроля версий Аркадии, хранящая данные в облаке и использующая виртуализацию рабочей копии.
* **Утилита ya** - основной консольный инструмент для сборки, тестирования и пакетирования исходных кодов.
* **Плагин для IDE** (*необязательно*) - добавляет поддержку инструментов разработки Яндекса в вашу IDE. Плагины существуют для [Visual Studio Code](https://en.wikipedia.org/wiki/Visual_Studio_Code) и IDE от [JetBrains](https://www.jetbrains.com/): Intellij IDEA, PyCharm, CLion, GoLand, WebStorm и т.п.

## Системные требования {#system-requirements}

Требования в зависимости от используемой ОС:

{% list tabs %}

- MacOS

    Установка утилит осуществляется через [Homebrew](https://brew.sh/), поэтому убедитесь, что он установлен и обновлен:

    ```bash
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    brew update
    ```

- Ubuntu

    В систему должны быть добавлены common-репозитории с deb-пакетами Яндекса. Добавить можно так:

    1. Допишите в `/etc/apt/sources.list.d/yandex.list`:
        ```
        deb http://common.dist.yandex.ru/common stable/all/
        deb http://common.dist.yandex.ru/common stable/amd64/
        ```

    2. Установите GPG-ключи:
        ```bash
        sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A
        sudo apt update
        sudo apt -y install yandex-archive-keyring
        ```

    {% note info "Доступ к репозиториям dist.yandex.ru" %}

    Для доступа к репозиториям у вас должна быть одна из следующих ролей в любом [ABC](https://abc.yandex-team.ru/) сервисе: "Разработчик", "Администратор", "Аналитик" или "Тестировщик".
    Если вы не знаете, что это - обратитесь к своему руководителю.

    *Документация: [Dist](https://wiki.yandex-team.ru/runtime-cloud/services/dist/#polzovatelju)*

    {% endnote %}

    {% note info "WSL1" %}

    Арк не работает на WSL1. Установите арк нативно или используйте WSL2. 

    {% endnote %}


- Windows

    - Требуется Windows 10 версии не ниже 1809.  Версию можно посмотреть командой `winver`. В системе должен быть установлен PowerShell.

    - Arc не работает под WSL1. Под WSL2 обратитесь к инструкции для Linux. 

    - Убедитесь, что в системе включена [поддержка длинных путей](https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd#enable-long-paths-in-windows-10-version-1607-and-later). В PowerShell, запущенном с административными привилегиями:
        ```
        New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
        ```

        Потребуется перезагрузить компьютер.

{% endlist %}

Для работы Arc и утилиты ya требуется наличие [Python](https://www.python.org/) 2 или 3 в системе.

Используйте пакетный менеджер соответствующей платформы для его установки:

{% list tabs %}

- MacOS

    ```bash
    brew install python
    ```

- Ubuntu (20.04 Focal и новее)

    ```bash
    sudo apt update
    sudo apt -y install python3 python-is-python3
    ```

- Ubuntu (18.04 Bionic и старее)

    ```bash
    sudo apt update
    sudo apt -y install python
    ```

- Windows

    Под Windows мы рекомендуем скачать и установить дистрибутив с [официального сайта](https://www.python.org/downloads/).

{% endlist %}


## Установка Arc {#arc-setup}

Установите Arc-клиент:

{% list tabs %}

- MacOS

    1. Установите MacFUSE:
        ```bash
        brew install --cask macfuse
        ```

    2. Добавьте Homebrew-репозиторий:
        ```bash
        brew tap yandex/arc https://arc-vcs.yandex-team.ru/homebrew-tap
        ```

    3. Установите Arc:
        ```bash
        brew install arc-launcher
        arc --update
        ```

    4. Включите автоматическое обновление:
        ```bash
        brew services start arc-launcher
        ```

        {% note info %}

            Если столкнулись с ошибкой:
            Error: Failure while executing; `/bin/launchctl bootstrap gui/648001662 /Users/{you}/Library/LaunchAgents/homebrew.mxcl.arc-launcher.plist` exited with 5.

            фиксится рестартом сервиса
            https://st.yandex-team.ru/DEVTOOLSSUPPORT-11606#6130b5b1422c47459c35c5a6

            Сначала стоппим:
            brew services stop yandex/arc/arc-launcher

            Затем запускаем сервис:
            brew services start arc-launcher
        {% endnote %}

    5. Если у вас Macbook с Apple Silicon, выполните дополнительные действия под катом.

       {% cut "Разрешение использования сторонних расширений ядра" %}

          1. Выключите компьютер.
		  1. Нажмите кнопку **Power** и продолжайте удерживать ее нажатой.
          1. Выберите вариант **Options** и нажмите **Далее**.
          1. В загрузившейся Recovery OS авторизуйтесь доменным паролем.
          1. В верхнем меню выберите **Утилиты/Utilites** -> **Утилита безопасной загрузки/Startup Security Utility**.
          1. Выберите загрузочный диск и нажмите **Политика безопасности/Security Policy**.
          1. Выберите **Сниженный уровень безопасности/Reduced Security** и отметьте галочками пункты **Разрешить пользователям управлять расширениями ядра от подтвержденных разработчиков/Allow user   management of kernel extensions from identified developers** и **Разрешить удалённое управление расширениями ядра/Allow remote management of kernel extensions and automatic update**, нажмите **OK**.
          1. В появившемся окне авторизуйтесь доменными данными.
          1. Выполните перезагрузку компьютера.

       {% endcut %}				  

- Ubuntu

    1. Установите Arc:
        ```bash
        sudo apt update
        sudo apt -y install yandex-arc-launcher
        ```

    2. Включите автоматическое обновление (работает на Ubuntu 16.04 Xenial и выше):
        ```bash
        systemctl --user enable --now arc-update.timer
        ```

- Windows

    1. Включите [ProjFS](https://docs.microsoft.com/en-us/windows/desktop/projfs/enabling-windows-projected-file-system).

        Для этого нужно выполнить в PowerShell, запущенном с административными привилегиями:
        ```
        Enable-WindowsOptionalFeature -Online -FeatureName Client-ProjFS -NoRestart
        ```

        При наличии в системе антивируса Касперского, выполните также:
        ```
        Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\PrjFlt\Instances\PrjFlt Instance" -Name "Altitude" -Value 324000

        ```

        Перезагрузите компьютер.

    2. Скачайте Arc [по ссылке](https://get.arc-vcs.yandex.net/launcher/windows)

    3. Добавьте средствами системы в переменную окружения PATH директорию с загруженным исполняемым файлом

    {% note warning %}

    На Windows недоступно автоматическое обновление клиента. Для ручного обновления нужно запустить:
    ```bash
    arc --update
    ```
    Рекомендуется обновлять Arc не реже, чем раз в неделю.

    {% endnote %}

- Другие дистрибутивы Linux

    Arc запускается кодом на Python, поэтому на любом дистрибутиве вы можете скачать и выполнить [скрипт arc](https://get.arc-vcs.yandex.net/launcher/linux).

    {% note info "WSL1" %}

    Арк не работает на WSL1. Установите арк нативно на Windows или используйте WSL2. 

    {% endnote %}

{% endlist %}

Настройте доступ к серверам Arc. Arc использует [OAuth](https://wiki.yandex-team.ru/oauth) для аутентификации пользователей.

Если у вас на устройстве [настроен SSH](https://wiki.yandex-team.ru/security/ssh/#instrukciiponastrojjkessh-klientov) (т.е. сгенерирован и выложен на Staff SSH-ключ), то для получения и сохранения OAuth-токена достаточно выполнить:

```bash
ssh-add && arc token store
```

{% cut "Если вы используете Yubikey и skotty" %}

После запуска команды `arc token store` нужно физически пальцем дотронуться до Yubikey, чтобы разблокировать доступ к ключу. 

{% endcut %}

{% cut "Если SSH не настроен или arc token store не работает" %}

Настроить SSH можно по [инструкции](https://wiki.yandex-team.ru/security/ssh)

Кроме того, получить токен и сконфигурировать Arc можно вручную:

1. Зайдите по [ссылке](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5c407aafc5c242948b532842a9a07da6) и нажмите "Разрешить".
2. Скопируйте полученный токен в файл `~/.arc/token` (`%USERPROFILE%\.arc\token`):

{% list tabs %}

- MacOS, Linux

    ```bash
    mkdir -p ~/.arc
    chmod 700 ~/.arc
    echo -n AQAD-... > ~/.arc/token
    chmod 600 ~/.arc/token
    ```

- Windows

    Выполните команды из обычной консоли cmd.exe, а не из оболочки Powershell:

    ```
    mkdir %USERPROFILE%\.arc
    echo AQAD-... > %USERPROFILE%\.arc\token
    ```

{% endlist %}

{% endcut %}


## Получение исходных кодов {#mount}

Единый репозиторий с исходными кодами называется Arcadia. Для работы с исходными кодами Arc использует специальную виртуальную файловую систему, поэтому доступ к исходному коду обеспечивается не через выкачивание, а через `монтирование` (`mount`).

{% list tabs %}

- MacOS

    ```bash
    arc mount ~/arcadia
    cd ~/arcadia
    ```

    {% note info "Первое монтирование" %}

    При первой попытке монтирования со свежеустановленным или обновленным MacFUSE монтирование не произойдет, система покажет уведомление о блокировке расширения ядра и попросит выдать ему разрешение. К такой же ситуации может привести обновление системы.

    Выдать необходимое разрешение можно в системных настройках: **System Preferences → Security & Privacy → General**.
    После этого нужно будет перезагрузиться, а затем попробовать примонтироваться еще раз.

    *Подробнее про использование сторонних расширений ядра MacOS можно почитать на портале [Apple Developer](https://developer.apple.com/library/content/technotes/tn2459/_index.html).*

    {% endnote %}

- Linux

    ```bash
    arc mount ~/arcadia
    cd ~/arcadia
    ```

- Windows

    ```bash
    cd %USERPROFILE%
    arc mount C:\arcadia
    cd arcadia
    ```

{% endlist %}

Монтирование приводит к созданию рабочей копии репозитория, работа которой обслуживается отдельным процессом. Поэтому при перезагрузке компьютера необходимо выполнить монтирование снова, использовав те же пути, что и ранее. Все данные, сохраненные в рабочей копии, после перемонтирования станут доступны вновь.

Операция, которая "отключает" рабочую копию репозитория, завершая обслуживающий ее процесс, называется `размонтированием` (`unmount`) и выполняется командой:

```bash
arc unmount arcadia
```

## Установка ya {#ya-setup}

Утилита `ya` приезжает вместе с деревом исходных кодов, поэтому вам остается добавить её исполняемый файл в PATH и проверить наличие токенов

{% list tabs %}

- MacOS, Linux

  **Вариант 1. Симлинк.**
  ```bash
  ln -s ~/arcadia/ya /usr/local/bin/ya # Предполагается, что исходные коды лежат в ~/arcadia
  ```

  **Вариант 2. Bash Alias**
  ```bash
  echo "alias ya='$HOME/arcadia/ya'" >> ~/.bashrc && source ~/.bashrc
  ```

  **Вариант 3. Добавление в PATH**
  ```bash
  echo 'PATH="$HOME/arcadia:$PATH"' >> ~/.bashrc && source ~/.bashrc
  ```
  {% note info %}

  На MacOS сборка может падать с собщением вида `<cmd line> failed with exit code -9 in <dir>`. Система иногда считает поведение инструмента, управляющего кэшем  сборочных инструментов, подозрительным, и убивает процессы сборочных команд, использующие инструменты из кэша. Если  вы столкнулись с такой проблемой, то нужно использовать ключ `--use-clonefile` при запуске сборки или генерации проектов для ide. Использование этого ключа немного замедляет сборку, и это может быть заметно на тяжёлых проектах, а проблема со сборкой проявляется не у всех пользователей MacOS, поэтому по умолчанию режим, активируемый данным ключом, не включен.

  {% endnote %}


- Windows

  Добавьте каталог с исходными кодами в PATH средствами Windows.

{% endlist %}

### Настройка прав доступа { #OAuthtoken }

Утилита ya для своей работы использует специальный токен со всеми необходимыми скоупами. Токен необходим, чтобы авторизовать обращения [ya pr](#ya-pr) в Арканум и обращения ``ya`` в [sandbox](http://sandbox.yandex-team.ru).

В норме утилита ya получает его по ssh-ключу и никакой дополнительной настройки не требуется. Чтобы проверить, что всё работает штатно, первый раз перед началом работы рекомендуется запустить команду `ya whoami`.

Утилита проверит доступы в svn (на данный момент это может требоваться лишь в некоторых случаях) и собственно работу с токеном для похода в Sandbox.

{% note tip %}

Если вы используете утилиты `screen` или `tmux` без специальной настройки обмен ssh-ключа на токен может не работать. В этом случае рекомендуется сохранить токен локально командой `ya whoami --save-token`. Токен будет записан в файл `~/.ya_token`.
При наличии такого файла токен всегда берётся из него.

{% endnote %}

Если по каким-то причинам обмен ssh-ключа на токен не работает, токен можно получить [в OAuth](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=f4d36b7671004ed9850148fa645acac6). Его следует положить в файл `~/.ya_token` с правами `600`.


## Установка плагина для IDE {#ide-plugin-setup}

Если вы пишете код в IDE, то рекомендуется установить плагин поддержки инструментов разработки.

### Установка плагина для Visual Studio Code {#vscode-plugin-setup}

1. Скачайте файл плагина в формате VSIX по [ссылке](https://proxy.sandbox.yandex-team.ru/last/VSCODE_PLUGIN?attrs={%22plugin_name%22:%20%22vscode-yandex-arc%22,%20%22released%22:%20%22stable%22}).
2. Установите плагин из файла, как описано в [документации](https://code.visualstudio.com/docs/editor/extension-gallery#_install-from-a-vsix).
3. Во время первого запуска плагин попросит доступ к OAuth токену. После нажатия на кнопку **"Open Browser"** откроется окно в браузере, в котором нужно дать разрешение. Если по каким-то причинам браузер не открылся, токен можно получить по [ссылке](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=f1ebd18767334ae29e664555c9a706df). Ввести токен можно при помощи команды (ctrl/cmd+shift+p) Arc: Setup OAuth Token.

Больше информации о плагине [здесь](https://a.yandex-team.ru/arc_vcs/devtools/ide/vscode-yandex-arc/README.md).

### Установка плагина для IDE от JetBrains {#jb-plugin-setup}

1. Для пользователей MacOS запустить:
  ```bash
  cd <путь до arcadia>
  ya ide fix-jb-fsnotifier
  ```
  (подробнее [тут](https://clubs.at.yandex-team.ru/arcadia/26326)).

2. В настройках IDE зайдите в **Preferences... > Appearance & Behavior > System Settings** и выключите опцию **"Back up files before saving."**

3. Зайдите в **Preferences... > Plugins** и добавьте репозиторий плагинов Яндекса: <https://a.yandex-team.ru/api/idea/updatePlugins.xml>

4. Найдите и установите плагин **Arcadia Plugin for IntelliJ**.

5. После перезагрузки IDE плагин попробует автоматически получить [OAuth](https://en.wikipedia.org/wiki/OAuth) токен для хождения в API Arcanum. Если автоматически это не удалось, то токен можно добавить руками через действие `Help -> Find Action... -> Add Arcanum token`.

6. Так же при первом старте IDE после установки плагина будет запрос на одобрение сертификата для CN *.n.yandex.net.  Этот сертификат необходим для работы некоторых внутренних интеграций плагина. Пожалуйста, примите его.

Подробная информация о плагине по [ссылке](https://a.yandex-team.ru/arc_vcs/devtools/intellij/README.md).

## Настройка проектов для IDE {#ide-project-setup}
Так как для сборки проектов в Аркадии используется самописная система сборки, с которой не знакомы используемые IDE, существует семейство утилит **ya ide**, упрощающих первичную настройку и генерацию проекта для основных используемых IDE, а также сборки ViM и Emacs, адаптированные для работы в Arcadia.
**Поддерживаемые IDE:**
- [CLion](https://docs.yandex-team.ru/ya-make/usage/ya_ide/clion)
- [Emacs](https://docs.yandex-team.ru/ya-make/usage/ya_ide/emacs)
- [GoLand](https://docs.yandex-team.ru/ya-make/usage/ya_ide/goland)
- [IDEA](https://docs.yandex-team.ru/ya-make/usage/ya_ide/idea)
- [MSVS](https://docs.yandex-team.ru/ya-make/usage/ya_ide/msvs)
- [PyCharm](https://docs.yandex-team.ru/ya-make/usage/ya_ide/pycharm)
- [QtCreator](https://docs.yandex-team.ru/ya-make/usage/ya_ide/qt)
- [ViM](https://docs.yandex-team.ru/ya-make/usage/ya_ide/vim)
- [VS Code](https://docs.yandex-team.ru/ya-make/usage/ya_ide/vscode)

