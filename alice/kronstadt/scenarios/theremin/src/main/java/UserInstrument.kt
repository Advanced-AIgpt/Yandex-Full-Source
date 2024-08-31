package ru.yandex.alice.paskill.dialogovo.scenarios.theremin

internal class UserInstrument(
    private val skillId: String,
    override val developerType: DeveloperType,
    override val public: Boolean
) : GenericInstument {

    constructor(skillId: String, developerType: String, isPublic: Boolean) :
        this(skillId, DeveloperType.fromString(developerType), isPublic)

    override val id: String
        get() = skillId

    override val humanReadableName: String
        get() = skillId

    override val index: Int
        get() = -1
}
