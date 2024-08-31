package ru.yandex.alice.paskill.dialogovo.config;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.env.Environment;

import ru.yandex.alice.paskill.dialogovo.scenarios.theremin.ThereminPacksConfig;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;

@SpringBootTest(classes = {TestConfigProvider.class})
class TestConfigParsing {

    @Autowired
    private SearchAppStyles styles;

    @Autowired
    private ThereminPacksConfig thereminPacksConfig;

    @Autowired
    private UserSkillProductConfig productConfig;

    @Test
    void testStylesParse() {
        assertNotNull(styles.getExternal());
    }

    @Test
    void testThereminParse() {
        assertNotNull(thereminPacksConfig.getPacks());
    }

    @Test
    void testProductConfigParse() {
        assertNotNull(productConfig.getMusicIdToTokenCode());
        assertFalse(productConfig.getMusicIdToTokenCode().isEmpty());
    }


    @SpringBootTest(
            classes = AbstractParsingTest.EmptyConfig.class,
            properties = "spring.config.additional-location=classpath:config/dialogovo-config-prod.yaml"
    )
    static class TestProdParsing extends AbstractParsingTest {

        TestProdParsing() {
            super("//home/paskills/skills-to-morning-show/prod");
        }
    }

    @SpringBootTest(
            classes = AbstractParsingTest.EmptyConfig.class,
            properties = "spring.config.additional-location=classpath:config/dialogovo-config-dev.yaml"
    )
    static class DevProdParsing extends AbstractParsingTest {

        DevProdParsing() {
            super("//home/paskills/skills-to-morning-show/dev");
        }

    }

    @SpringBootTest(
            classes = AbstractParsingTest.EmptyConfig.class,
            properties = "spring.config.additional-location=classpath:config/dialogovo-config-priemka.yaml"
    )
    static class PriemkaProdParsing extends AbstractParsingTest {
        PriemkaProdParsing() {
            super("//home/paskills/skills-to-morning-show/priemka");
        }
    }

    abstract static class AbstractParsingTest {

        @Autowired
        private Environment env;

        @Value("${show.yt.directory}")
        private String t;

        private final String expected;

        AbstractParsingTest(String expected) {
            this.expected = expected;
        }

        @Test
        void testPopulateProperties() {
            assertEquals(expected, t);
        }

        @Configuration
        protected static class EmptyConfig {
        }
    }

}
