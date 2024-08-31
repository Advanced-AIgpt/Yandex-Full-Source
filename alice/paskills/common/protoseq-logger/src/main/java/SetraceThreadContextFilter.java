package ru.yandex.alice.paskills.common.logging.protoseq;

import java.util.Collections;
import java.util.List;
import java.util.Map;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.config.Configuration;
import org.apache.logging.log4j.core.config.Node;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginConfiguration;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.filter.AbstractFilter;
import org.apache.logging.log4j.util.ReadOnlyStringMap;

@Plugin(
        name = "SetraceThreadContextFilter",
        category = Node.CATEGORY,
        elementType = Filter.ELEMENT_TYPE,
        printObject = true
)
public class SetraceThreadContextFilter extends AbstractFilter {

    private static final List<String> REQUIRED_KEYS = ThreadContextKey.stringValues(
            ThreadContextKey.SERVICE_NAME,
            ThreadContextKey.ACTIVATION_ID,
            ThreadContextKey.REQUEST_ID,
            ThreadContextKey.REQUEST_TIMESTAMP
    );

    private static final Map<Level, List<String>> REQUIRED_KEYS_PER_LOG_LEVEL = Map.of(
            LogLevels.ACTIVATION_STARTED, Collections.emptyList(),
            LogLevels.ACTIVATION_FINISHED, Collections.emptyList(),
            LogLevels.CHILD_ACTIVATION_STARTED, ThreadContextKey.stringValues(
                    ThreadContextKey.CHILD_ACTIVATION_ID,
                    ThreadContextKey.CHILD_ACTIVATION_DESCRIPTION
            ),
            LogLevels.CHILD_ACTIVATION_FINISHED, ThreadContextKey.stringValues(
                    ThreadContextKey.CHILD_ACTIVATION_ID,
                    ThreadContextKey.CHILD_ACTIVATION_RESULT_OK
            )
    );

    @PluginFactory
    public static SetraceThreadContextFilter create(@PluginConfiguration final Configuration config) {
        return new SetraceThreadContextFilter();
    }

    @Override
    public Result filter(final LogEvent event) {
        ReadOnlyStringMap context = event.getContextData();
        return (
                REQUIRED_KEYS.stream().allMatch(context::containsKey) &&
                REQUIRED_KEYS_PER_LOG_LEVEL.getOrDefault(event.getLevel(), Collections.emptyList())
                        .stream()
                        .allMatch(context::containsKey)
        )
                ? Result.ACCEPT
                : Result.DENY;
    }

}
