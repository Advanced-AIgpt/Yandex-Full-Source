package ru.yandex.alice.kronstadt.core.directive

data class HideViewDirective(
    val layer: Layer
) : MegaMindDirective {

    sealed class Layer {
        companion object {
            val DIALOG = Dialog(DialogLayer())
            val ALARM = Alarm(AlarmLayer())
            val CONTENT = Content(ContentLayer())
        }
    }

    data class Dialog(val dialog: DialogLayer) : Layer()

    class DialogLayer

    data class Content(val alert: ContentLayer) : Layer()

    class ContentLayer

    data class Alarm(val alert: AlarmLayer) : Layer()

    class AlarmLayer
}
