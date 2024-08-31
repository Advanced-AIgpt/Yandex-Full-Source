Библиотека для классификации запросов по поверхностям и uaas-ограничениям.

При добавлении новой поверхности обратитесь к nstbezz@ или paxakor@, чтобы уточнить содержимое поля UaasInfo.

Матчинг по полю Restrictions в uaas_info.yaml:
1) Если поле указано в Restrictions, то соответствующее поле, переданное клиентом, должно совпадать.
2) AppId клиента и AppId вхождения (если указано) сравниваются без учёта регистра (case insensitive).
3) Выдаётся первое сматчившееся uaas_info. То есть порядок вхождений в рамках одного app_id в файле uaas_info.yaml важен!
4) Предполагается, что каждый ключ QuasarPlatform сопоставляется одному из значений UaasInfo. Список всех QuasarPlatform тут: https://a.yandex-team.ru/arc/trunk/arcadia/quasar/backend/src/backend/utils.cc?rev=r8096108#L15.
