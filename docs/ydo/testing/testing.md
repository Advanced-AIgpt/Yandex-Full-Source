# Тестирование

## Ручное тестирование

Сейчас актуальная информация находится на [странице тестирования](https://wiki.yandex-team.ru/users/klimovaksyu/testirovanie-uslug)

### Тестирование функциональности для операторов колл-центра

Нужно быть залогиненным под аккаунтом оператора колл-центра. Такой аккаунт можно создать, запросив роль оператора КЦ в [IDM](https://idm.yandex-team.ru/system/ydo_adminka_test)

{% note tip %}

Совет: создавайте для этого логин с суффиксом `-operator` на конце, чтобы уже из логина было понятно, что это оператор.

{% endnote %}

## Автотесты

Виды автотестов (не все виды тестов есть во всех проектах):

- Юниты (и не очень) на Jest
  - Тесты на компоненты (Enzyme и RTL). Примеры: [RTL](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/components/GeoObjectsList/GeoObjectsList.test.tsx?rev=r9729137#L69), [Enzyme](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/components/forms/FinalFields/PaymentCardListField/tests/PaymentCardListField.test.tsx)
  - Тесты на хуки: [Пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/features/order-customizer/components/newForms/Questions/QuestionGeo/hooks/__tests__/useQuestionGeoInitialValue.test.tsx)
  - Тесты на саги: [Пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/store/balance/sagas/index.test.ts)
  - Другое (всякие утилитарные функции и прочее): [Пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/features/order-customizer/utils/formatPrice.test.ts)
- Скриншоты на сторибуке: [Пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/src/features/ui-kit/components/Button/tests/Button.common.hermione.ts)
- Скриншоты и сценарии на Hermione: [Документация](auto-tests/hermione.md), [Пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/tests/hermione/suites/common/my-orders/index.js?rev=r8922230)
- E2E: тесты на живых данных с бека, без моков и дампов. [Пример](https://a.yandex-team.ru/arcadia/frontend/services/ydo/tests/e2e/suites/common/order/create/create-order.e2e.js)

{% note warning %}

Старайтесь не писать снапшотных тестов на компоненты. Мы от них отказываемся.

{% endnote %}

Команды запуска тестов и другая кастомная информация о тестах для каждого проекта хранится внутри его раздела.

### Выбор способа автотестирования

Какой тест написать? Вот краткая справка:

1. Утилитарная функция или хелпер? – **Jest**
2. Сага? – **redux-saga-test-plan**
3. Компонент (без скриншотов) или хук? – **RTL**
4. Внешний вид компонента? – **Storybook** + Hermione для скриншотов
5. Сложный пользовательский сценарий или happy-path? – **Hermione**

Предполагается, что в большинстве случаев будет достаточно использовать Jest с соответствующими модулями (redux-saga-test-plan или RTL).
