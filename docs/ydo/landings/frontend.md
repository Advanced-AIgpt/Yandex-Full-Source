# Работа лендингов на фронте клиентского приложения Услуг

Данные для лендингов хранятся [в слайсе сторы ```orderCustomizer``` в поле ```landing```](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo/src/features/order-customizer/models/index.ts?rev=r9560153#L65), а приехать на клиент могут как [с первоначальной загрузкой страницы](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo/src/adapters/RootStateAdapter.ts?rev=r9401919#L80), так и поменяться после в результате вызова некоторых ручек (к примеру при переходах [на лендинг с главной](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo/src/report-renderer/api-router.ts?rev=r9571983#L148) или [между шагами пошаговой формы](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo/src/report-renderer/api-router.ts?rev=r9571983#L586)).

Пример данных:
```
{
    id: 'dostavka-voda-v1_0',
    blocks: [{
        blockMeta: {
            withOrderCustomizer: true,
            type: 'HeaderWithForm',
            backgroundColor: 'white'
        },
        blockData: {
            image: {
                verticalSize: 'large',
                path: 'https://yastatic.net/s3/ydo-frontend/landings/water/bottle_desk_new.jpeg'
            },
            title: 'Вода приедет, когда вам удобно',
            text: 'Бесплатная доставка от 1 часа \n Бутылки — тоже бесплатно \n Курьер будет точно в срок',
            imageTouch: {
                verticalSize: 'small',
                path: 'https://yastatic.net/s3/ydo-frontend/landings/water/bottle_mob.jpg'
            }
        }
    }]
}
```

В данных хранится идентификатор лендинга ```id``` и массив конфигураций блоков ```blocks```.
Каждый элемент этого массива описывает отдельный блок лендинга.
[Подробнее о конфигурации отдельного блока](block-config.md)
