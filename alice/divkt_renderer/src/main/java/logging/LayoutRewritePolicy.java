package ru.yandex.alice.divktrenderer.logging;

import org.apache.logging.log4j.core.Core;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.appender.rewrite.RewritePolicy;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginElement;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.config.plugins.validation.constraints.Required;
import org.apache.logging.log4j.core.impl.Log4jLogEvent;
import org.apache.logging.log4j.core.layout.PatternLayout;
import org.apache.logging.log4j.message.SimpleMessage;

/**
 * RewritePolicy для Rewrite аппендера, применяющая layout к сообщению
 */
@Plugin(name = "LayoutRewritePolicy", category = Core.CATEGORY_NAME, elementType = "rewritePolicy", printObject = true)
public class LayoutRewritePolicy implements RewritePolicy {
    private final PatternLayout layout;

    public LayoutRewritePolicy(PatternLayout layout) {
        this.layout = layout;
    }

    @PluginFactory
    public static LayoutRewritePolicy createPolicy(@Required @PluginElement("PatternLayout") PatternLayout layout) {
        return new LayoutRewritePolicy(layout);
    }

    @Override
    public LogEvent rewrite(LogEvent source) {
        return new Log4jLogEvent.Builder(source)
                .setMessage(new SimpleMessage(layout.toSerializable(source)))
                .build();
    }
}
