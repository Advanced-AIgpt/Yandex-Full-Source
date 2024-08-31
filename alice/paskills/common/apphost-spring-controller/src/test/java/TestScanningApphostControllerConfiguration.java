package ru.yandex.alice.paskills.common.apphost.spring;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import NAppHostHttp.Http;
import NAppHostHttp.Http.THttpRequest;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.protobuf.Any;
import com.google.protobuf.ByteString;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.StringValue;
import com.google.protobuf.Struct;
import com.google.protobuf.Value;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.stub.StreamObserver;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;

import ru.yandex.alice.paskills.common.apphost.http.HttpRequest;
import ru.yandex.alice.paskills.common.apphost.http.HttpResponse;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.web.apphost.api.AppHostService;
import ru.yandex.web.apphost.api.compression.Compression;
import ru.yandex.web.apphost.api.format.Format;
import ru.yandex.web.apphost.api.request.ApphostRequest;
import ru.yandex.web.apphost.api.request.ApphostResponseBuilder;
import ru.yandex.web.apphost.grpc.proto.TAnswer;
import ru.yandex.web.apphost.grpc.proto.TServantGrpc;
import ru.yandex.web.apphost.grpc.proto.TServiceRequest;
import ru.yandex.web.apphost.grpc.proto.TServiceResponse;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static ru.yandex.alice.paskills.common.apphost.http.HttpRequest.Method.POST;
import static ru.yandex.alice.paskills.common.apphost.http.HttpRequest.Scheme.HTTP;
import static ru.yandex.alice.paskills.common.apphost.spring.HandlerScanner.HTTP_REQUEST_APPHOST_KEY;
import static ru.yandex.alice.paskills.common.apphost.spring.HandlerScanner.HTTP_RESPONSE_APPHOST_KEY;

@SpringBootTest(classes = {
        ScanningApphostControllerConfiguration.class,
        ApphostServiceConfiguration.class,
        TestScanningApphostControllerConfiguration.TestApphostController.class,
        ObjectMapper.class,
        HandlerScanner.class,
        MetricRegistry.class
})
class TestScanningApphostControllerConfiguration {

    @Autowired
    private AppHostService appHostService;

    @Test
    void testEmptyRequestMethodForNullableArgument() throws InterruptedException {


        List<TServiceResponse> responses = call("/foo1");

        assertEquals(1, responses.size());
        TAnswer answer = responses.get(0).getAnswers(0);
        Any any = Format.parse(Compression.decompress(answer.getData()), Any.class, Any.getDefaultInstance(), false);

        assertEquals(Any.pack(Struct.getDefaultInstance()), any);

    }

    @Test
    void testRequestWithRequiredArgument() throws InterruptedException {


        Struct s = Struct.newBuilder().putFields("a", Value.newBuilder().setStringValue("b").build()).build();
        List<TServiceResponse> responses = call("/foo2", makeAnswer("f1", s));

        assertEquals(1, responses.size());
        TAnswer answer = responses.get(0).getAnswers(0);
        Any any = Format.parse(Compression.decompress(answer.getData()), Any.class, Any.getDefaultInstance(), false);

        assertEquals(Any.pack(s), any);

    }

    @Test
    void testRequestWithMissingArgument() throws InterruptedException {

        List<TServiceResponse> responses = call("/foo2");

        assertEquals(1, responses.size());
        assertEquals("Expected only one item of type f1 but 0 items provided",
                responses.get(0).getException().toStringUtf8());
        assertEquals(List.of(), responses.get(0).getAnswersList());
    }

    @Test
    void testNullResult() throws InterruptedException {
        Struct s = Struct.newBuilder().putFields("a", Value.newBuilder().setStringValue("b").build()).build();
        List<TServiceResponse> responses = call("/nullResult", /*ignored*/makeAnswer("f1", s));

        assertEquals(1, responses.size());
        assertEquals(1, responses.get(0).getAnswersCount());
        assertEquals("!nothing", responses.get(0).getAnswers(0).getType());
    }

    @Test
    void testVoid() throws InterruptedException {
        Struct s = Struct.newBuilder().putFields("a", Value.newBuilder().setStringValue("b").build()).build();
        List<TServiceResponse> responses = call("/void", /*ignored*/makeAnswer("f1", s));

        assertEquals(1, responses.size());
        assertEquals(1, responses.get(0).getAnswersCount());
        assertEquals("!nothing", responses.get(0).getAnswers(0).getType());
    }

    @Test
    void testHttpRequestResult() throws InterruptedException {
        List<TServiceResponse> responses = call("/http_request_result");

        assertEquals(1, responses.size());
        assertEquals(1, responses.get(0).getAnswersCount());
        TAnswer answer = responses.get(0).getAnswers(0);
        assertEquals(HTTP_REQUEST_APPHOST_KEY, answer.getType());

        THttpRequest actual = Format.parse(Compression.decompress(answer.getData()),
                THttpRequest.class, THttpRequest.getDefaultInstance(), false);

        var expected = THttpRequest.newBuilder()
                .setMethod(THttpRequest.EMethod.Post)
                .setScheme(THttpRequest.EScheme.Http)
                .setPath("/somepath")
                .addHeaders(Http.THeader.newBuilder().setName("Content-Length").setValue("9").build())
                .addHeaders(Http.THeader.newBuilder().setName("Content-Type").setValue("application/json").build())
                .setContent(ByteString.copyFromUtf8("{\"s\":\"s\"}"))
                .build();

        assertEquals(expected, actual);

    }

    @Test
    void testHttpRequestResultCustomKey() throws InterruptedException {
        List<TServiceResponse> responses = call("/http_request_result/custom_key");

        assertEquals(1, responses.size());
        assertEquals(1, responses.get(0).getAnswersCount());
        TAnswer answer = responses.get(0).getAnswers(0);
        assertEquals("http_request_custom", answer.getType());

        THttpRequest actual = Format.parse(Compression.decompress(answer.getData()),
                THttpRequest.class, THttpRequest.getDefaultInstance(), false);

        var expected = THttpRequest.newBuilder()
                .setMethod(THttpRequest.EMethod.Post)
                .setScheme(THttpRequest.EScheme.Http)
                .setPath("/somepath")
                .addHeaders(Http.THeader.newBuilder().setName("Content-Length").setValue("9").build())
                .addHeaders(Http.THeader.newBuilder().setName("Content-Type").setValue("application/json").build())
                .setContent(ByteString.copyFromUtf8("{\"s\":\"s\"}"))
                .build();

        assertEquals(expected, actual);

    }

    @Test
    void testHttpResponseArgument() throws InterruptedException {
        Http.THttpResponse responseArg = Http.THttpResponse.newBuilder()
                .setStatusCode(200)
                .setFromHttpProxy(true)
                .addHeaders(Http.THeader.newBuilder().setName("Content-Length").setValue("9").build())
                .addHeaders(Http.THeader.newBuilder().setName("Content-Type").setValue("application/json").build())
                .setContent(ByteString.copyFromUtf8("{\"s\":\"s\"}"))
                .build();
        List<TServiceResponse> responses = call("/http_response_argument",
                makeAnswer(HTTP_RESPONSE_APPHOST_KEY, responseArg));

        assertEquals(1, responses.size());
        assertEquals(1, responses.get(0).getAnswersCount());

        TAnswer answer = responses.get(0).getAnswers(0);
        assertEquals("dummy", answer.getType());
        StringValue actual = Format.parse(Compression.decompress(answer.getData()),
                StringValue.class, StringValue.getDefaultInstance(), false);

        // same as in initial "s" field
        assertEquals("s", actual.getValue());
    }

    @Test
    void testHttpResponseArgumentWithCustomKey() throws InterruptedException {
        Http.THttpResponse responseArg = Http.THttpResponse.newBuilder()
                .setStatusCode(200)
                .setFromHttpProxy(true)
                .addHeaders(Http.THeader.newBuilder().setName("Content-Length").setValue("9").build())
                .addHeaders(Http.THeader.newBuilder().setName("Content-Type").setValue("application/json").build())
                .setContent(ByteString.copyFromUtf8("{\"s\":\"s\"}"))
                .build();
        List<TServiceResponse> responses = call("/http_response_argument/custom_key",
                makeAnswer("http_response_custom", responseArg));

        assertEquals(1, responses.size());
        assertEquals(1, responses.get(0).getAnswersCount());

        TAnswer answer = responses.get(0).getAnswers(0);
        assertEquals("dummy", answer.getType());
        StringValue actual = Format.parse(Compression.decompress(answer.getData()),
                StringValue.class, StringValue.getDefaultInstance(), false);

        // same as in initial "s" field
        assertEquals("s", actual.getValue());
    }

    @Test
    void testWithApphostKeyContainer() throws InterruptedException {
        Struct s = Struct.newBuilder().putFields("a", Value.newBuilder().setStringValue("b").build()).build();
        List<TServiceResponse> responses = call("/apphostKeyContainer",
                makeAnswer("f1", StringValue.of("f1")),
                makeAnswer("datasource_USER_LOCATION", s));

        assertEquals(1, responses.size());
        TAnswer answer = responses.get(0).getAnswers(0);
        StringValue actual = Format.parse(Compression.decompress(answer.getData()), StringValue.class,
                StringValue.getDefaultInstance(), false);

        assertEquals(StringValue.of("""
                        DataSourceApphostContainer[userLocation=fields {
                          key: "a"
                          value {
                            string_value: "b"
                          }
                        }
                        , begemotExternalMarkup=null, userInfo=null]"""),
                actual);

    }

    @Test
    void requestHandlerWithApphostRequestArgument() throws InterruptedException {
        Struct s = Struct.newBuilder().putFields("a", Value.newBuilder().setStringValue("b").build()).build();
        List<TServiceResponse> responses = call("/apphostRequest",
                makeAnswer("f1", StringValue.of("f1")),
                makeAnswer("datasource_USER_LOCATION", s));

        assertEquals(1, responses.size());
        TAnswer answer = responses.get(0).getAnswers(0);
        StringValue actual = Format.parse(Compression.decompress(answer.getData()), StringValue.class,
                StringValue.getDefaultInstance(), false);

        assertEquals(StringValue.of("f1,datasource_USER_LOCATION"), actual);

    }

    @Test
    void requestHandlerWithApphostResponseBuilderArgument() throws InterruptedException {
        List<TServiceResponse> responses = call("/apphostResponseBuilder");

        assertEquals(1, responses.size());
        assertEquals(2, responses.get(0).getAnswersCount());
        TAnswer answer1 = responses.get(0).getAnswers(0);
        assertEquals("f1", answer1.getType());
        Struct actual1 = Format.parse(Compression.decompress(answer1.getData()), Struct.class,
                Struct.getDefaultInstance(), false);

        assertEquals("""
                        fields {
                          key: "a"
                          value {
                            string_value: "b"
                          }
                        }
                        """,
                actual1.toString());

        TAnswer answer2 = responses.get(0).getAnswers(1);
        assertEquals("f2", answer2.getType());
        Struct actual2 = Format.parse(Compression.decompress(answer2.getData()), Struct.class,
                Struct.getDefaultInstance(), false);

        assertEquals("""
                        fields {
                          key: "c"
                          value {
                            string_value: "d"
                          }
                        }
                        """,
                actual2.toString());
    }

    private TAnswer makeAnswer(String name, GeneratedMessageV3 data) {
        ByteBuffer formattedData = Format.format(data,
                "unused_by_protobuf_formatter",
                Format.PROTOBUF, false);
        return TAnswer.newBuilder()
                .setType(name)
                .setData(Compression.NULL.compress(formattedData))
                .build();
    }

    private List<TServiceResponse> call(String path, TAnswer... answers) throws InterruptedException {
        List<TServiceResponse> responses = new java.util.ArrayList<>();
        CountDownLatch latch = new CountDownLatch(1);
        StreamObserver<TServiceResponse> responseObserver = new StreamObserver<>() {
            @Override
            public void onNext(TServiceResponse response) {
                responses.add(response);
            }

            @Override
            public void onError(Throwable t) {
                latch.countDown();
                throw new IllegalStateException("Unexpected error", t);
            }

            @Override
            public void onCompleted() {
                latch.countDown();
            }
        };

        ManagedChannel channel =
                ManagedChannelBuilder.forAddress("localhost", appHostService.getPort()).usePlaintext().build();

        TServantGrpc.TServantStub client = TServantGrpc.newStub(channel);
        StreamObserver<TServiceRequest> requestObserver = client.invoke(responseObserver);

        TServiceRequest request = TServiceRequest.newBuilder()
                .setPath(path)
                .addAllAnswers(List.of(answers))
                .build();
        requestObserver.onNext(request);
        requestObserver.onCompleted();

        assertTrue(latch.await(5, TimeUnit.SECONDS));

        channel.shutdownNow();

        return responses;
    }

    @TestConfiguration
    @ApphostController
    static class TestApphostController {

        @ApphostHandler("/foo1")
        @ApphostKey("f2")
        public Any method1(@Nullable @ApphostKey("f1") Struct f1) {
            return Any.pack(Objects.requireNonNullElse(f1, Struct.getDefaultInstance()));
        }

        @ApphostHandler("/foo2")
        public TestResponse method2(@ApphostKey("f1") Struct f1) {
            return new TestResponse(Any.pack(f1));
        }


        @ApphostHandler("/nullResult")
        @Nullable
        public TestResponse nullResult() {
            return null;
        }

        @ApphostHandler("/void")
        public void voidMethod() {
        }

        @ApphostHandler("/http_request_result")
        HttpRequest<DummyData> httpRequestResult() {
            return new HttpRequest<>(POST, HTTP, "/somepath", Map.of(),
                    new DummyData("s"), null);
        }

        @ApphostHandler("/http_request_result/custom_key")
        @ApphostKey("http_request_custom")
        HttpRequest<DummyData> httpRequestResultCustomKey() {
            return HttpRequest.<DummyData>builder("/somepath")
                    .method(POST)
                    .scheme(HTTP)
                    .content(new DummyData("s"))
                    .build();
        }

        @ApphostHandler("/http_response_argument")
        @ApphostKey("dummy")
        StringValue httpResponseArgument(HttpResponse<DummyData> r) {
            return StringValue.of(r.content().s());
        }

        @ApphostHandler("/http_response_argument/custom_key")
        @ApphostKey("dummy")
        StringValue httpResponseArgumentCustomKey(@ApphostKey("http_response_custom") HttpResponse<DummyData> r) {
            return StringValue.of(r.content().s());
        }

        @ApphostHandler("/apphostKeyContainer")
        @ApphostKey("dummy")
        StringValue httpResponseArgumentCustomKey(DataSourceApphostContainer r) {
            return StringValue.of(r.toString());
        }

        @ApphostHandler("/apphostRequest")
        @ApphostKey("dummy")
        StringValue apphostRequest(ApphostRequest r) {
            return StringValue.of(r.getRequestItems()
                    .stream()
                    .map(item -> item.getType())
                    .collect(Collectors.joining(","))
            );
        }

        @ApphostHandler("/apphostResponseBuilder")
        @ApphostKey("dummy")
        void apphostResponseBuilder(ApphostResponseBuilder r) {
            Struct f1 = Struct.newBuilder().putFields("a", Value.newBuilder().setStringValue("b").build()).build();
            Struct f2 = Struct.newBuilder().putFields("c", Value.newBuilder().setStringValue("d").build()).build();
            r.addProtobufItem("f1", f1);
            r.addProtobufItem("f2", f2);
        }
    }

    record DummyData(String s) {
    }

    record TestResponse(@ApphostKey("f3") Any f3) {
    }
}
