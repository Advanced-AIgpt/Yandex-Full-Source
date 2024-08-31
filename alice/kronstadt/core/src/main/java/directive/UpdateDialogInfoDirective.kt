package ru.yandex.alice.kronstadt.core.directive

data class UpdateDialogInfoDirective @JvmOverloads constructor(
    val title: String,
    val url: String,
    val imageUrl: String,
    val adBlockId: String?,
    val style: Style,
    val darkStyle: Style,
    val menuItems: List<MenuItem> = emptyList(),
    val name: String? = null,
) : MegaMindDirective {

    data class MenuItem(val title: String, val url: String)
}
