package ru.yandex.alice.library.protobufutils;

import java.util.Map;
import java.util.concurrent.TimeUnit;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.ObjectMapper;
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
import static ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA_STRUCT;

/**
 * Benchmark                                             Mode  Cnt      Score   Error  Units
 * StructToMapBenchmark.benchStructToMapDirect           avgt        1355.864          ns/op
 * StructToMapBenchmark.benchStructToMapViaObjectMapper  avgt       15156.105          ns/op
 */

@Fork(value = 1)
@Warmup(iterations = 1)
@Measurement(iterations = 1)
@BenchmarkMode(Mode.AverageTime)
@State(Scope.Benchmark)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
public class StructToMapBenchmark {

    private final ObjectMapper mapper = jacksonObjectMapper().copy()
            .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES);
    private final JacksonToProtoConverterProxy protoUtil = new JacksonToProtoConverterProxy();
    private final TypeReference<Map<String, Object>> ref = new TypeReference<Map<String, Object>>() {
    };

    @Benchmark
    public void benchStructToMapViaObjectMapper(Blackhole bh) {
        bh.consume(protoUtil.structToMapViaObjectMapper(TEST_DATA_STRUCT));
    }

    @Benchmark
    public void benchStructToMapDirect(Blackhole bh) {
        bh.consume(protoUtil.structToMapDirect(TEST_DATA_STRUCT));
    }

}
