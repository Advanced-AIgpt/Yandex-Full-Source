# Доступные блоки и как их собрать

Общие для всех блоков параметры можно посмотреть [здесь](block-config.md).
Более прикладное описание в разделе категорийных менеджеров [тут](https://wiki.yandex-team.ru/ydo/content-category/blocks/)

### Шапка HeaderWithForm и HeaderWithCallBack

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "HeaderWithForm", //шапка с формой;  HeaderWithCallBack - шапка с call back
        "withOrderCustomizer": true, //встраиваем форму в шапку (в этом случае пропадает заголовок вопроса из формы, можно указать false или не использовать параметр, при этом форму поставить блоком ниже)
    },

    "blockData": {
        "title": "Специалист приедет, когда вам удобно",
        "text": "⏱ Есть бесплатная экспресс-доставка за 1 час \n\u2028\uD83E\uDD11 Бутылки и помпа — бесплатно \n\u2028\uD83D\uDE9A От 300 ₽ за 19 литров",

        // картинка на desktop в шапке
        "image": {
            "path": "https://s3.mds.yandex.net/ydo-frontend/landings/water/bottle2.png"
        },

        // картинка на touch в шапке
        "imageTouch": {
          "path": "https://s3.mds.yandex.net/ydo-frontend/landings/water/bottle2.png"
        },

        // если не нужна картинка в шапке, можно не использовать блок "image"
        // если не указана картинка для "imageTouch", но указана картина для "image",
        // если нужно убрать картинку из шапки на одной из платформ, нужно явно прописать такое
        // "imageTouch": {
        //    "path": ""
        // }
        // для тачей можно прописать бенефиты кратко таким образом
        "benefitsTouch": {
            "icon": "pin_outlined", //общие иконки, которые задаются для всего блока, можно для каждого пункта сделать отдельную
            "iconSize": "xs",
            "items": [
                { "title": "Быстро и без перекуров" },
                { "title": "Диагностика — бесплатно" },
                { "title": "Диагностика — бесплатно" },
                { "title": "Диагностика — бесплатно" }
            ]
        }
    }
}
```
{% endcut %}


### Блок текстовых бенефитов

от 2-ух до 8 компонент

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "TextBenefitsNew",
        // platform принимает значения "touch" / "desktop", если его не указывать, то блок показан будет на обеих платформах
        "platform": "desktop"//задает платформу, на которой нужно или не нужно показывать контент, то есть можно показать разное кол-во блоков с разным контентом на разных платформах
    },

    "blockData": {
        "title": "Блок новых бенефитов", // заголовок
        "text": "Вам помогут частные специалисты. Они тоже очень классные", //подзаголовок

        "items": [{
            "icon": "pin_outlined", //иконка задается отдельно для
            "text": "Мастер приедет быстро — даже в выходные",
            "title": "Когда вам удобно"
        }, {
            "iconUrl": "https://yastatic.net/s3/ydo-frontend/landings/design-project/icon-money.svg",
            "text": "Среднее время ремонта — 46 минут",
            "title": "Быстро и без перекуров"
        }, {
            "text": "Если вы заказываете ремонт",
            "title": "Диагностика — бесплатно"
        }]
    }
}
```
{% endcut %}

### Блок с последовательностью действий

от 2 до 4-ех компонент

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "id": "Steps",
        "type": "Steps",
        "view": "Steps",
    },

    "blockData": {
        "title": "Как мы выбираем исполнителей?",
        "text": "",

        "buttonUrl": "", // урл для кнопки (переход на определенный шаг формы)
        "buttonText": "", // текст на кнопке
        "background": "yellow", // задает фон для цифер (точно можно еще делать 'grey', 'white')
        "circlesWithBorder": false, // значения true / false рисовать ли бордер у кружков
        "type": "numbers", // "icons"/"numbers" тип списка - нумерованный или с иконками,
        "showAsCards": "true", // показывает каждый шаг отдельной карточкой
        "touchTextAlign": "center", // еще есть "top", по умолчанию "top"
        "steps": [{
            "title": "Проверяем",
            "description": "Проверяем, есть\u00A0ли у\u00A0компании все нужные документы, лицензии и\u00A0разрешения на\u00A0работу",
            "icon": "", // строковой идентификатор иконки из ui-kit, например `"info_filled_grey"`. если нет иконки будет порядковый номер
            "iconUrl": "" // альтернативный способ задать иконку - ссылка на урл с s3
        }, {
            "title": "Тестируем",
            "description": "Тестируем сервис на\u00A0себе\u00A0\u2014 если всё хорошо, подключаем партнёра и\u00A0 назначаем испытательный срок"
        }, {
            "title": "Контролируем",
            "description": "Если поступают жалобы, разбираемся и\u00A0отключаем\u00A0\u2014 отдельных исполнителей или всю компанию сразу"
        }]
    }
}
```
{% endcut %}


### Блок с "Перезвоните мне"

имеет 2 темы: одну - чтобы использовать на лендосе, вторую - на форме

{% cut "конфиг" %}
```json5
{
      "blockMeta": {
            "type": "Callback",
            "theme": "clear",
            // если хотим использовать на лендосе, если на форме, то не используем этот параметр
            "background": {
                "imageUrl": "https://s3.mds.yandex.net/ydo-frontend/rubricator/santehnicheskie_rabotyi/shutterstock_1833087568.jpg"
            }
      },
      "blockData": {
            "title": "Текст, который я задал из конфига",
            "subtitle": "Иной текст из конфига",

            //параметр центрирования контента блока, может принимать значения left / center / right
            "helmetAlign": "center"
      }
}
```
{% endcut %}

### Блок пакетных предложений

от 3ех до N компонент

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "PackageOffers"
    },

"blockData": {
    "title": "Блок PackageOffers", // какой-то тайтл
    "text": "Описання долго и упорно и свежо и красиво и много харошаго иже с ним", // какой-то текст в подзаголовке

    "items": [{
        "title": "Тайтл какой-то", // тайтл для отдельной карточки
        "icon": "approved", // иконка в этой карточке
        "iconUrl": "...", // url внешней иконки в этой карточке
        "imageUrl": "https://s3.mds.yandex.net/ydo-frontend/rubricator/santehnicheskie_rabotyi/shutterstock_1833087568.jpg", // url картинки в карточке
        "iconBackground": false, // прячет фон иконки
        "imageAlt": "картинка", // описание картинки на случай непрогрузки
        // не использовать одновременно картинки и иконки в карточках
        "description": "Описание какое-то", // описание в карточке
        "services": [{
            "title": "Первое что-то", // услуга
            "price": 300, // цена, можно писать текстом в формате "от 500 ₽"
            "serviceType": "green-tick" // вид иконки отдельной услуги: 'green-tick' | 'red-cross'
        }, {
            "title": "Еще что-то",
            "price": 400
        }, {
            "title": "Еще что-то",
            "price": 400
        }],
        "servicesType": "green-tick", // общая иконка для всех услуг: 'green-tick' | 'red-cross'
        "oldPrice": 39999, // цена общая на карточку, можно писать текстом в формате "от 500 ₽", будет перечеркнута, чтобы показать скидку
        "price": 40000, // цена , можно писать текстом в формате "от 500 ₽"
        "buttonText": "Выбрать это", // тексты на кнопках экшена
        "buttonUrl": "https://yandex.ru/uslugi", // ссылка на кнопке в карточке
        "hideButton": true, // прячет кнопку
    }],

    "itemsIcon": "ban", // общие иконки для всех карточек, если не хочется указывать для каджого айтема
    "itemsIconUrl": "...", // внешний url общих иконок
    "itemsColorScheme": "grey", // общая цветовая схема карточек, принимает значения:
    // * "grey" (серые карточки без обводки),
    // * "bordered" (белые с обводкой),
    // * "white"(белые без обводки)
    "itemsButtonText": "Нажимати", // общий текст кнопок внизу каждой карточки
    "moreButtonColor": "grey" // цвет кнопки, которая раскрывает другие предложения в блоке (если их больше 3ех)
    }
}
```
{% endcut %}

### Блок с регламентом

{% cut "конфиг" %}
```json5
{
    "blockMeta":{
        "type":"Regulations",
        "background":{
            "imageUrl":"https://yastatic.net/s3/ydo-frontend/landings/water/water1.jpg",
            "isSticky":true // ипользуем6 при наличии фона
        },
        "position":"separate"
    },

    "blockData":{
        "title": "",
        "text": "",

        "moreServicesButtonText": "Ещё опции", // текст кнопки "еще опции" для секции, использовать параметр только в особых случаях
        "hideServicesButtonText": "Скрыть", // текст кнопки "скрыть" для секции, использовать параметр только в особых случаях

        "includedTitle": "", // звголовок во включенных услугах, использовать параметр только в особых случаях
        "includedServices": [{
            "title":"Помыть пол и протереть плинтусы",
            "hint": "какой-то текст об этой услуге"
        }, {
            "title":"Пылесосить ковры и коврики"
        }, {
            "title":"Протереть все доступные поверхности"
        }],

        "excludedTitle": "", // звголовок в услугах, которые не были включены, использовать параметр только в особых случаях
        "excludedServices":[{
            "title":"Помыть пол и протереть плинтусы"
        }, {
            "title":"Пылесосить ковры и коврики"
        }, {
            "title":"Протереть все доступные поверхности"
        }],

        "additionalTitle": "", // заголовок в доп услугах, использовать параметр только в особых случаях
        "additionalServices":[{
            "title":"Помыть окна",
            "price":600
        }, {
            "title":"Почистить люстру",
            "price":300
        }]
    }
}
```
{% endcut %}

### Блок отзывов

от 3ех до N

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "Reviews" //тип блока
    },

    "blockData": {
        "title": "Что о нас говорят?", // заголвок
        "text": "",//подзаголовок
        "helmetAlign": "center",//центрирование

        "reviews": [{ //отзывы
            "id": "id1", //id
            "title": "Ремонт унитаза", //заголовок отзыва, обычно пишем туда услугу/группу, по которой быз создан отзыв
            "description": "Сделано качественно, еще и заменили гофру. Указал на будущие и текущие проблемы по другим вопросам. Отличная работа, рекомендую.", // текст отзыва
            "reviewer": {
                "name": "Андрей", // имя реевьюера
                "rating": 5, // рейтинг он же оценка
                "avatarUrl": "https://yastatic.net/s3/ydo-frontend/landings/Cleaning/face-new.jpg" //аватрка, если есть, иначе - юзаем эту заглушку
            }
        }, {
            "id": "id2",
            "title": "Замена смесителя",
            "description": "Большое спасибо мастеру Антону - приехал вовремя, все сразу рассказал и объяснил почему нельзя отремонтировать старый смеситель. Свозил за новым, помог выбрать хороший смеситель (с учётом моего бюджета), рассказал про дальнейшее обслуживание. Оценка 6 из 5. Очень благодарны!",
            "reviewer": {
                "name": "Татьяна",
                "rating": 5,
                "avatarUrl": "https://yastatic.net/s3/ydo-frontend/landings/Cleaning/face-new.jpg"
            }
        }]
    }
}
```
{% endcut %}


### Блок FAQ

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "Faq"
    },

    "blockData": {
        "title": "Частые вопросы",
        "helmetAlign": "center", //договаривались с дизайномм, что будем центрировать все, так что лучше везде писать этот блок, чтооб он автоматически прорастал
        "items": [{
            "title": "Как найти подходящего специалиста?",
            "text": "На Яндекс.Услугах не нужно выбирать, звонить и договариваться с мастером. Вам нужно только оставить заявку на сайте: расскажите, что и когда нужно сделать. Мы сами подберём свободного специалиста, который справится с вашей задачей, посвятим его во все детали и проследим, чтобы работа была сделана качественно."
        }, {
            "title": "От чего зависит цена?",
            "text": "Цены на Яндекс.Услугах — не с потолка: у нас есть подробный прайс-лист, которого придерживаются все специалисты. Примерная стоимость вашего заказа рассчитается автоматически, пока вы заполняете заявку на сайте. Чтобы рассчитать цену как можно точнее, мы задаём уточняющие вопросы. Не знаете каких-то деталей? Это не страшно — мастер приедет и разберётся на месте."
        }, {
            "title": "Может ли цена измениться?",
            "text": "Иногда цену трудно предсказать дистанционно: бывает, что мастер приезжает и видит, что задача сложнее, чем кажется, или нужно докупить материалы. Тогда цена может измениться. Но не в разы. Специалист сверится с прайс-листом и согласует все дополнительные услуги с вами."
        }]
    }
}
```
{% endcut %}

### Блок картиночных бенефитов

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "view": "Benefits"
    },

    "blockData": {
        "title": "Как вызвать сантехника?", // заголовок
        "text": "", //подзаголовок
        "helmetAlign": "center", //выравнивание

        "items": [{
            "text": "Ответьте на несколько вопросов — мы подберём специалиста, который сможет решить вашу проблему",
            "imagePath": "https://yastatic.net/s3/ydo-frontend/landings/benefits/benefit-img-2.jpg"
        }, {
            "text": "Вы заранее узнаете стоимость и полный состав работ. Никаких накруток — все цены фиксированные",
            "imagePath": "https://yastatic.net/s3/ydo-frontend/rubricator/santehnicheskie_rabotyi/shutterstock_1915755781.jpg"
        }, {
            "text": "Платить нужно только после окончания всех работ",
            "imagePath": "https://yastatic.net/s3/ydo-frontend/rubricator/santehnicheskie_rabotyi/shutterstock_1849980781.jpg"
        }]
    }
}
```
{% endcut %}


### Блок фотографий до/после

{% cut "конфиг" %}
```json5
{
    "blockMeta":{
        "type":"BeforeAfterPhotos"
    },

    "blockData":{
        "title":"Было / стало", // заголовок
        "text":"Смотрите, какое необыкновенное преображение!", // подзаголовок

        // высота фотографий для десктопа (если не указать, будет 600)
        "previewHeightDesktop": 800,
        // высота фотографий для тачей (если не указать, будет 280)
        "previewHeightTouch": 100,

        "photos":[{
            // первое фото из пары ("до")
            "before": "https://yastatic.net/s3/ydo-frontend/landings/water/water1.jpg",
            // второе фото из пары ("после")
            "after": "https://yastatic.net/s3/ydo-frontend/landings/water/water2.jpg",
            // заголовок под парой и в просмотрщике
            "title": "Квартира после доставки воды",
            // описание пары (отображается только в просмотрщике)
            "subtitle": "Удивительно, не так ли?"
        }]
    }
}
```
{% endcut %}

### Блок Галерея

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "PhotoGallery"
    },

    "blockData": {
        "title": "Примеры работ",
        "text": "На вашем пути встретятся сложные заказы и невежливые заказчики, но это не должно сломать вас",

        // Тип галереи, влияет на размер карточки и соотношение сторон:
        //   * "photo" — для фотографий. Размер изображения для десктопа: 600 x 440 px, для тачей: 328 x 280 px;
        //   * "docs" — для документов. Размер изображения для десктопа: 300 x 420 px, для тачей: 240 x 340 px;
        // Изображения больших или меньших размеров будут вписаны в размеры, указанные выше
        // Высота карточки может быть выше высоты изображения если под ним есть текст/подпись
        "imageType": "photo",
            "items": [{
            "thumbSrc": "https://yastatic.net/s3/ydo-frontend/landings/water/water2.jpg",
            // необязательно (если указана ссылка в photoSrc, то на этой фото будет просмотрщик)
            "photoSrc": "https://yastatic.net/s3/ydo-frontend/landings/water/water2.jpg",
            // необязательно (если текст не указан, то в карточке будет показана только картинка)
            // отображаются две строки текста, остальное обрезается
            "text": "Передаём бутылку с водой девушке",
        }],
    },
}
```
{% endcut %}

### Кнопка с экшеном возврата к определенному блоку лендинга

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
      "type": "Actions"
    },

    "blockData": {
        "title": "Какой-то тайтл",
        "text": "Какой-то сабтайтл",

        "helmetAlign": "center",
        "buttonScrollText": "Создать заказ ↑",
        "buttonScrollTarget": "OrderCustomizer",
        "buttonPhone": "+79123456789"
    }
}
```
{% endcut %}

### Блок со списком карточек

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "Cards"
    },

    "blockData": {
        "title": "Карточки, с которыми мы работаем",

        "items": [{
            "title": "Общий клинический анализ крови",
            "description": "1 рабочий день. Развернутое исследование состава крови",
            "price": "400 ₽",
            "oldPrice": "от 5000",
            "buttonText": "Заказать",
            "buttonUrl": "https://yandex.ru",
            "infoText": "описание из блока инфо"
        }, {
            "title": "Общий клинический анализ крови 2",
            "description": "1 рабочий день. Развернутое исследование состава крови",
            "price": "1400 ₽",
            "oldPrice": "от 10000",
            "buttonText": "Заказать",
            "buttonUrl": "https://yandex.ru",
            "infoText": "описание из блока инфо"
        }, {
            "title": "Общий клинический анализ крови 3",
            "description": "1 рабочий день. Развернутое исследование состава крови",
            "price": "800 ₽",
            "oldPrice": "от 20000",
            "buttonText": "Заказать",
            "buttonUrl": "https://yandex.ru",
            "infoText": "описание из блока инфо"
        }],
        "moreUrl": "https://yandex.ru"
    }
}
```
{% endcut %}

### Блок брендов

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        "type": "Brands"
    },

    "blockData": {
        "title": "Бренды, с которыми мы работаем",
        "description": "Описание брендов, с которыми мы работаем",

        "items": [{
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/a12.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/a12.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/beautifulkitchen.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/beautifulkitchen.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/boconcept.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/boconcept.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/lifestyle.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/lifestyle.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/mebelpro.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/mebelpro.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/a12.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/a12.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/beautifulkitchen.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/beautifulkitchen.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/boconcept.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/boconcept.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/lifestyle.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/lifestyle.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/mebelpro.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/mebelpro.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/lifestyle.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/lifestyle.png"
        }, {
            "iconRetinaUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/mebelpro.png",
            "iconUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/brand-icons/mebelpro.png"
        }]
    }
}
```
{% endcut %}

### Блок популярных услуг

{% cut "конфиг" %}
```json5
{
    "blockMeta": {
        // Тип блока
        "type": "HighlightedServices",
    },

    "blockData": {
        // Заголовок блока. По-умолчанию: "С чем нужна помощь?"
        "title": "С чем нужна помощь?",
        // Текст под заголовком
        "text": "Установим, починим и отладим радиаторы, котлы отопления и многое другое",

        // Ссылка для кнопки под карточками: абсолютная или относительная. Используется только если
        // не указан moreButtonTeleportUrl. Если не укзана ни одна из ссылок для кнопки, то кнопка
        // просто не отобразится
        "moreButtonUrl": "https://uslugi.yandex.ru",
        // Ссылка-телепорт в новом формате
        "moreButtonTeleportUrl": "/category/...",
        // Текст на кнопке под карточками. По-умолчанию: "Мне нужно что-то другое"
        "moreButtonText": "Посмотрите кое-что ещё",
        // Информация для карточек (4 штуки)
        "items": [{
            // Ссылка на картинку
            "imageUrl": "https://yastatic.net/s3/ydo-frontend/rubricator/meditsinskie-analizi/map.png",
            // Альтернативный текст, отображается пока картинка не загрузилась
            "imageAlt": "Радиатор",
            // Название услуги
            "title": "Радиаторы",
            // Пояснение к цене, но вообще можно любой текст
            "description": "Цена за проект",
            // Цена за услугу (валюта подставится автоматически)
            "price": "2390",
            // Старая цена (зачёркнутая)
            "oldPrice": "5000",
            // Ссылка для кнопки, используется если не указан teleportUrl, иначе игнорируется
            "buttonUrl": "https://uslugi.yandex.ru/my-order/28029324-4803-4786-b5f6-d84589552913",
            // Ссылка-телепорт в новом формате
            "teleportUrl": "/category/...",
            // Текст для кнопки, по-умолчанию: "Заказать"
            "buttonText": "Добавить в корзину"
        }]
    }
}
```
{% endcut %}

