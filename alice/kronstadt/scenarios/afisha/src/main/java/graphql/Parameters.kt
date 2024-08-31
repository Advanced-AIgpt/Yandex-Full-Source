package ru.yandex.alice.kronstadt.scenarios.afisha.graphql

enum class Parameters(val paramName: String, val paramType: String) {
    IMAGE_SIZE("imageSize", "MediaImageSizes!"),
    SHORT_DATE("shortDate", "Boolean"),
    TAG("tag", "[String]");

    fun getFieldInitialization(): String {
        return "\$${paramName}: ${paramType}"
    }
}
