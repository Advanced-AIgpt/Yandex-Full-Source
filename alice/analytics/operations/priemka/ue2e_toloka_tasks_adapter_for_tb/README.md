== ue2e adapter

Python код преобразования данных для Толоки из ue2e формата в формат для Template Builder ue2e-dialog

==== how to:
* arc:
    * установить, счекаутить аркадию, примонтировать https://docs.yandex-team.ru/arc/

* работа с ue2e-adapter:
    * перейти в папку `alice/analytics/operations/priemka/ue2e_toloka_tasks_adapter_for_tb`
    * обновиться: `arc checkout trunk && arc pull`
    * отвести свою ветку: `arc checkout -b ATAN-777_my_feature`
    * написать код
    * прогнать существующие тесты: `ya make -t`
    * добавить новые тесты в файл (`test_data.in.json`)
    * прогнать существующие тесты и убедиться, что они падают: `ya make -t`
    * канонизировать новое поведение кода: `ya make -t -Z`
    * закоммитить все файлы `arc add . && arc commit -m "ATAN-777: изменение блаблабла"`
    * запушить на сервер и создать пулл-реквест `arc pr create --push`
    * новые коммиты `arc push`