package ru.yandex.alice.social.sharing

import com.google.protobuf.GeneratedMessageV3
import com.google.protobuf.Message
import ru.yandex.inside.yt.kosher.ytree.YTreeNode
import ru.yandex.web.apphost.api.exception.AppHostItemExpectationException
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder
import ru.yandex.web.apphost.api.request.FlagWithSource
import ru.yandex.web.apphost.api.request.Location
import ru.yandex.web.apphost.api.request.RequestContext
import ru.yandex.web.apphost.api.request.RequestItem
import java.util.*

data class ProtoRequestItem(
    private val data: Message,
    private val type: String = "type",
    private val sourceName: String = "source_name",
): RequestItem {

    override fun getSourceName(): String {
        return sourceName
    }

    override fun getType(): String {
        return type
    }

    override fun getRawSize(): Int {
        return data.serializedSize
    }

    override fun <T : Any> getJsonData(dataClass: Class<T>): T {
        throw NotImplementedError()
    }

    override fun <T : Any> getProtobufData(defaultDataObject: T): T {
        return data as T
    }

    override fun getYsonData(): YTreeNode {
        throw NotImplementedError()
    }

    override fun <T : YTreeNode?> getYsonData(nodeClass: Class<T>): T {
        throw NotImplementedError()
    }

}

data class LocationImpl(
    private val path: String, private val name: String
): Location {
    override fun getPath() = path
    override fun getName() = name
}

class MockRequestContext(
    private val requestItems: List<RequestItem>,
    private val flags: Set<String> = emptySet(),
    private val guid: String = UUID.randomUUID().toString(),
    private val ruid: Long = Random().nextLong(),
    private val requestId: String = UUID.randomUUID().toString(),
    private val location: Location = LocationImpl("", "")
): RequestContext {

    private val outputItems: MutableList<RequestItem> = mutableListOf()
    private val outputFlags: MutableList<String> = mutableListOf()

    constructor(
        inputItems: Map<String, Message>,
        flags: Set<String> = emptySet(),
        guid: String = UUID.randomUUID().toString(),
        ruid: Long = Random().nextLong(),
    ): this(
        inputItems.map { ProtoRequestItem(it.value, it.key) },
        flags,
        guid,
        ruid,
    )

    constructor(
        vararg inputItems: Pair<String, Message>,
        flags: Set<String> = emptySet(),
        guid: String = UUID.randomUUID().toString(),
        ruid: Long = Random().nextLong(),
    ): this(
        inputItems.map { ProtoRequestItem(it.second, it.first) },
        flags,
        guid,
        ruid,
    )

    override fun getGuid(): String = guid

    override fun getRuid(): Long = ruid

    override fun getRequestId(): String = requestId

    override fun getLocation(): Location = location

    override fun getRequestItems(): MutableList<RequestItem> {
        return Collections.unmodifiableList(requestItems)
    }

    override fun getRequestItems(itemType: String?): MutableList<RequestItem> {
        val items = requestItems.filter { it.type == itemType }
        return Collections.unmodifiableList(items)
    }

    override fun getFlags(): MutableList<FlagWithSource> {
//        return Collections.unmodifiableList(flags.toList())
        TODO()
    }

    override fun checkFlag(flagName: String?): Boolean {
        return flags.any { it == flagName }
    }

    override fun checkFlag(flagName: String?, source: String?): Boolean {
        TODO("Flags with sources are not supported yet")
    }

    override fun getSingleRequestItem(itemType: String?): RequestItem {
        val items = requestItems.filter { it.type == itemType }
        if (items.size != 1) {
            throw AppHostItemExpectationException.singleItem(itemType, "0")
        }
        return items[0]
    }

    override fun getSingleRequestItemO(itemType: String?): Optional<RequestItem> {
        return Optional.ofNullable(requestItems.firstOrNull { it.type == itemType })
    }

    override fun addJsonItem(type: String?, data: Any?): ApphostResponseBuilder {
        TODO("Not yet implemented")
    }

    override fun addProtobufItem(type: String, data: Message): ApphostResponseBuilder {
        outputItems.add(ProtoRequestItem(data as GeneratedMessageV3, type))
        return this
    }

    override fun addFlag(flag: String): ApphostResponseBuilder {
        outputFlags.add(flag)
        return this
    }

    override fun addFlags(flags: MutableCollection<String>): ApphostResponseBuilder {
        outputFlags.addAll(flags)
        return this
    }

    override fun addLog(log: String): ApphostResponseBuilder {
        TODO("Not yet implemented")
    }

    override fun getEnableDumpRequest(): Boolean {
        return false
    }

    override fun addHint(sourceName: String, value: Long): ApphostResponseBuilder {
        TODO("Not yet implemented")
    }

    fun getOutputItems(): List<RequestItem> {
        return outputItems
    }

    fun getOutputFlags(): List<String> {
        return outputFlags
    }

    fun getOutputItem(itemType: String): RequestItem {
        val items = outputItems.filter { it.type == itemType }
        if (items.size != 1) {
            throw AppHostItemExpectationException.singleItem(itemType, "0")
        }
        return items[0]
    }

    fun <T: Message> getOutputItemProto(itemType: String, defaultObject: T): T {
        val item = getOutputItem(itemType)
        return item.getProtobufData(defaultObject)
    }
}
