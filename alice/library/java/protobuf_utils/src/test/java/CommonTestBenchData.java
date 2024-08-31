package ru.yandex.alice.library.protobufutils;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.protobuf.Struct;
import com.google.protobuf.util.Values;

import ru.yandex.alice.library.protobufutils.proto.Obj1;
import ru.yandex.alice.library.protobufutils.proto.ObjList;
import ru.yandex.alice.library.protobufutils.proto.TestDataProto;

public final class CommonTestBenchData {

    public static final TestData TEST_DATA = new TestData(
            true,
            "test",
            new Obj(true),
            List.of("foo", "foo2", "foo3", "foo4"),
            List.of(new Obj(true), new Obj(true), new Obj(true), new Obj(true)),
            List.of(
                    new ObjArr(List.of(new Obj(true), new Obj(true), new Obj(true), new Obj(true))),
                    new ObjArr(List.of(new Obj(true), new Obj(true), new Obj(true), new Obj(true))),
                    new ObjArr(List.of(new Obj(true), new Obj(true), new Obj(true), new Obj(true))),
                    new ObjArr(List.of(new Obj(true), new Obj(true), new Obj(true), new Obj(true)))
            )
    );
    public static final Struct TEST_DATA_STRUCT = Struct.newBuilder()
            .putFields("a", Values.of(true))
            .putFields("b", Values.of("test"))
            .putFields("c", Values.of(Struct.newBuilder().putFields("z", Values.of(true)).build()))
            .putFields("d", Values.of(Stream.of("foo", "foo2", "foo3", "foo4").map(Values::of).toList()))
            .putFields("e",
                    Values.of(Stream.of(true, true, true, true)
                            .map(it -> Values.of(Struct.newBuilder()
                                    .putFields("z", Values.of(it))
                                    .build()))
                            .toList()))
            .putFields("f",
                    Values.of(
                            Stream.of(1, 2, 3, 4)
                                    .map(i -> Values.of(Struct.newBuilder()
                                                    .putFields("ff", Values.of(
                                                            Stream.of(true, true, true, true)
                                                                    .map(it -> Values.of(Struct.newBuilder()
                                                                            .putFields("z", Values.of(it))
                                                                            .build()))
                                                                    .toList())
                                                    )
                                                    .build()
                                            )

                                    ).toList()
                    )
            )
            .putFields("g", Values.ofNull())
            .putFields("$h", Values.of(true))
            .build();
    public static final Struct TEST_DATA_STRUCT_WITHOUT_NULLS = TEST_DATA_STRUCT.toBuilder()
            .removeFields("g")
            .build();
    public static final TestDataProto TEST_DATA_PROTO = TestDataProto.newBuilder()
            .setA(true)
            .setB("test")
            .setC(Obj1.newBuilder().setZ(true).build())
            .addAllD(List.of("foo", "foo2", "foo3", "foo4"))
            .addAllE(Stream.of(true, true, true, true).map(it -> Obj1.newBuilder().setZ(it).build()).toList())
            .addAllF(Stream.of(1, 2, 3, 4)
                    .map(i -> ObjList.newBuilder()
                            .addAllFf(Stream.of(true, true, true, true)
                                    .map(it -> Obj1.newBuilder().setZ(it).build())
                                    .toList())
                            .build())
                    .toList())
            //.setG(null)
            .setH(true)
            .build();
    public static final Map<String, Object> TEST_DATA_MAP;
    public static final Map<String, Object> TEST_DATA_MAP_WITHOUT_NULLS;
    public static final ObjectNode TEST_DATA_OBJECT_NODE;
    public static final ObjectNode TEST_DATA_OBJECT_NODE_WITHOUT_NULLS;


    static {
        var tmpMap = new LinkedHashMap<String, Object>();

        tmpMap.put("a", true);
        tmpMap.put("b", "test");
        tmpMap.put("c", Map.of("z", true));
        tmpMap.put("d", List.of("foo", "foo2", "foo3", "foo4"));
        tmpMap.put("e", Stream.of(true, true, true, true).map(it -> Map.of("z", it)).toList());
        tmpMap.put("f", Stream.of(1, 2, 3, 4).map(i ->
                Map.of("ff", Stream.of(true, true, true, true).map(it -> Map.of("z", it)).toList())
        ).toList());
        tmpMap.put("g", null);
        tmpMap.put("$h", true);

        TEST_DATA_MAP = Collections.unmodifiableMap(tmpMap);
        var tmpMap2 = new LinkedHashMap<>(tmpMap);
        tmpMap2.remove("g");
        TEST_DATA_MAP_WITHOUT_NULLS = Collections.unmodifiableMap(tmpMap2);

        TEST_DATA_OBJECT_NODE = new ObjectMapper().valueToTree(TEST_DATA_MAP);
        TEST_DATA_OBJECT_NODE_WITHOUT_NULLS = TEST_DATA_OBJECT_NODE.deepCopy();
        TEST_DATA_OBJECT_NODE_WITHOUT_NULLS.remove("g");
    }

    private CommonTestBenchData() {
        throw new UnsupportedOperationException();
    }

    record TestData(
            boolean a,
            String b,
            Obj c,
            List<String> d,
            List<Obj> e,
            List<ObjArr> f
    ) {
        @JsonProperty("g")
        @Nullable
        public Integer g() {
            return null;
        }

        @JsonProperty("$h")
        public boolean h() {
            return true;
        }
    }

    record ObjArr(List<Obj> ff) {

    }

    record Obj(boolean z) {

    }
}
