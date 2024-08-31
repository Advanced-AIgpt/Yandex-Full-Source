package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

interface GenericInstument {
    val id: String
    val developerType: DeveloperType
    val public: Boolean
    val index: Int
    val humanReadableName: String
}
