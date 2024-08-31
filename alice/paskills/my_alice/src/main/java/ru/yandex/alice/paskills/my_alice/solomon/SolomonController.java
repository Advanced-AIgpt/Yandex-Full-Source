package ru.yandex.alice.paskills.my_alice.solomon;

import java.io.IOException;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.common.io.CountingOutputStream;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.encode.MetricFormat;
import ru.yandex.monlib.metrics.encode.spack.MetricSpackEncoder;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

@RestController
@RequestMapping(path = "/solomon")
public class SolomonController {

    private static final int TIMESTAMP_CAP = 0;
    private static final Logger logger = LogManager.getLogger();

    private final MetricRegistry metricRegistry;

    public SolomonController(MetricRegistry metricRegistry) {
        this.metricRegistry = metricRegistry;
    }

    @GetMapping("")
    public void dumpMetrics(HttpServletRequest request, HttpServletResponse response) throws IOException {
        CountingOutputStream out = new CountingOutputStream(response.getOutputStream());
        response.setContentType(MetricFormat.SPACK.contentType());
        try (MetricEncoder encoder = new MetricSpackEncoder(TimePrecision.SECONDS, CompressionAlg.LZ4, out)) {
            metricRegistry.supply(TIMESTAMP_CAP, encoder);
        } catch (Exception e) {
            logger.error("Error occurred while writing Solomon sensors", e);
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR, e.getMessage());
        }

        response.setContentLengthLong(out.getCount());
    }

}
