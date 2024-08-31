package ru.yandex.alice.library.protobufutils;

import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.RunnerException;
import org.openjdk.jmh.runner.options.OptionsBuilder;

public final class AllBenchmarks {
    private AllBenchmarks() {
        throw new UnsupportedOperationException();
    }

    public static void main(String[] args) throws RunnerException {
        var opt = new OptionsBuilder()
                .include(FromMessageConverterBenchmark.class.getSimpleName())
                .include(ObjectToStructBenchmark.class.getSimpleName())
                .include(StructToObjectBenchmark.class.getSimpleName())
                .include(StructToMapBenchmark.class.getSimpleName())
                .build();
        new Runner(opt).run();
    }
}
