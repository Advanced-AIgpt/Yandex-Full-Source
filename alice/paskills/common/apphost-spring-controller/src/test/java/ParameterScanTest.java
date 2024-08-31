package ru.yandex.alice.paskills.common.apphost.spring;

import java.util.List;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.protobuf.Any;
import org.junit.jupiter.api.Test;

import ru.yandex.alice.paskills.common.apphost.http.HttpRequest;
import ru.yandex.alice.paskills.common.apphost.http.HttpResponse;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

class ParameterScanTest {


    @Test
    void testUnsupportedParameterType() {
        var scanner = new HandlerScanner(new ObjectMapper());
        assertThrows(RuntimeException.class, () -> scanner.scanHandler(Controller.class));
    }

    static class Controller {
        @ApphostHandler("/foo")
        @ApphostKey("x")
        public Any foo(String f1) {
            throw new UnsupportedOperationException("");
        }
    }

    @Test
    void testReturnHttpRequest() {
        var scanner = new HandlerScanner(new ObjectMapper());
        var result = scanner.scanHandler(Controller2.class);
        assertEquals(1, result.size());
        ApphostHandlerSpecification handler = result.get(0);
        assertEquals("foo", handler.method().getName());
        assertEquals(0, handler.argumentGetters().size());
        assertEquals(1, handler.resultSetters().size());
    }

    static class Controller2 {
        @ApphostHandler("/foo")
        public HttpRequest<String> foo() {
            throw new UnsupportedOperationException("");
        }
    }

    @Test
    void testHttpResponseArgument() {
        var scanner = new HandlerScanner(new ObjectMapper());
        var result = scanner.scanHandler(Controller3.class);
        assertEquals(1, result.size());
        ApphostHandlerSpecification handler = result.get(0);
        assertEquals("foo", handler.method().getName());
        assertEquals(1, handler.argumentGetters().size());
        assertEquals(0, handler.resultSetters().size());
    }

    static class Controller3 {
        @ApphostHandler("/foo")
        public void foo(HttpResponse<String> a) {
            throw new UnsupportedOperationException("");
        }
    }

    @Test
    void testControllerWithBaseUrl() {
        var scanner = new HandlerScanner(new ObjectMapper());
        var result = scanner.scanHandler(Controller4.class);

        assertEquals(2, result.size());

        ApphostHandlerSpecification handler = result.get(0);
        assertEquals("foo", handler.method().getName());
        assertEquals("/base/foo", handler.path());

        ApphostHandlerSpecification handler2 = result.get(1);
        assertEquals("foo2", handler2.method().getName());
        assertEquals("/base/foo2", handler2.path());
    }

    @ApphostHandler("/base")
    static class Controller4 {
        @ApphostHandler("/foo")
        public void foo(HttpResponse<String> a) {
            throw new UnsupportedOperationException("");
        }

        @ApphostHandler("foo2")
        public void foo2(HttpResponse<String> a) {
            throw new UnsupportedOperationException("");
        }
    }

    @Test
    void testUnsupportedKotlinParameterType() {
        var scanner = new HandlerScanner(new ObjectMapper());
        List<ApphostHandlerSpecification> specifications = scanner.scanHandler(Controller5.class);
        assertEquals(1, specifications.size());
    }

    static class Controller5 {
        @ApphostHandler("/foo")
        public void foo(DataSourceApphostContainer a) {
            throw new UnsupportedOperationException("");
        }
    }

    @Test
    void testControllerWithBaseUrlViaApphostController() {
        var scanner = new HandlerScanner(new ObjectMapper());
        var result = scanner.scanHandler(Controller6.class);

        assertEquals(2, result.size());

        ApphostHandlerSpecification handler = result.get(0);
        assertEquals("foo", handler.method().getName());
        assertEquals("/base/foo", handler.path());

        ApphostHandlerSpecification handler2 = result.get(1);
        assertEquals("foo2", handler2.method().getName());
        assertEquals("/base/foo2", handler2.path());
    }

    @ApphostController("/base")
    static class Controller6 {
        @ApphostHandler("/foo")
        public void foo(HttpResponse<String> a) {
            throw new UnsupportedOperationException("");
        }

        @ApphostHandler("foo2")
        public void foo2(HttpResponse<String> a) {
            throw new UnsupportedOperationException("");
        }
    }


}
