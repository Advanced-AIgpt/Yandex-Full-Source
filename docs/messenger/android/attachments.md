# Отправка медиа файлов в чат

Чтобы поддержать полноценную отправку аттачей, добавьте attachments зависимости в свой `gradle` файл:
```groovy
implementation "com.yandex.alicekit:attachments-base:{alicekitVersion}"
implementation "com.yandex.alicekit:attachments-common:{alicekitVersion}"
implementation "com.yandex.alicekit:images:{alicekitVersion}"
```


Создайте класс `com.yandex.attachments.common.AttachmentsHost` и реализуйте интерфейс `AttachmentsHostSpec`

{% cut "Пример реализации:" %}

```kotlin
package com.yandex.attachments.common

import android.content.Context
import com.yandex.images.*
import com.yandex.messaging.attachments.MessengerAttachmentsUiConfiguration


class AttachmentsHost(private val context: Context) : AttachmentsHostSpec {

    override fun getImageManager(): ImageManager {
        val imageParams = DefaultImagesParams()
        val cache = DefaultImageCache(context, imageParams, SharedBitmapLruCache(), null)
        return ImagesLib.builder(context, cache).imagesParams(imageParams)
            .addImageHandler(SimpleUriImageHandler(context.applicationContext))
            .addImageHandler(SimpleNetImageHandler())
            .build()
            .get()
    }

    override fun overrideUiConfiguration() = MessengerAttachmentsUiConfiguration()
}
```

{% endcut %}
