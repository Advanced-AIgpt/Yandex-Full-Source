package ru.yandex.alice.paskill.dialogovo.webhook.client;

import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.websocket.server.PathParam;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.boot.test.context.TestConfiguration;
import org.springframework.boot.web.server.LocalServerPort;
import org.springframework.context.annotation.Bean;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.servlet.view.RedirectView;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.paskill.dialogovo.config.TestConfigProvider;

import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.fail;

/**
 * Test for {@link WebhookRestTemplateConfiguration}
 */
@SpringBootTest(
        classes = {
                TestConfigProvider.class,
                WebhookRestTemplateConfigurationTest.DummyControllerTestConfig.class
        },
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT
)
class WebhookRestTemplateConfigurationTest {

    @Autowired
    @Qualifier("webhookRestTemplate")
    private RestTemplate webhookRestTemplate;

    @LocalServerPort
    private int port;

    @Test
    @DisplayName("RestTemplate не должен хранить куки")
    void testCookieIsDisable() {
        webhookRestTemplate.getForObject(getUri("cookie").build().toUri(), Void.class);
        webhookRestTemplate.getForObject(getUri("cookie_absent_checker").build().toUri(), Void.class);
    }

    @Test
    @DisplayName("RestTemplate не должен ходить по редиректам")
    void testRedirectIsDisable() {
        webhookRestTemplate.getForObject(
                getUri("redirect")
                        .queryParam("redirectTo", getUri("redirect_with_fail").build().toString())
                        .build()
                        .toUri(),
                Void.class
        );
    }

    private UriComponentsBuilder getUri(String path) {
        return UriComponentsBuilder.fromUriString("http://localhost:" + port)
                .path(path);
    }

    @TestConfiguration
    static class DummyControllerTestConfig {
        @Bean
        DummyController dummySkillController() {
            return new DummyController();
        }
    }

    @RestController()
    private static class DummyController {
        @GetMapping("/cookie")
        public void cookie(HttpServletResponse response) {
            response.addCookie(new Cookie("data", "cookie_data"));
        }

        @GetMapping("/cookie_absent_checker")
        public void cookieAbsentChecker(HttpServletRequest request) {
            assertNull(request.getCookies(), "We shouldn't send any cookie");
        }

        @GetMapping("/redirect")
        public RedirectView redirect(@PathParam("redirectTo") String redirectTo) {
            return new RedirectView(redirectTo);
        }

        @GetMapping("/redirect_with_fail")
        public void redirectWithFail() {
            fail("The redirect url shouldn't be call");
        }
    }
}
