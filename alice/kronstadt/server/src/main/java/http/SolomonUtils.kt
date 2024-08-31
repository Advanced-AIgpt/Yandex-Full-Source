package ru.yandex.alice.paskill.dialogovo.solomon

import com.google.common.cache.CacheStats
import com.google.common.collect.ImmutableSet
import com.google.common.io.CountingOutputStream
import org.apache.logging.log4j.LogManager
import org.springframework.http.HttpHeaders
import ru.yandex.alice.paskills.common.solomon.utils.NamedSensorsRegistry
import ru.yandex.monlib.metrics.encode.MetricEncoder
import ru.yandex.monlib.metrics.encode.MetricFormat
import ru.yandex.monlib.metrics.encode.json.MetricJsonEncoder
import ru.yandex.monlib.metrics.encode.spack.MetricSpackEncoder
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision
import ru.yandex.monlib.metrics.labels.Labels
import ru.yandex.monlib.metrics.primitives.Histogram
import ru.yandex.monlib.metrics.primitives.Rate
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.io.IOException
import java.io.OutputStream
import java.util.function.Supplier
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse
import kotlin.contracts.ExperimentalContracts
import kotlin.contracts.InvocationKind
import kotlin.contracts.contract

/**
 * Utility класс для работы с Solomon.
 */
object SolomonUtils {
    private const val TIMESTAMP_CAP = 0
    private val DEFAULT_FORMAT = MetricFormat.JSON
    private const val FORMAT_PARAMETER = "format"
    private val SUPPORTED_FORMATS: Set<MetricFormat> = ImmutableSet.of(MetricFormat.JSON, MetricFormat.SPACK)
    private val log = LogManager.getLogger()

    /**
     * Создает необходимый [MetricEncoder] по параметрам в запросе.
     * По умолчанию создается [MetricJsonEncoder]
     *
     *
     * NB: LZ4 - https://wiki.yandex-team.ru/solomon/api/dataformat/spackv1/
     * ZSTD не работает
     */
    @JvmStatic
    fun prepareEncoder(request: HttpServletRequest, response: HttpServletResponse, out: OutputStream): MetricEncoder {
        val format = findFormat(request)
        response.status = HttpServletResponse.SC_OK
        return when (format) {
            MetricFormat.SPACK -> {
                response.contentType = format.contentType()
                MetricSpackEncoder(TimePrecision.SECONDS, CompressionAlg.LZ4, out)
            }
            MetricFormat.JSON -> {
                response.contentType = format.contentType()
                MetricJsonEncoder(out)
            }
            else -> {
                throw IllegalStateException("Unsupported format: $format")
            }
        }
    }

    private fun findFormat(request: HttpServletRequest): MetricFormat {
        val formatHeader = request.getParameter(FORMAT_PARAMETER) ?: request.getHeader(HttpHeaders.ACCEPT)
        return formatHeader?.let {
            it.split(",".toRegex())
                .map { contentType -> MetricFormat.byContentType(contentType) }
                .firstOrNull { SUPPORTED_FORMATS.contains(it) }
        }
            ?: DEFAULT_FORMAT
    }

    /**
     * Запуск стрима сенсоров для [ru.yandex.monlib.metrics.MetricConsumer].
     *
     * @param encoder target sensors consumer
     */
    @JvmStatic
    fun dump(registry: MetricRegistry, encoder: MetricEncoder) {
        registry.supply(TIMESTAMP_CAP.toLong(), encoder)
    }

    @JvmStatic
    @JvmOverloads
    fun measureCacheStats(
        metricRegistry: MetricRegistry,
        name: String,
        labels: Labels = Labels.empty(),
        statsSupplier: Supplier<CacheStats>
    ) {
        val registry = NamedSensorsRegistry(metricRegistry, "caches")
            .withLabels(Labels.of("target", name).addAll(labels))
        registry.lazyRate("hit_count") { statsSupplier.get().hitCount() }
        registry.lazyRate("eviction_count") { statsSupplier.get().evictionCount() }
        registry.lazyRate("load_exception_count") { statsSupplier.get().loadExceptionCount() }
        registry.lazyRate("load_count") { statsSupplier.get().loadCount() }
        registry.lazyRate("miss_count") { statsSupplier.get().missCount() }
        registry.lazyRate("request_count") { statsSupplier.get().requestCount() }
        registry.lazyRate("load_success_count") { statsSupplier.get().loadSuccessCount() }
    }

    @Throws(IOException::class)
    @JvmStatic
    fun dumpSolomonSensorsToResponse(
        request: HttpServletRequest, response: HttpServletResponse,
        registry: MetricRegistry
    ) {
        val outputStream = CountingOutputStream(response.outputStream)
        try {
            prepareEncoder(request, response, outputStream).use { encoder -> dump(registry, encoder) }
        } catch (e: Exception) {
            log.error("Error occurred while writing Solomon sensors", e)
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, e.message)
        }
        response.setContentLengthLong(outputStream.count)
    }
}

@OptIn(ExperimentalContracts::class)
inline fun <T> Histogram.measureMillis(action: () -> T): T {
    contract {
        callsInPlace(action, InvocationKind.EXACTLY_ONCE)
    }
    val start = System.nanoTime()
    return try {
        action.invoke()
    } finally {
        this.record((System.nanoTime() - start) / 1_000_000)
    }
}

@OptIn(ExperimentalContracts::class)
inline fun <T> Histogram.measureMicros(action: () -> T): T {
    contract {
        callsInPlace(action, InvocationKind.EXACTLY_ONCE)
    }
    val start = System.nanoTime()
    return try {
        action.invoke()
    } finally {
        this.record((System.nanoTime() - start) / 1_000)
    }
}

@OptIn(ExperimentalContracts::class)
inline fun <T> Histogram.measureNanos(action: () -> T): T {
    contract {
        callsInPlace(action, InvocationKind.EXACTLY_ONCE)
    }

    val start = System.nanoTime()
    return try {
        action.invoke()
    } finally {
        this.record(System.nanoTime() - start)
    }
}

enum class HistogramUnit {
    MILLIS,
    MICROS,
    NANOS
}

@OptIn(ExperimentalContracts::class)
class CallMeter(val rate: Rate, val histogram: Histogram, unit: HistogramUnit = HistogramUnit.MILLIS) {
    val div: Long = when (unit) {
        HistogramUnit.MILLIS -> 1_000_000
        HistogramUnit.MICROS -> 1_000
        HistogramUnit.NANOS -> 1
    }

    inline fun <T> call(action: () -> T): T {
        contract {
            callsInPlace(action, InvocationKind.EXACTLY_ONCE)
        }
        rate.inc()
        val start = System.nanoTime()
        return try {
            action.invoke()
        } finally {
            histogram.record((System.nanoTime() - start) / div)
        }
    }
}
