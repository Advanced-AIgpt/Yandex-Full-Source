package ru.yandex.alice.library.protobufutils;

import java.util.concurrent.TimeUnit;

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

import static ru.yandex.alice.library.protobufutils.CommonTestBenchData.TEST_DATA_STRUCT;


/**
 * Benchmark                                                 Mode  Cnt      Score   Error  Units
 * StructToObjectBenchmark.benchPrinterPrint                 avgt       13477.579          ns/op
 * StructToObjectBenchmark.benchStructToObjectNode           avgt        1439.154          ns/op
 * StructToObjectBenchmark.benchStructToObjectViaJsonString  avgt       15772.837          ns/op
 * StructToObjectBenchmark.benchStructToObjectViaTree        avgt        3269.990          ns/op
 */
@Fork(value = 1)
@Warmup(iterations = 1)
@Measurement(iterations = 1)
@BenchmarkMode(Mode.AverageTime)
@State(Scope.Benchmark)
@OutputTimeUnit(TimeUnit.NANOSECONDS)
public class StructToObjectBenchmark {
    private final JacksonToProtoConverterProxy jacksonToProtoConverter = new JacksonToProtoConverterProxy();
    private final JsonFormat.Printer printer = JsonFormat.printer();


    @Benchmark
    public void benchStructToObjectViaJsonString(Blackhole bh) {
        bh.consume(jacksonToProtoConverter.structToObjectViaJsonString(
                TEST_DATA_STRUCT, CommonTestBenchData.TestData.class)
        );
    }

    @Benchmark
    public void benchStructToObjectViaTree(Blackhole bh) {
        bh.consume(jacksonToProtoConverter.structToObjectViaTree(
                TEST_DATA_STRUCT, CommonTestBenchData.TestData.class)
        );
    }

    @Benchmark
    public void benchPrinterPrint(Blackhole bh) throws InvalidProtocolBufferException {
        bh.consume(printer.print(TEST_DATA_STRUCT));
    }

    @Benchmark
    public void benchStructToObjectNode(Blackhole bh) {
        bh.consume(jacksonToProtoConverter.structToObjectNode(TEST_DATA_STRUCT));
    }

}
