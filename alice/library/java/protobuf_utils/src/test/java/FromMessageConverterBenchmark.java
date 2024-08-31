package ru.yandex.alice.library.protobufutils;

import java.util.concurrent.TimeUnit;

import com.fasterxml.jackson.databind.DeserializationFeature;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
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
import static ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA_PROTO;

/**
 * Benchmark                                                     Mode  Cnt      Score   Error  Units
 * FromMessageConverterBenchmark.benchMessageToMapDirect         avgt        3056.557          ns/op
 * FromMessageConverterBenchmark.benchMessageToObjectNodeDirect  avgt        2784.034          ns/op
 * FromMessageConverterBenchmark.benchMessageToStructDirect      avgt        3686.738          ns/op
 * FromMessageConverterBenchmark.benchMessageToStructViaJson     avgt       19112.253          ns/op
 */
@Fork(value = 1)
@Warmup(iterations = 1)
@Measurement(iterations = 1)
@BenchmarkMode(Mode.AverageTime)
@State(Scope.Benchmark)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
public class FromMessageConverterBenchmark {
    private final FromMessageConverter converter = FromMessageConverter.DEFAULT_CONVERTER;
    private final JacksonToProtoConverter protoUtil = new JacksonToProtoConverter(jacksonObjectMapper().copy()
            .disable(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES));
    private final JsonFormat.Printer protoJsonPrinter = JsonFormat.printer();


    @Benchmark
    public void benchMessageToStructDirect(Blackhole bh) {
        bh.consume(converter.convertToStruct(TEST_DATA_PROTO));
    }

    @Benchmark
    public void benchMessageToMapDirect(Blackhole bh) {
        bh.consume(converter.convertToMap(TEST_DATA_PROTO));
    }

    @Benchmark
    public void benchMessageToObjectNodeDirect(Blackhole bh) {
        bh.consume(converter.convertToObjectNode(TEST_DATA_PROTO));
    }

    @Benchmark
    public void benchMessageToStructViaJson(Blackhole bh) throws InvalidProtocolBufferException {
        bh.consume(protoUtil.jsonStringToStruct(protoJsonPrinter.print(TEST_DATA_PROTO)));
    }

}
