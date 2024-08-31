# Последовательность действий

1. Склонировать sandbox-таску для сборки Docker-образа ([пример таски](https://sandbox.yandex-team.ru/task/520072694/view))
    
    ![Screenshot](https://jing.yandex-team.ru/files/ivangromov/uhura_2019-11-14T12%3A40%3A10.900350.jpg)
2. Запустить склонированную таску  

    ![Screenshot](https://jing.yandex-team.ru/files/ivangromov/uhura_2019-11-14T13%3A44%3A18.758642.jpg)
3. После успешного выполнения таски на вкладке `Resources` будет указан тег собранного образа

    ![Screenshot](https://jing.yandex-team.ru/files/ivangromov/uhura_2019-11-14T12%3A46%3A04.675436.jpg)

    ![Screenshot](https://jing.yandex-team.ru/files/ivangromov/uhura_2019-11-14T12%3A47%3A38.373916.jpg)
4. Выкатить на тестинг:
    
    1. Открыть дэшборд https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/dialogovo_testing/
    2. Нажать `Layers`, `Edit layers`, указать новый тег Docker-образа
    3. Нажать `Deploy`, откроется рецепт деплоя
    4. Ещё раз нажать `Deploy`
    5. Дождаться активации сервиса   

    Сервис в няне: https://nanny.yandex-team.ru/ui/#/services/catalog/dialogovo_testing/
5. Выкатить в прод:

    Аналогично выкатке в тестинг, но другим дэшбордом: https://nanny.yandex-team.ru/ui/#/services/dashboards/catalog/dialogovo/
    Рецепт деплоя устроен так, что SAS активируется автоматически после Prepare, а активация в MAN (и далее VLA) **требует ручного потверждения**.
        
    Сервисы в няне (3 штуки, по одному на ДЦ):
    * https://nanny.yandex-team.ru/ui/#/services/catalog/dialogovo_sas/
    * https://nanny.yandex-team.ru/ui/#/services/catalog/dialogovo_man/
    * https://nanny.yandex-team.ru/ui/#/services/catalog/dialogovo_vla/  

# Куда смотреть во время релиза

- [Dialogovo health](https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&dashboard=dialogovo-health)
- [Панель сигналов с балансера](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=dialogovo.alice.yandex.net;itype=balancer;ctype=prod;locations=man,sas,vla;prj=dialogovo.alice.yandex.net;signal=service_total;)
