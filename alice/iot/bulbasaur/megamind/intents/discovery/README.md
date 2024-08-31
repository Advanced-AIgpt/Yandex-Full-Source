# Описание процесса discovery через колонку

Основная статья на вики - https://wiki.yandex-team.ru/users/mavlyutov/zigbee/zigbeediscovery#scenarijjdobavlenijasf/tsf/directives/nlg/nluhints/sideeffects

## Описание процессоров 
### StartDiscoveryProcessor
Реагирует на TSF и granet frame начала discovery. 
Понимает, какие протоколы поддерживает колонка, и отправляет директиву StartDiscoveryDirective с этим набором протоколов.

### StartTuyaBroadcastProcessor
Работает в случае, когда в StartDiscoveryDirective передан протокол WiFi. 
Ходит в облако Tuya для получения необходимых данных для передачи ssid/password лампочкам. 

### HowToProcessor
Работает на вопросы вида "как подключить устройство". Отдает nlg с инструкцией.  

### CancelProcessor
Работает на голосовые кнопки "отмени поиск / хватит / не ищи". Отдает CancelDiscoveryDirective.  

### FinishDiscoveryProcessor
Валидирует и сохраняет найденные устройства в базу. Создан для ответа на прямое пользовательское взаимодействие. 
В качестве сайдэффектов рассылает xiva-пуши в ПП, чтобы сообщить приложению о завершении поиска. 
Для сохранения устройств после обновления прошивки используется FinishSystemDiscoveryProcessor. 

### FinishSystemDiscoveryProcessor
Валидирует и сохраняет найденные устройства в базу. 
Создан для ответа на запросы колонки после того, как в ней обновилась прошивка и устройства научились чему-то новому.
Кроме сохранения в базу, других сайдэффектов не порождает. 