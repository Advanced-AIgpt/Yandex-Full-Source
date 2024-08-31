package ru.yandex.alice.social.sharing

import ru.yandex.alice.social.sharing.document.Image
import ru.yandex.alice.social.sharing.document.ImageProvider

interface TestImageProvider: ImageProvider {
    fun clear()
}

class InMemoryImageProvider: TestImageProvider {

    private val images: MutableMap<String, Image> = mutableMapOf()

    override fun clear() {
        images.clear()
    }

    override fun get(url: String): Image? {
        return images[url]
    }

    override fun upsert(image: Image, avatarsResponse: String) {
        images[image.externalUrl] = image
    }
}
