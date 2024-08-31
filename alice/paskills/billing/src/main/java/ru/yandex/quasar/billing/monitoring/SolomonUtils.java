package ru.yandex.quasar.billing.monitoring;

import java.io.OutputStream;
import java.util.Arrays;
import java.util.Set;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.common.collect.ImmutableSet;
import org.springframework.http.HttpHeaders;

import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.encode.MetricFormat;
import ru.yandex.monlib.metrics.encode.json.MetricJsonEncoder;
import ru.yandex.monlib.metrics.encode.spack.MetricSpackEncoder;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

public final class SolomonUtils {
    private static final int TIMESTAMP_CAP = 0;
    private static final MetricFormat DEFAULT_FORMAT = MetricFormat.JSON;
    private static final String FORMAT_PARAMETER = "format";
    private static final Set<MetricFormat> SUPPORTED_FORMATS =
            ImmutableSet.of(MetricFormat.JSON, MetricFormat.SPACK);

    private SolomonUtils() {
        throw new UnsupportedOperationException("it's util class");
    }

    /**
     * Создает необходимый {@link ru.yandex.monlib.metrics.encode.MetricEncoder} по параметрам в запросе.
     * По умолчанию создается {@link ru.yandex.monlib.metrics.encode.json.MetricJsonEncoder}
     * <p>
     * NB: LZ4 - https://wiki.yandex-team.ru/solomon/api/dataformat/spackv1/
     * ZSTD не работает
     */
    @SuppressWarnings("checkstyle:LeftCurly")
    public static MetricEncoder prepareEncoder(HttpServletRequest request, HttpServletResponse response,
                                               OutputStream out
    ) {
        MetricFormat format = findFormat(request);

        response.setStatus(HttpServletResponse.SC_OK);
        if (format == MetricFormat.SPACK) {
            response.setContentType(format.contentType());
            return new MetricSpackEncoder(
                    TimePrecision.SECONDS,
                    CompressionAlg.LZ4,
                    out
            );
        } else if (format == MetricFormat.JSON) {
            response.setContentType(format.contentType());
            return new MetricJsonEncoder(out);
        } else {
            throw new IllegalStateException("Unsupported format: " + format);
        }
    }

    private static MetricFormat findFormat(HttpServletRequest request) {
        String format = request.getParameter(FORMAT_PARAMETER);
        if (format == null) {
            format = request.getHeader(HttpHeaders.ACCEPT);
            if (format == null) {
                return DEFAULT_FORMAT;
            }
        }

        return Arrays.stream(format.split(","))
                .map(MetricFormat::byContentType)
                .filter(SUPPORTED_FORMATS::contains)
                .findFirst()
                .orElse(DEFAULT_FORMAT);
    }

    public static void dump(MetricRegistry registry, MetricEncoder encoder) {
        registry.supply(TIMESTAMP_CAP, encoder);
    }
}
