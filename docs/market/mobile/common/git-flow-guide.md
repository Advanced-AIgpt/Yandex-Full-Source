# Как мы работаем с VCS

После переезда в Аркадию мы работаем по принципу Trunk-Based Development или TBD. [По ссылке](https://trunkbaseddevelopment.com) ты найдёшь подробную информацию о TDB.

Расписание релизного поезда не изменилось, прочитай о нём в [статье от Димы Полякова](https://habr.com/ru/company/yandex/blog/504752/).

## Начало работы {#environment-setup}

Для начала ознакомься с разделом [Быстрый старт](https://docs.yandex-team.ru/devtools/intro/quick-start-guide), из которого ты узнаешь:

* О [системных требованиях](https://docs.yandex-team.ru/devtools/intro/quick-start-guide#system-requirements) для работы с Аркадией
* Как [установить Arc-клиент](https://docs.yandex-team.ru/devtools/intro/quick-start-guide#arc-setup)
* Как [смонтировать репозиторий](https://docs.yandex-team.ru/devtools/intro/quick-start-guide#mount) для доступа к исходным кодам
* Как [установить плагины для](https://docs.yandex-team.ru/devtools/intro/quick-start-guide#ide-plugin-setup) VS Code и IDE от JetBrains

Также тебе пригодится [справочник по командам Arc](https://docs.yandex-team.ru/arc/ref/commands).

### Расположение {#locations}

Наш проект в Аркадии называется [Market apps](https://a.yandex-team.ru/projects/beruapps). Путь к исходным кодам приложений:

{% list tabs %}

- Android

  * В Arcanum: [https://a.yandex-team.ru/arc_vcs/mobile/market/android/app-market](https://a.yandex-team.ru/arc_vcs/mobile/market/android/app-market)
  * Локально: `~/arcadia/mobile/market/android/app-market` (если репозиторий смонтирован в `~/arcadia`)

- iOS

  * В Arcanum: [https://a.yandex-team.ru/arc_vcs/mobile/market/ios/app-market](https://a.yandex-team.ru/arc_vcs/mobile/market/ios/app-market)
  * Локально: `~/arcadia/mobile/market/ios/app-market` (если репозиторий смонтирован в `~/arcadia`)

{% endlist %}

### Уведомления {#notifications}

* Запусти бота [@ya_arcanum_bot](https://t.me/ya_arcanum_bot), чтобы получать уведомления в Telegram.
* Настрой уведомления в [Arcanum](https://a.yandex-team.ru) по клику на аватарку. Описание настроек ты найдёшь [здесь](https://docs.yandex-team.ru/arcanum/notifications/readme).

### GUI для Arc {#arc-gui}

* Установи плагины [для VC Code](https://a.yandex-team.ru/arc_vcs/devtools/ide/vscode-yandex-arc/README.md) или [для IDE от JetBrains](https://a.yandex-team.ru/arc_vcs/devtools/intellij/README.md), если хочешь использовать графический интерфейс.
* Плагины позволяют выполнять основные операции — создавать и удалять ветки, переключаться между ветками, добавлять изменения в индекс, отменять изменения, создавать коммиты, подливать изменения из других веток, разрешать конфликты, синхронизировать локальные и удалённые ветки, создавать ПР и так далее.
* Также плагины позволяют просматривать авторство последних изменений (`blame`). Нажми `Command + Shift + P` в VC Code и выбери из списка `Arc: Toggle File Decorations` или `Arc: Toggle Current Line Decorations` чтобы активировать просмотр авторства изменений.
* Плагин для IDE от JetBrains предлагает больше функций по сравнению с плагином для VC Code. Например, показывает изменения в коммитах, позволяет ревьювить код входящих ПР прямо из IDE.

### Тонкая настройка {#fine-tuning}

* Добавь в [конфигурационный файл](https://docs.yandex-team.ru/devtools/src/arc/config#config) `~/.arcconfig` необходимые тебе настройки. Например, сокращения для команд и удобный редактор.

* Можешь включить [автодополнение команд](https://docs.yandex-team.ru/devtools/src/arc/config#shell-completion):

  {% list tabs %}

  - Bash в MacOS

    ```bash
    mkdir -p $(brew --prefix)/etc/bash_completion.d
    arc completion bash > $(brew --prefix)/etc/bash_completion.d/arc
    ```
    
  - Zsh в MacOS
    
    ```bash
    mkdir -p ~/.zfunc
    arc completion zsh > ~/.zfunc/_arc
    ```
    также добавь в начало `~/.zshrc`
    ```
    fpath+=~/.zfunc
    autoload -Uz compinit && compinit
    ```

  {% endlist %}

* Также можешь сделать более информативным [приглашение командной строки](https://docs.yandex-team.ru/devtools/src/arc/config#prompt).

* При монтировании репозитория Аркадии используй параметры `-m`, `--store` и `--object-store`:
  ```bash
  arc mount -m ~/path/to/arc/repo --store ~/path/to/store --object-store ~/path/to/common/object/store
  ```
  Параметр `-m` задает точку монтирования виртуальной файловой системы, а `--store` и `--object-store` — реальные папки для хранения локального состояния репозитория и скачиваемых данных.

  В параметре `--store` укажи путь к индивидуальному хранилищу каждого монтирования, в `--object-store` — путь к общему хранилищу для всех монтирований. Так ты сэкономишь место на диске.
  
  Если не задашь параметр `--store`, то `arc` использует путь по умолчанию `~/.arc/store` для первого репозитория и `~/.arc/stores/_path_to_another_store` для последующих
* Можешь несколько раз смонтировать репозиторий Аркадии в разные папки, тем самым получишь возможность одновременной работы с несколькими ветками:
  ```bash
  arc mount -m ~/path/to/another/arc/repo --store ~/path/to/another/store --object-store ~/path/to/common/object/store
  ```
* Когда копия репозитория станет совсем ненужной, размонтируй её с флагом `--forget`:
  ```bash
  arc umount --forget ~/path/to/another/arc/repo
  ```
  Указанная копия пропадёт из списка известных.

* После выполнения команды `mount` файлы из репозитория Аркадии скачиваются не сразу, а по мере обращения к ним. Поэтому холодная сборка может выполняться значительное время.

  Для ускорения скачай все файлы из нужной папки, например:

  {% list tabs %}

  - Android

    ```bash
    arc prefetch-files ~/arcadia/mobile/market/android/app-market
    ```

  - iOS

    ```bash
    arc prefetch-files ~/arcadia/mobile/market/ios/app-market
    ```

  {% endlist %}

  {% note alert %}

  Не запускай `arc prefetch-files` в корне репозитория чтобы не скачивать чрезмерный объём данных

  {% endnote %}

* Arc по умолчанию запускает VS Code командой `code`. Чтобы эта команда `code` была доступна, нажми `Command + Shift + P` в VS Code и выбери из списка `Shell Command: Install 'code' command in PATH`.

## Работа с исходным кодом {#workflow}

Теперь, когда репозиторий смонтирован и Arc настроен, изучи:

* [Основные сценарии](https://docs.yandex-team.ru/devtools/src/arc/workflow) использования Arc как системы контроля версий
* [Принципы](https://docs.yandex-team.ru/devtools/src/arc/branches) работы с ветками
* Конфликты и [способы их разрешения](https://docs.yandex-team.ru/devtools/src/arc/conflict)
* Способы [менять историю](https://docs.yandex-team.ru/devtools/src/arc/history) коммитов, пока они не влиты в `trunk` или `release`

Далее рассмотрим прикладные задачи.

### Какие ветки используем {#brach-naming}

{% list tabs %}

- Android

  * **trunk** — общая на весь Яндекс ветка, в которую мы отправляем протестированный код
  * **users/`username`/xxxx-my-feature** — ветка для разработки фичи, где `username` — твой логин на стафе. Далее по тексту `feature`
  * **releases/mobile/market/release/x** — ветка для тестирования релиза. Далее по тексту `release`
  * **releases/mobile/market/hotfix/x** — ветка для тестирования хотфикса. Далее по тексту `hotfix`

- iOS

  * **trunk** — общая на весь Яндекс ветка, в которую мы отправляем протестированный код
  * **users/`username`/BLUEMARKETAPPS-xxxxx** — ветка для разработки фичи, где `username` — твой логин на стафе. Далее по тексту `feature`
  * **releases/mobile/market/release/x** — ветка для тестирования релиза. Далее по тексту `release`
  * **releases/mobile/market/hotfix/x** — ветка для тестирования хотфикса. Далее по тексту `hotfix`

{% endlist %}

### Как сделать фичу {#how-to-create-feature-branch}

Каждая фича должна быть привязана к тикету из очереди [BLUEMARKETAPPS](https://st.yandex-team.ru/BLUEMARKETAPPS).

{% list tabs %}

- Android

  Создай `feature` ветку для своей фичи от `trunk` (шаблон **<номер_тикета>-<название_фичи>**), например:
  ```bash
  arc pull trunk
  arc checkout trunk
  arc checkout -b 21014-my-feature-part-one
  ```

- iOS

  Создай `feature` ветку для своей фичи от `trunk` (шаблон **BLUEMARKETAPPS-<номер_тикета>**), например:
  ```bash
  arc pull trunk
  arc checkout trunk
  arc checkout -b BLUEMARKETAPPS-21014
  ```

{% endlist %}

### Как добавлять и отменять изменения {#how-to-manage-changes}

* Внеси изменения в рабочее дерево. Перед тем, как делать коммит, добавь изменения в индекс с помощью `arc add`. Например:
  ```bash
  arc add .
  ```
* Для просмотра изменений используй команду `arc diff`.
  Например, для просмотра изменений, не добавленных в индекс:
  ```bash
  arc diff
  ```
  Для просмотра изменений, добавленных в индекс:
  ```bash
  arc diff --cached
  ```
  Для просмотра всех изменений, добавленных и не добавленных в индекс:
  ```bash
  arc diff HEAD
  ```
* Для отмены изменений, ранее добавленных в индекс, используй команду `arc reset`. Например, для удаления и из индекса, и из рабочего дерева:
  ```bash
  arc reset --hard HEAD
  ```
* Для удаления из рабочего дерева изменений, не добавленных в индекс, используй команду `arc clean`. Например:
  ```bash
  arc clean -d
  ```

  {% note alert %}

  Используй команды отмены изменений с осторожностью!

  {% endnote %}

### Как подливать изменения из других веток {#how-to-rebase}

* Чтобы подлить в свою ветку изменения из других веток, используй команду `arc rebase`. Например, подливай свежие изменени из `trunk`:
  ```bash
  arc pull trunk
  arc rebase trunk
  ```
* Если при выполнении `arc rebase` возникли конфликты, разреши их и добавь изменения в индекс при помощи `arc add`. Затем продолжи выполнение:
  ```bash
  arc rebase --continue
  ```
* Если конфликты так просто не решить, можешь прервать выполнение `arc rebase`:
  ```bash
  arc rebase --abort
  ```
* Подливать изменения можно не только из `trunk`. Например, если вы вместе с коллегой работаете над новой фичей, можешь подлить изменения, ещё не попавшие в транк также с помощью `arc rebase` или же с помощью `arc cherry-pick`. Например:
  ```bash
  arc cherry-pick xxxxxdfc2287c9c470d92d6dfbce7390039d035f
  ```
* Для редактирования коммитов, ещё не попавших в `trunk`, используй интерактивный режим `arc rebase`. Например, чтобы отредактировать все коммиты и перебазировать их на `trunk`:
  ```bash
  arc rebase -i trunk
  ```
  После чего в редакторе откроется файл со списком коммитов и действиями для каждого их них:
  ```
  pick 9ba841bb5203a7d67ec875b3f95aac9efafc7147 BLUEMARKETAPPS-21014: Добавлен класс Demo
  pick dc6d3ded95106cd0053bb2d58964999fbf24f160 BLUEMARKETAPPS-21014: Исправлена ошибка в Demo
  pick 343e6dfc2287c9c470d92d6dfbce7390039d035f BLUEMARKETAPPS-21014: Добавлены комментарии

  # Commands:
  # p, pick <commit> = use commit
  # r, reword <commit> = use commit, but edit the commit message
  # e, edit <commit> = use commit, but stop for amending
  # s, squash <commit> = use commit, but meld into previous commit
  # f, fixup <commit> = like "squash", but discard this commit's log message
  # d, drop <commit> = remove commit
  #
  # These lines can be re-ordered; they are executed from top to bottom.
  #
  # If you remove a line here THAT COMMIT WILL BE LOST.
  #
  # However, if you remove everything, the rebase will be aborted.
  #
  ```
  По умолчанию предлагается сохранить все коммиты без изменений. Файл можно отредактировать и сохранить. После выхода из редактора команды из файла будут применены к коммитам.

  Например, если отредактировать файл следующим образом:
  ```
  pick 9ba841bb5203a7d67ec875b3f95aac9efafc7147 BLUEMARKETAPPS-21014: Добавлен класс Demo
  fixup dc6d3ded95106cd0053bb2d58964999fbf24f160 BLUEMARKETAPPS-21014: Исправлена ошибка в Demo
  drop 343e6dfc2287c9c470d92d6dfbce7390039d035f BLUEMARKETAPPS-21014: Добавлены комментарии
  ```
  то второй коммит объединится с первым, а третий будет удалён.
* О том, как подливать изменения в релизные ветки, прочитай в [руководстве по работе с релизными ветками](https://docs.yandex-team.ru/devtools/src/arc/branches#release-branches).

### Как сделать коммит {#how-to-commit}

{% list tabs %}

- Android

  * Убедись, что все необходимые изменения добавлены в индекс
    ```bash
    arc diff --cached
    ```
  * Проверь что не нарушен стиль kotlin кода командой `./gradlew detekt`
  * Проверь что не нарушен стиль java кода командой `./gradlew callInquisitor`
  * Сделай коммит с комментарием о внесенных изменениях на русском языке
    ```bash
    arc commit -m "21014-my-feature-part-one: Добавлен класс Demo"
    ```
  * Загрузи локальные коммиты на сервер командой `arc push`. При первой загрузке укажи связь локальной ветки с серверной:
    ```bash
    arc push -u /users/username/21014-my-feature-part-one
    ```
    В дальнейшем связь с серверной веткой можешь не указывать, например:
    ```bash
    arc push -f
    ```

- iOS

  * Убедись, что все необходимые изменения добавлены в индекс
    ```bash
    arc diff --cached
    ```
  * Сделай коммит с комментарием о внесенных изменениях на русском языке
    ```bash
    arc commit -m "BLUEMARKETAPPS-21014: Добавлен класс Demo"
    ```
  * Загрузи локальные коммиты на сервер командой `arc push`. При первой загрузке укажи связь локальной ветки с серверной:
    ```bash
    arc push -u /users/username/BLUEMARKETAPPS-21014
    ```
    В дальнейшем связь с серверной веткой можешь не указывать, например:
    ```bash
    arc push -f
    ```

{% endlist %}


### Как создать pull request {#create-pull-request}

* Создай PR командой `arc pr create`. Название PR должно начинаться с `BLUEMARKETAPPS-xxxxx` и содержать краткое описание изменений, например:
  ```bash
  arc pr create -m "BLUEMARKETAPPS-21014: Добавлен класс Demo" --publish
  ```
* Бот добавит ревьюеров и отправит им ссылку на PR. После завершения ревью ты получишь [уведомление](#notifications).
* Убедись что успешно завершились все необходимые проверки
* Если PR успешно прошел ревью и завершены все проверки, можешь нажимать кнопку мержа (бот переведет тикет в статус `Ждёт релиза`)

### Про ревью и проверки PR {#about-review-and-checks}

* Придерживайся следующего процесса для тикетов: В работе → Ревью (автоматически при публикации PR) → Можно тестировать (вручную при готовности сборки QA) → Протестировано (устанавливает QA) → Ждёт релиза (автоматически после мержа)
* Чтобы участвовать в ревью, получи в нашем [ABC-сервисе](https://abc.yandex-team.ru/services/beruapps/) роль `Разработчик Android приложений` или `Разработчик iOS приложений`, в зависимости от платформы.
* Для одобрения PR, где ты назначен ревьювером, используй кнопку `Ship`. Когда автор PR вносит изменения, например, разрешает конфликты с `trunk`, твой `Ship` сбрасывается и требуется повторное одобрение.

  Если доверяешь автору, можешь использовать `Sticky ship` из выпадающего меню кнопки `Ship`. При последующих изменениях `Sticky ship` сбрасываться не будет.
* Чтобы отправить ревьюверам повторное уведомление, используй значок мегафона (`Ping reviewers`)
* Если ты сильно занят и не можешь участвовать в ревью, добавь в PR комментарий `/busy`. Бот найдёт тебе замену.
* Ссылки на сборки QA сейчас не добавляются в связанный тикет автоматически. Перейди из PR по ссылке `QA сборка` в TeamCity, нажми `Show full log`, найди в логе ссылку на сборку (ссылка начинается с `https://beta.m.soft.yandex.ru/`) и самостоятельно добавь её в связанный тикет.
* Чтобы настроить альтернативу specialReviewers в Аркадии: создай новую роль в нашем [ABC-сервисе](https://abc.yandex-team.ru/services/beruapps/), добавь в `a.yaml` новую секцию, указав относительные пути и slug роли.
* Если хочешь поместить скриншот в описание PR, залей его в [Jing](https://jing.yandex-team.ru/) и добавь ссылку под кат:
  ```
  {% cut "Скриншот" %}

  ![](https://jing.yandex-team.ru/files/username/screenshot.jpg)

  {% endcut %}
  ```
* PR проходит следующие проверки:

  Проверка | Обязательная | Требования для выполнения проверки
  :--- | :--- | :---
  Code Review | Да | Одобрение от двух ревьюверов
  Closed issues | Да | Нет открытых замечаний ревьюверов
  Task linked | Да | Прилинкован тикет BLUEMARKETAPPS
  Проверка полей | Да | В прилинкованном тикете заполнены поля Регресс и Затронутые компоненты
  MOBILE_IS_OK | Нет | [Мета-статус](https://wiki.yandex-team.ru/mobvteam/tier1/#rabotasrevju)
  Проверка конфликтов | Да | Отсутствие конфликтов с целевой веткой (`trunk`, `release` или `hotfix`)
  PCI-DSS integrity | Нет | Пройдена [проверка целостности](https://a.yandex-team.ru/arc_vcs/repo/pciexpress/docs/integrity-checks.md)
  Тикет протестирован | Да | Прилинкованный тикет в статусе Протестировано
  SwiftLint и SwiftFormat | Да | Успешно отработали SwiftLint и SwiftFormat
  Qa сборка | Нет | - (возможность собрать сборку QA для тестирования)
  UI и Unit тесты | Да | Успешно пройдены все Unit, Snapshot и UI тесты
  Unit тесты | Нет | Успешно пройдены все Unit тесты

  Некоторые обязательные проверки могут быть пропущены, если они неприменимы к данному PR. Например, ты добавляешь часть большой доработки, скрытую под невключаемым тоглом и недоступную для QA. В этом случае может быть пропущена проверка "Тикет протестирован".

## Полезности {#tips-and-tricks}

### Быстрый checkout {#fast-checkout}

По умолчанию выбирается конфиг хуков, где при каждом чекауте выполняется генерация проекта, из-за этого чекаут между ветками занимает очень много времени.

Если вам это не нужно и вы готовы вручную вызывать `make gen` после чекаута, можно выбрать быстрый конфиг. Для этого нужно выполнить команду `make fast_githooks` и чекаутиться со скоростью света.

Это нужно в том случае, если вам требуется переключиться между несколькими ветками, при этом не компилируя проект после каждого чекаута, или в случае если ветки не имеют различий в файловой структуре проекта, только изменения в отдельных файлах.

Чтобы вернуться к базовой конфигурации хуков, выполни `make common_githooks`.

### Автоматическое добавление тикета в название PR

{% list tabs %}

- iOS

  Сохрани [скрипт](https://paste.yandex-team.ru/9207764) и добавь в `~/.arcconfig`:
  ```
  [alias]
  prpush = "!f() { sh "полный путь к скрипту" $1;}; f"
  ```
  далее используй `prpush`:
  ```
  arc prpush "Описание PR"
  ```
  Скрипт создаёт PR, подставляя имя ветки в начало названия.

{% endlist %}

## Решение проблем {#help}

* **У меня проблема или вопрос, где искать помощь?**

  Убедись, что в Аркадии нет [открытых инцидентов](https://infra.yandex-team.ru/timeline?preset=YerLTPA8B34), из-за которых у тебя возникли проблемы. Прочитай [FAQ по Arc](https://wiki.yandex-team.ru/arcadia/arc/faq/) или посты [клуба Аркадии в Этушке](https://clubs.at.yandex-team.ru/arcadia/), возможно, там есть готовый ответ. Задай вопрос в чате [Arc VCS](https://t.me/joinchat/EPm1hEXREWprMLUdqsQ7Mw). Если решение не найдено — напиши [обращение в поддержку devtools](https://forms.yandex-team.ru/surveys/devtools).

* **Ошибка в пайплайне джобы Проверка Pull Request's "Required checks of release pull request has not completed"**

  Если ревью пулл-реквеста проведено, но по какой-то причине он не в статусе "review completed", то джоба проверки PR\`ов упадёт.
Необходимо в PR\`е проставить лейбл "review completed"

