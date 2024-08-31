package ru.yandex.alice.memento.controller

import org.springframework.http.HttpHeaders
import ru.yandex.monlib.metrics.encode.MetricEncoder
import ru.yandex.monlib.metrics.encode.MetricFormat
import ru.yandex.monlib.metrics.encode.json.MetricJsonEncoder
import ru.yandex.monlib.metrics.encode.spack.MetricSpackEncoder
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision
import ru.yandex.monlib.metrics.registry.MetricRegistry
import java.io.OutputStream
import javax.servlet.http.HttpServletRequest
import javax.servlet.http.HttpServletResponse

/**
 * Utility класс для работы с Solomon.
 */
internal object SolomonUtils {
    private val SOLOMON_REGISTRY = MetricRegistry()
    private const val TIMESTAMP_CAP = 0
    private val DEFAULT_FORMAT = MetricFormat.JSON
    private const val FORMAT_PARAMETER = "format"
    private val SUPPORTED_FORMATS: Set<MetricFormat> = setOf(MetricFormat.JSON, MetricFormat.SPACK)

    /**
     * Создает необходимый [MetricEncoder] по параметрам в запросе.
     * По умолчанию создается [MetricJsonEncoder]
     *
     *
     * NB: LZ4 - https://wiki.yandex-team.ru/solomon/api/dataformat/spackv1/
     * ZSTD не работает
     */
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
        val format = request.getParameter(FORMAT_PARAMETER) ?: request.getHeader(HttpHeaders.ACCEPT)
        if (format == null) {
            return DEFAULT_FORMAT
        }

        return format.split(",")
            .map { contentType: String? -> MetricFormat.byContentType(contentType) }
            .firstOrNull { o: MetricFormat -> SUPPORTED_FORMATS.contains(o) }
            ?: DEFAULT_FORMAT
    }

    /**
     * Запуск стрима сеносров для [ru.yandex.monlib.metrics.MetricConsumer].
     *
     * @param encoder target sensors consumer
     */
    fun dump(encoder: MetricEncoder) {
        SOLOMON_REGISTRY.supply(TIMESTAMP_CAP.toLong(), encoder)
    }

    fun dump(registry: MetricRegistry, encoder: MetricEncoder) {
        registry.supply(TIMESTAMP_CAP.toLong(), encoder)
    }
}
