# Урок 13. Создание композитных операций

{% include [note-alert](../_includes/onboarding-alert.md) %}

## Шаг 1. Изучим основы использования композитных операций {#step-1}

1. **Что такое композитная операция**Это операция, составленная из нескольких связанных между собой других операций.
2. **Когда стоит создавать новую композитную операцию**

- Тебе требуется реализовать функционал, для которого нет готовой операции в [библиотеке](https://nirvana.yandex-team.ru/browse?selected=879274) и во всём [пространстве операций](https://nirvana.yandex-team.ru/operations)
- Отдельные части необходимого функционала уже реализованы в виде других операций
- Другие коллеги и команды могут использовать этот потенциал (т.е. ты создаешь операцию не для себя или своей команды — в такой ситуации лучше использовать подграфы)
- Ты планируешь поддерживать операцию в будущем

# Шаг 2. Создадим композитную операцию {#step-2}

На этом шаге мы хотим создать операцию, которая будет выгружать из поданного на вход json случайный сэпмл данных заданного количества.
1. Перейдем в боковое меню Нирваны и выберем раздел Operations.
2. В открывшемся окне выберем Create operations... → Composite operation.![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_16-21-54.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_16-21-54.png)
3. Заполним блок с деталями операции: название, версия, описание, признак детерминированности![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_16-25-01.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_16-25-01.png)
4. Добавим в граф две связанные операции:
   Light
    Json Shuffle и
   Light
    Json Limit![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_16-51-59.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_16-51-59.png)
5. Создадим глобальную опцию limit и свяжем её с опцией операции. Эту опцию можно использовать также, как и глобальную опцию в обычном графе (например, `${global.limit}`), а также переназначать при использовании композитной операции в других графах

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-21-35.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-21-35.png)

6. Создадим вход и выход операции

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-22-43.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-22-43.png)

7. Сохраним операцию.
> При необходимости можно настроить дополнительные опции операции: доступ и тэги.

Операция готова, ты можешь найти ее на [странице](https://nirvana.yandex-team.ru/operations/filter/1?@filterName=All&ownedBy=ExprUserMe()).

# Шаг 3. Запустим граф с созданной композитной операцией {#step-3}

1. Чтобы использовать операцию в графах ее необходимо утвердить. После этого она появится на [странице активных операций в Нирване](https://nirvana.yandex-team.ru/operations/1/filter?@filterName=All&createdBy=ExprUserMe()&statusLifecycle=ExprEnumIn(ready)&deprecation=ExprFlowchartBlockTypeDeprecation(active)&@sorting=created%3Atrue).

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-36-04.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-36-04.png)

2. Создадим новый граф, добавим на него [этот датасет](https://nirvana.yandex-team.ru/data/c33a8221-5f7a-40f6-9d7c-9bcf59a1c984) и созданную операцию. Укажем значение опции limit = 10 и попробуем позапускать граф несколько раз.
3. Твоя композитная операция готова.

# Шаг 4. Изучим дополнительные настройки композитных операций {#step-4}

1. Твоя операция будет привязана к отдельному элиасу и получит отдельную ссылку.

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-53-33.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-53-33.png)

2. Ты можешь клонировать операцию и создавать её новые версии, они будут привязаны к этому элиасу.

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-55-43.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-55-43.png)

3. Не забывай помечать старые версии операции устаревшими и указывать версию для замены. При этом новую версию нужно установить основной для элиаса.

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-56-58.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-56-58.png)

4. Операции, в востребованности которых другими командами ты абсолютно уверен можно предложить разместить в [библиотеке операций](https://nirvana.yandex-team.ru/browse?selected=879274) (пожалуйста, не предлагай в библиотеку операцию, созданную в рамках этого урока).

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-57-58.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_17-57-58.png)

5. Обращай внимание на опцию детерминированности операции — в некоторых случаях включение этой опции может поломать ключевой функционал. Например, включение опции для только что созданной операции приведет к тому, что в результатах операция будет выдавать один и тот же результат, а не случайный сэмпл.
> В конце урока пометь только что созданную операцию устаревшей. Подобная операция уже существует, твоя версия не будет использоваться.

    ![https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_18-03-11.png](https://jing.yandex-team.ru/files/mikelius/browser_2021-07-30_18-03-11.png)

---

В следующем уроке мы научимся [работать с подграфами](https://wiki.yandex-team.ru/nirvana/dev/onboarding-nirvana-q32021/workflows-in-graphs/).
