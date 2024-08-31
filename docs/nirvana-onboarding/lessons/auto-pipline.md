# Урок 30. Автоматизируем пайплайн

{% include [note-alert](../_includes/onboarding-alert.md) %}

[YANG-3874](https://st.yandex-team.ru/YANG-3874)

В этом уроке мы реализуем часть пайплайна из туториала по краудсорсингу в [Мёбиусе](https://moe.yandex-team.ru/courses/my/course/582).

Наш будущий граф делает следующие:
1. Забирает данные датасета.
2. Загружает данные в проект №1 в Толоке
3. Ожидает разметку данных в проекте №1
4. Забирает и агрегирует результаты разметки по MV
5. Фильтрует данные, получившие вердикт отсутствия дорожных знаков
6. Загружает изображения с дорожными знаками в проект №2
Выглядеть он может приблизительно следующим образом.

![https://wiki.yandex-team.ru/users/osekachev/osnovnye-pajplajny/.files/screenshot2021-07-29at17.35.08.png](https://wiki.yandex-team.ru/users/osekachev/osnovnye-pajplajny/.files/screenshot2021-07-29at17.35.08.png)

и задуманное нами происходит в этих его частях

![https://wiki.yandex-team.ru/users/osekachev/osnovnye-pajplajny/.files/screenshot2021-07-29at17.44.58.png](https://wiki.yandex-team.ru/users/osekachev/osnovnye-pajplajny/.files/screenshot2021-07-29at17.44.58.png)

разберем подробнее
1. Забирает данные датасета.
Данные для датасета мы будем хранить в YT и получать с помощью блока **[YT Get](https://nirvana.yandex-team.ru/operation/ae873adb-c8af-42ef-a720-61f9e8f57e75)**.Подробнее о данном блоке можно прочитать [тут](https://wiki.yandex-team.ru/hitman/nirvana/yt/#vygruzkadannyxytget), кратко нам нужно верно заполнить поля *encryptedOauthToken* - токен для доступа к YT, *environment* - кластер на котором находятся данные, чаще всего это *HAHN*, а так же *tableName* путь к таблице какой-то вроде *//home/assessor-research/tech_managers_start/
ТВОЙ_ЛОГИН*/
2. Загружает данные в проект №1 в Толоке
Для того чтобы загрузить полученные данные мы используем два блока.

**[[Light] Groovy Json Map](https://nirvana.yandex-team.ru/operation/10515e31-51a0-11e7-89a6-0025909427cc)**. Он Преобразует JSON объекты на основе groovy выражения. Выражение в нем используется такое:
```java
result = [:]
result["poolId"] = "729055"
result["inputValues"] = _
result
```
Оно указывает id пула в который нужно загрузить задания и данные, а именно url изображений добавляет в inputValues.**[Toloka v1 Upload Tasks](https://nirvana.yandex-team.ru/operation/46f03f90-fd0f-4236-8e78-038ed55338bd)**. Это блок непосредственно загружает задания получив в poolId идентификатор пула в который загружать и информацию о заданиях, которые нужно загрузить.В нем мы укажем *environment* - окружение например *PRODUCTION* (продовое окружение Толоки) и *encryptedOauthToken* токен для доступа к Толоке.
3. Ожидает разметку данных в проекте №1
В этом пункте используются два простых блока.**[Toloka v1 Open Pools](https://nirvana.yandex-team.ru/operation/570c4026-8489-4bbd-ba55-f8e62522e93b) ** который открывает для выполнения заданий пул id которого он получил из предыдущего блока. В нем мы так же укажем *environment* - окружение например *PRODUCTION* (продовое окружение Толоки) и *encryptedOauthToken* токен для доступа к Толоке.**[Toloka v1 Wait Pools](https://nirvana.yandex-team.ru/operation/570c4026-8489-4bbd-ba55-f8e62522e93b)** блок ожидающий закрытия пула из предыдущего блока. Точно так же укажем *environment* - окружение например *PRODUCTION* (продовое окружение Толоки) и *encryptedOauthToken* токен для доступа к Толоке.
4. Забирает и агрегирует результаты разметки по MV
Для того чтобы получить результаты выполненных заданий возьмем блок**[Toloka v1 Get Assignments](https://nirvana.yandex-team.ru/operation/45d0bf20-036d-47f9-bb95-c11f49d90caf)**В нем заполним обычные *environment* - окружение например *PRODUCTION* (продовое окружение Толоки) и *encryptedOauthToken* токен для доступа к Толоке, а так же статус заданий *assignmentStatus* который для нашего примера будет ACCEPTED. Этот блок получит id пула из которого нужно получить задания, а отдаст результаты выполненных заданий, а так же контрольных и обучающих заданий. В нашем примере мы не используем контрольные и обучающие задания, поэтому получаем только обычные.Задания могут быть c перекрытием 2 и больше, поэтому чтобы получить конечный результат мы агрегируем данные. Для агрегации используем простой алгоритм Majority Vote и блок реализующий его.**[[Light] MV (json](https://nirvana.yandex-team.ru/operation/48267ffc-6fc3-4c94-91d3-e06558fc2854))** подробнее о данном блоке можно почитать [тут](https://wiki.yandex-team.ru/hitman/nirvana/aggregationmodel/#mvjson). Мы же укажем в *jiKey* где находятся входные данные полученных заданий  *inputValues*, в *resultKey* где находятся выходные данные *outputValues* и в *workersKey* данные об исполнителях, а именно id исполнителя для каждого задания *workerId*
5. Фильтрует данные, получившие вердикт отсутствия дорожных знаков
Для фильтрации существует блок**[[Light] Groovy Json Filter](https://nirvana.yandex-team.ru/operation/c6332dae-413c-11e7-89a6-0025909427cc)** он фильтрует объекты из JSON-массива на входе по условию, переданному в параметрах в виде groovy-предиката.Наш groovy будет таким.
```java
_.outputValues.result == "OK"
```
И на выходе из данного блока мы получив все задания подходящие под данное выражение в *output_true* и не подходящие *output_false*. Мы возьмем подходящие результаты подготовим их для загрузки в пул №2 с помощью**[[Light] Groovy Json Map](https://nirvana.yandex-team.ru/operation/10515e31-51a0-11e7-89a6-0025909427cc)** groovy выражение теперь будет такое
```java
result = [:]
result["poolId"] = "729119"
result["inputValues"] = _["inputValues"]
result
```
Оно указывает id в который пула в который нужно загрузить задания и отбирает только входные данные *inputValues*

6. Загружает изображения с дорожными знаками в проект №2Мы снова используем блок **[Toloka v1 Upload Tasks](https://nirvana.yandex-team.ru/operation/46f03f90-fd0f-4236-8e78-038ed55338bd)** в нашем случае с теми же настройками.
