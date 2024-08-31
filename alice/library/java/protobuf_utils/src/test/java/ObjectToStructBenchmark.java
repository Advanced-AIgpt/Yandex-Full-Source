package ru.yandex.alice.library.protobufutils;

import java.util.Map;
import java.util.concurrent.TimeUnit;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.BenchmarkMode;
import org.openjdk.jmh.annotations.Fork;
import org.openjdk.jmh.annotations.Measurement;
import org.openjdk.jmh.annotations.Mode;
import org.openjdk.jmh.annotations.OutputTimeUnit;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.State;
import org.openjdk.jmh.annotations.Warmup;
import org.openjdk.jmh.infra.Blackhole;

import static com.fasterxml.jackson.module.kotlin.ExtensionsKt.jacksonObjectMapper;
import static ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA;

/**
 * Benchmark                                                         Mode  Cnt      Score   Error  Units
 * ObjectToStructBenchmark.benchObjectToStructViaString     avgt       11940.410          ns/op
 * ObjectToStructBenchmark.benchObjectToStructViaTree       avgt        3409.144          ns/op
 * ObjectToStructBenchmark.mapperValueToMap                 avgt        2115.402          ns/op
 * ObjectToStructBenchmark.mapperValueToTree                avgt        1597.407          ns/op
 * ObjectToStructBenchmark.mapperWriteObjectAsString        avgt         692.327          ns/op
 */

@Fork(value = 1)
@Warmup(iterations = 1)
@Measurement(iterations = 1)
@BenchmarkMode(Mode.AverageTime)
@State(Scope.Benchmark)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
public class ObjectToStructBenchmark {

    private final ObjectMapper mapper = jacksonObjectMapper().copy()
            .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES);
    private final JacksonToProtoConverterProxy protoUtil = new JacksonToProtoConverterProxy();
    private final TypeReference<Map<String, Object>> ref = new TypeReference<Map<String, Object>>() {
    };

    @Benchmark
    public void benchObjectToStructViaTree(Blackhole bh) {
        bh.consume(protoUtil.objectToStructViaTree(TEST_DATA));
    }

    @Benchmark
    public void benchObjectToStructViaString(Blackhole bh) {
        bh.consume(protoUtil.objectToStructViaString(TEST_DATA));
    }

    @Benchmark
    public void mapperWriteObjectAsString(Blackhole bh) throws JsonProcessingException {
        bh.consume(mapper.writeValueAsString(TEST_DATA));
    }

    @Benchmark
    public void mapperValueToTree(Blackhole bh) {
        bh.consume(mapper.<ObjectNode>valueToTree(TEST_DATA));
    }

    @Benchmark
    public void mapperValueToMap(Blackhole bh) {
        bh.consume(mapper.convertValue(TEST_DATA, ref));
    }

}
