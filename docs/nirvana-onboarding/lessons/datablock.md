# Урок 7. Забираем данные из Нирваны

{% include [note-alert](../_includes/onboarding-alert.md) %}

Два основных способа создать блок данных:

#### I. Сохранить результат работы операции

1. Выбираем успешно выполненную операцию на графе и переходим на вкладку `I/O` на панели `Selection Details`.
2. Нажимаем **Save as Data** под нужным выходом. Ошибки над этой кнопкой говорят о том, что либо операция выполнилась с ошибкой, либо у данных закончился срок хранения (Absent file).
3. Выбираем имя, тип, квоту, срок хранения и при необходимости добавляем описание.
4. Нажимаем **Save**.
#### II. Загрузить самостоятельно

1. Открываем раздел `Data` в боковом меню и нажимаем **Create Data**.
2. Загружаем файл с компьютера на вкладке `Local File`. Альтернативный вариант — копируем и вставляем данные в окно редактора на вкладке `Text` (обычно открыта по умолчанию). Можно включить подсветку синтаксиса — например, json, java или yaml.
3. Указыаем:

- `Name` — название файла.
- `Type` — [тип данных или формат](https://wiki.yandex-team.ru/nirvana/vodstvo/tipydannyx/). Наиболее распространенные — TSV, JSON и Text.
- `Quota` — квота, в которой будут храниться данные. Подробнее о квотах смотри в соответствующем разделе.
- `TTL (Days)` — time to live, время хранения файла в днях. По умолчанию этот срок равен 14. Будет отсчитываться с момента последнего использования данных в каком-либо инстансе, после этого срока данные могут удалиться в результате регулярной очистки квоты.
- `Tags` — теги.
- `Description` — описание или комментарии.
- `Permissions` — права на чтение, они же действуют и на использование данных. Прав на изменение нет, поскольку созданный в Нирване блок данных изменить невозможно — только сделать новый.

4. Нажимаем **Create**.

    ![https://lead-assessors.s3.yandex.net/2583fa78-4546-4b23-a7a0-2327cf19e10a](https://lead-assessors.s3.yandex.net/2583fa78-4546-4b23-a7a0-2327cf19e10a)

    ![https://lead-assessors.s3.yandex.net/3d2d1426-407a-4e16-ae1a-a086af6caaeb](https://lead-assessors.s3.yandex.net/3d2d1426-407a-4e16-ae1a-a086af6caaeb)