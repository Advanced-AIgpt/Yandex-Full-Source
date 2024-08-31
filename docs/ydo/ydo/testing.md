# Тестирование клиентского приложения

Общая информация находится о тестировании в сервисе в [разделе тестирования](../testing/testing.md)

### Запуск Hermione
Общая информация на [странице](../testing/auto-tests/hermione.md)
- В консоли:  `npm run hermione:local`
- В браузере: `npm run hermione:gui`
- Не скипать указанные в Testcop: `hermione_muted_tests_enabled=false npm run hermione:gui`
- Патчинг дампов:
    - В качестве строки: `./tools/patchHermioneDumps.js`
    - В качестве JSON: `./tools/patchJSONHermioneDumps.js`

Для тестов в `tests/hermione` доступен `linux-chrome` на десктопе и `linux-chrome-iphone` на тачах.
Для Storybook дополнительно работает `chrome-grid-480`. `appium-chrome-phone` можно добавить через `hermione.also.in('appium-chrome-phone')`, если вам нужен тест на реальном девайсе и `chrome-grid-375` - если надо проверить вёрстку на другой ширине.

### Погонятор
Прогнать тест на стабильность локально (с использованием [hermione_test_repeater](https://github.com/gemini-testing/hermione-test-repeater))

```bash
hermione_test_repeater_enabled=true npm run hermione:local -- --grep 'Имя теста' --repeat=10
```

### Тесты с авторизацией

Для тестирования верстки под залогином на проекте используется Hermione-плагин [`hermione-auth-on-record-commands`](https://a.yandex-team.ru/arcadia/frontend/projects/infratest/packages/hermione-auth-on-record-commands#команды) с командами авторизации, преконфигурированными для тестирования на дампах, которые позволяют избавиться от необходимости в протаскивании кук в тесты через переменные окружения или прочих явных ручных действиях для авторизации тестовых аккаунтов. При переснятии дампов или редактировании существующих тестов, которые пока используют устаревшую команду `yaLogin` следует использовать новые команды:

- `authOnRecord([login], [options])` - следует использовать в тестах, где не меняется состояние пользователя. Реальная авторизация происходит только при снятии дампов, при воспроизведении устанавливается только мета-поле `tus`.
- `authAnyOnRecord([groupLoginPrefix], [options])` - следует использовать в тестах, где состояние пользователя изменяется и по этой причине параллельное снятие дампов в нескольких браузерах может быть затруднено. Для восстановления состояния можно использовать команду `yaOnTeardown`, описанную ниже. Реальная авторизация происходит только при снятии дампов, при воспроизведении устанавливается только мета-поле `tus`.
- `logoutOnRecord()` - логаут аккаунта из Паспорта. Может использоваться перед дополнительными проверками за логаутом тестового аккаунта. Реальный логаут происходит только при снятии дампов, при воспроизведении только обновляется мета-поле `tus`.
- `onRecordTeardown(callback)` - выполняет переданный `callback` в `afterEach` в режиме снятия дампов. При необходимости выполнения колбека в любом режиме следует использовать базовую команду `onTeardown`.

Почитать подробнее об обернутых командах можно в readme соответствующих пакетов.

При снятии дампов логин, пароль и uid используемого аккаунта будут выведены в командную строку.  Для управления аккаунтами есть утилита [`tus-cli`](https://a.yandex-team.ru/arcadia/frontend/projects/infratest/packages/tus-cli). Ниже приведены команды для Услуг (на примере аккаунта `yndx-ydo-not-workers0`):
- `tus-cli -c 'uslugi' show -l yndx-ydo-not-workers0 -e prod` - посмотреть информацию;
- `tus-cli -c 'uslugi' bind-phone -l yndx-ydo-not-workers0 -e prod` - привязать тестовый телефон +70000000007.

##### Пример

В тесте добавляем нужную команду `authOnRecord` или `authAnyOnRecord` с явным указанием логина тестового аккаунта или группы аккаунтов соответственно:
```js
it('should show some example', function() {
    return this.browser
        .authOnRecord('worker')
        // или
        // .authAnyOnRecord('workers')
});
```

### Тесты компонентов скриншотами через Storybook

Перед снятием скриншота необходимо собрать Storybook командой `npm run storybook:build` или достаточно выполнить команду `npm run hermione:storybook -- <имя файла>` или `npm run hermione:storybook -- --grep "<имя story>"` (save и play не нужно).

Если нужно собрать не весь Storybook, читаем [тут](tools/env-vars.md).

`Для компонента нужно создать файл `*.story.tsx`, где отобразить нужные состояния компонента. Story для взаимодействия с компонентом называется `Components|<component_name>`, story для теста - `Tests|<component_name>` Пример:

```jsx
//Для независимого от платформы компонента
import * as React from 'react';
import { ComponentStories } from '@yandex-int/storybook-with-platforms';
import { Icon } from '.';

new ComponentStories(module, 'Components|Icon', Icon, 'ydo')
    .add('plain', () => (
        <Icon type={KnownIcons.Clip} />
    ));
```

```jsx
//Для зависимого от платформы компонента
import * as React from 'react';
import { boolean, number } from '@storybook/addon-knobs';
import { ComponentStories } from '@yandex-int/storybook-with-platforms';

import { GreenRating as Desktop } from './GreenRating@desktop';
import { GreenRating as Touch } from './GreenRating@touch';

new ComponentStories(module, 'Components|GreenRating', {
    desktop: Desktop,
    'touch-phone': Touch,
}, 'ydo')
    .add('plain', GreenRating => (
        <GreenRating
            compact={boolean('compact', false)}
            value={number('value', 3.5)}
            wrap={boolean('wrap', false)}
            isDisabled={boolean('isDisabled', false)}
            withEmptyText={boolean('withEmptyText', false)}
        />
    ));
```

```jsx
//Для компонента, использующего другие платформозависимые компоненты, redux store и router

new ComponentStories(module, 'Tests|ServicePage', {
    desktop: Desktop,
}, 'ydo')
    .addDecorator(withRouter())
    .addDecorator(withState())
    .addDecorator(withPlatform(Platform.Desktop))
    .addDecorator(withRootStyle(testRootStyle))
    .add('plain', Component => (
        <Component
            {...defaultProps}
        />
    ))
    .add('bigcard', Component => (
        <Component
            {...defaultProps}
            withBigCard
        />
    ));

new ComponentStories(module, 'Tests|ServicePage', {
    'touch-phone': Touch,
}, 'ydo')
    .addDecorator(withRouter())
    .addDecorator(withState())
    .addDecorator(withPlatform(Platform.Touch))
    .addDecorator(withRootStyle(testRootStyle))
    .add('plain', Component => (
        <Component
            {...defaultProps}
        />
    ))
    .add('bigcard', Component => (
        <Component
            {...defaultProps}
            withBigCard
        />
    ));
```

Hermione-тест пишется в файле `*.hermione.ts` рядом с файлом `*.story.tsx`. Пример теста:

```js
describe('Storybook', function() {
    describe('Icon', function() {
        it('plain', function() {
            const PO = this.PO;

            return this.browser
                .yaOpenComponent('components-icon--all')
                .yaAssertView('plain', PO.Icon());
        });
    })
})

describe('Storybook', function() {
    describe('ServicePage', function() {
        it('plain', function() {
            const PO = this.PO;

            return this.browser
                .yaOpenComponent(`tests-servicepage--plain`, true)
                .yaAssertView('plain', PO.ServicePage(), {
                    ignoreElements: [PO.ServicePage.RecommendedWorkers(), PO.ServicePage.Reviews()],
                });
        });
    });
});
```

В команду `yaOpenComponent` передается id story, который можно получить из URL `http://localhost:9876/?path=/story/`*`components-text--plain`*. Если второй параметр команды `true`, то в название запрашиваемой story будет автоматически добавлено название платформы.

### Фильтрация тестов

Если хочется расскипывать большое количество тестов или нужно делать много
прогонов определённых тестов - может быть удобно сформировать файл со списком
тестов для прогона. Это можно сделать через переменную окружения:

```shell
TEST_FILTER=~/filter.json npm run hermione
```

Формат файла со списком тестов:

```json
[
    {
        "fullTitle": "some-title",
        "browserId": "some-browser"
    }
]
```
