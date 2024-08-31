package ru.yandex.alice.paskill.dialogovo.megamind.directive;

import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.function.Executable;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.context.annotation.ClassPathScanningCandidateComponentProvider;
import org.springframework.core.type.filter.AssignableTypeFilter;

import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.Directive;

import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toList;
import static java.util.stream.Collectors.toSet;
import static org.junit.jupiter.api.Assertions.assertAll;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

class CallbackDirectiveTest {

    @Test
    void testCallbackDirectivesAnnotatedWithDirectiveName() {
        ClassPathScanningCandidateComponentProvider scanner = new ClassPathScanningCandidateComponentProvider(false);
        scanner.addIncludeFilter(new AssignableTypeFilter(CallbackDirective.class));
        Set<BeanDefinition> beans = scanner.findCandidateComponents("ru.yandex.alice");

        List<Class<CallbackDirective>> classes = beans.stream()
                .map(BeanDefinition::getBeanClassName)
                .map(CallbackDirectiveTest::getClass)
                .collect(toList());
        assertAll(classes.stream()
                .map(c -> () -> checkCallbackClass(c))
        );

        assertAll(classes.stream()
                .collect(groupingBy(c -> c.getAnnotation(Directive.class).value(),
                        mapping(Class::getCanonicalName, toSet())))
                .entrySet()
                .stream()
                .map(entry -> (Executable) () -> checkDuplicates(entry))
                .collect(toList()));

        // check duplicated on DirectiveName

    }

    private void checkDuplicates(Map.Entry<String, Set<String>> entry) {
        assertEquals(1, entry.getValue().size(),
                "Duplicates found for key " + entry.getKey() + ": " +
                        String.join(", ", entry.getValue()));
    }

    private void checkCallbackClass(Class<CallbackDirective> c) {
        assertNotNull(c.getAnnotation(Directive.class),
                "Directive " + c.getCanonicalName() + " is not " + "annotated with `@DirectiveName`");
    }

    private static Class<CallbackDirective> getClass(String className) {
        try {
            return Objects.requireNonNull((Class<CallbackDirective>) Class.forName(className),
                    "Cant find class " + className);
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
    }
}
