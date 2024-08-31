package ru.yandex.quasar.billing.filter;

import java.util.regex.Pattern;

import lombok.Data;

@Data
class PathRule {
    private final Pattern pattern;
    private final String replacement;

    static PathRule createForTemplate(String template) {
        Pattern p = Pattern.compile(
                template
                        // на самом деле заменяет "." на "\.", но из-за особенностей escaping-а выглядит так странно
                        .replaceAll("\\.", "\\\\.")
                        .replaceAll("@", "([^/]+)")
        );

        return new PathRule(p, template);
    }
}
