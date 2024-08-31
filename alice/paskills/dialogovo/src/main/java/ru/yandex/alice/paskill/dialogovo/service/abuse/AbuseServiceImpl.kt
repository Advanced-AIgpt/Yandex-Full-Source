package ru.yandex.alice.paskill.dialogovo.service.abuse

import com.fasterxml.jackson.annotation.JsonProperty
import com.google.common.cache.CacheBuilder
import com.google.common.cache.CacheLoader
import com.google.common.cache.CacheStats
import com.google.common.cache.LoadingCache
import com.google.common.collect.Sets
import org.apache.logging.log4j.LogManager
import org.springframework.web.client.RestTemplate
import org.springframework.web.client.postForObject
import java.time.Duration
import java.util.concurrent.ExecutionException

private const val METHOD = "tmu.check_all"
private const val JSONRPC_2_0 = "2.0"
private val ID: Any = 1
private val EXPIRATION_DURATION = Duration.ofDays(1)

internal class AbuseServiceImpl(
    private val restTemplate: RestTemplate,
    private val abuseUrl: String,
    cacheSize: Long,
) : AbuseService {

    private val logger = LogManager.getLogger()

    private val cache: LoadingCache<String, String> = CacheBuilder.newBuilder()
        .maximumSize(cacheSize)
        .expireAfterWrite(EXPIRATION_DURATION)
        .recordStats()
        .build(object : CacheLoader<String, String>() {
            override fun load(key: String): String {
                return loadAll(listOf(key)).values.iterator().next()
            }

            override fun loadAll(keys: Iterable<String>): Map<String, String> {
                return callAbuser(Sets.newHashSet(keys))
            }
        })

    val cacheStats: CacheStats
        get() = cache.stats()

    override fun checkAbuse(docs: List<AbuseDocument>): Map<String, String> {
        if (docs.isEmpty()) {
            return emptyMap()
        }

        // group by string to text
        val textToIds: Map<String, Set<String>> = docs.groupBy { it.value }
            .mapValues { e -> e.value.map { it.id }.toHashSet() }
        val results: Map<String, String> = censor(textToIds.keys)

        // associate IDs to censored text
        return results.entries
            .flatMap { entry -> (textToIds[entry.key] ?: setOf()).map { uri -> uri to entry.value } }
            .toMap()
    }

    private fun censor(textToCensor: Set<String>): Map<String, String> {
        return try {
            cache.getAll(textToCensor)
        } catch (e: ExecutionException) {
            logger.error("failed to censor texts", if (e.cause != null) e.cause else e)

            // try to use everything available in cache on failure
            val fromCache = cache.getAllPresent(textToCensor)
            textToCensor.associateWith { key -> fromCache.getOrDefault(key, key) }
        } catch (e: Exception) {
            logger.error("failed to censor texts", e)

            // try to use everything available in cache on failure
            val fromCache = cache.getAllPresent(textToCensor)
            textToCensor.associateWith { key -> fromCache.getOrDefault(key, key) }
        }
    }

    private fun callAbuser(docs: Set<String>): Map<String, String> {
        val docToGuid: MutableMap<String, Int> = HashMap(docs.size)
        var i = 0
        for (doc in docs) {
            docToGuid[doc] = i++
        }
        val guidToCensoredDoc = executeCall(docToGuid)
        return docToGuid.mapValues { entry -> guidToCensoredDoc[entry.value] ?: entry.key }
    }

    private fun executeCall(docToGuid: Map<String, Int>): Map<Int, String> {
        val params = docToGuid.map { (key, value) -> AbuseRequestItem(value.toString(), key) }

        val body = JsonRpcRequest(JSONRPC_2_0, METHOD, listOf(params), ID)

        val response = restTemplate.postForObject<AbuseServiceResponse?>(abuseUrl, body)

        return response?.result
            ?.filter { it.uri != null && it.result?.censoredText != null }
            ?.associate { it.uri!!.toInt() to it.result!!.censoredText!! }
            ?: mapOf()
    }

    private data class AbuseRequestItem(
        val uri: String,
        val document: String,
    )

    private class AbuseServiceResponse(
        override val jsonrpc: String,
        override val id: Any,
        override val result: List<AbuseResult>?,
    ) : JsonRpcResponse<List<AbuseResult>>

    private data class AbuseResult(
        val uri: String?,
        val result: AbuseResultItem?,
    )

    private data class AbuseResultItem(
        @JsonProperty("CENSORED_TEXT")
        val censoredText: String?,
    )
}

internal data class JsonRpcRequest(
    val jsonrpc: String,
    val method: String,
    val params: List<*>,
    val id: Any,
)

internal interface JsonRpcResponse<T : Any> {
    val jsonrpc: String
    val id: Any
    val result: T?
}
