package ru.yandex.quasar.billing.config;

import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.regex.Pattern;

import lombok.Getter;

@Getter
public class SecurityConfig {
    private Set<String> corsAllowedDomains = Collections.emptySet();
    //@JsonDeserialize(using = FromStringDeserializer.class)
    private List<Pattern> allowedHostPatterns = Collections.emptyList();
}
