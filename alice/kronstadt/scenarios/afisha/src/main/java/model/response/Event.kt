package ru.yandex.alice.kronstadt.scenarios.afisha.model.response

data class Event(
    val title: String?,
    val contentRating: String?,
    val image: Image?
) {
    data class Image(
        val image: ImageItem?
    ) {
        data class ImageItem(
            val url: String?
        )
    }
}
