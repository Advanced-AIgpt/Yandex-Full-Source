# Конфигурация блока

Конфигурация блока состоит из двух полей:
1. ```blockMeta``` - описывает блок в целом

{% cut "blockMeta" %}
```json5
{
    "blockMeta": {
        // тип блока
        "type": "",

        // вид лейаута блока, возможные значения : 'separate' (скругления, баббл) или "underlaid" (уши)
        // если используем фон, то по дефолту - 'separate', остальное обсуждаем с дизайном
        "view": "separate",

         // "gapTop" - верхний отступ блоков с лейаутом 'underlaid' и отступ от контента до верхней границы блока для блоков с лейаутом 'separate'
         // (не использовать это параметр, или использовать всегда дефолтный large, в отдельный кейсах решать с дизайном)
         // "gapBottom" - нижний отступ блока (одинаково работает для обоих видов лейаутов)
         // возможные значения для "gapTop" и "gapBottom": "large", "medium", "small", "none"
        "gapTop": "large",
        "gapBottom": "large",

        // верхний отступ блоков с лейаутом "separate"
        // в этом случае то "gapTop" регулирует отступы от фона до текста блока, а "separator" - отступ от верхнего края текущего блока до нижнего края предыдущего
        // возможные значения: "large", "medium", "small", "none"
        "separator": "large",

        // есть ли серый фон у блока (лучше использовать 'background.fill'),
        "hasBackground": true,

        //"background" описывает фон со следующими полями:
        "background": {
            "fill": "gray", // при  значении 'gray' выставляет серый фон подобно hasBackground
            "imageUrl": "https://s3.mds.yandex.net/ydo-frontend/landings/water/bottle2.png", // урл фонового изображения (имеет больший приоритет, чем 'fill')
            "imageConfig": { // конфиг фонового изображения
                "fit": "cover", // покрыть изображением блок ('cover') или вписать в него ('contain'),
                "position": "top" //п озиционирование изображения по горизонтали-вертикали, принимает 'top', 'right', 'bottom', 'left', 'center'
            },
            "imageUrlTouch": null, // урл фонового избражения для тачей (используется для переопределения 'imageUrl' для тачей или его сброса - для этого надо передать null)
            "imageConfigTouch": null, // конфиг фонового изображения на тачах, переопределяет поле 'imageConfig'
            "withXxlBottomGapTouch": true // увеличивает нижний отступ до 180px для блоков с фоном на тачах
        },

        // показывает блок только на определенной платформе, принимает значения 'desktop'\'touch', тем самым можно делать разное оттображение и разное кол-во блоков на разных платформах
        "platform": "touch",
      }
}
```

{% endcut %}

{% cut "blockMeta с адаптивными изображениями" %}
```json5
{
    "blockMeta": {
        // остальные поля аналогичны примеру blockMeta выше
        ...,

        //"background" описывает фон со следующими полями:
        "background": {
            // остальные поля аналогичны полю background из примеру blockMeta выше
            ...,
            "imageUrl": {
                "url": "https://avatars.mds.yandex.net/get-ydo_frontend/6287309/2a00000181ce4534b6bd6e87c009fdd1a6b8",
                "size": "x2"
            },
            "imageUrlTouch": {
                "url": "https://avatars.mds.yandex.net/get-ydo_frontend/5403800/2a00000181ce456ed3628911e51dd64a4f84",
                "size": "x2"
            },
        },
      }
}
```

{% endcut %}

2. ```blockData``` - содержит данные конкретного блока

{% cut "blockData" %}
```json5
{
    "blockData": {
        "title": "заголовок",
        "text": "подзаголовок",

        // helmetAlign - параметр расположения контента блока, может принимать значения left / center / right.
        // просьба от дизайна: всегда использовать "center"
        "helmetAlign": "center",

        // stretchTexts - параметр для десктопа, заставляет текст в заголовке и подзаголовке тянуться по всей ширине
        "stretchTexts": true
    }
}
```
{% endcut %}

* По идее, всё что есть в блоке (текст, картинки), должно быть задаваемо через конфиг. Если не получается, можно спросить в канале [#landings](https://yndx-uslugi.slack.com/archives/C0293RC2DRU).
* По идее, любой параметр в ```blockData``` должно быть возможно стереть, и соответствующий кусок блока должен либо исчезнуть, либо заполниться по умолчанию (например, дефолтный заголовок). Если кусок не исчезает, а хочется, можно спросить в канале [#landings](https://yndx-uslugi.slack.com/archives/C0293RC2DRU).
* Чтобы поменять картинки на свои, их нужно [сжать по инструкции](../common-tech/image-optimization/), загрузить в папку [в облаке](https://yc.yandex-team.ru/folders/akuh9g42ec2f43anr1b1/storage/buckets/ydo-frontend?key=landings%2F) и вставить URL в конфиг.

