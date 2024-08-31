# Введение
Aпп исполнителей для сервисной модели.

Весь апп находится за урлом `/employee` и работает под оболочкой Услуги.ПРО.

Определение того, что исполнитель относится к СМ происходит по наличию [type=erp_organization_employee](https://shared-dev.hamster.yandex.ru/uslugi/employee?renderer_export_type=erp_organization_employee)

## Стенды {#stands}

* [Прод](https://uslugi.yandex.ru/employee)
* [Тестинг](https://shared-dev.hamster.yandex.ru/uslugi/employee) - тестинг (тестовая БД), можно использовать для регресса релиза (обновление во время отведения релиза)
* [Мастер](https://renderer-ydo-mobapp-employee-trunk.hamster.yandex.ru/uslugi/employee/schedule) - бета с кодом из trunk (обновление на каждый мерж ПР)
* [Мастер Storybook](https://frontend-test.s3.mds.yandex.net/story/@yandex-int/ydo-mobapp-employee/trunk/index.html) - Storybook с кодом из trunk (обновление на каждый мерж ПР)
