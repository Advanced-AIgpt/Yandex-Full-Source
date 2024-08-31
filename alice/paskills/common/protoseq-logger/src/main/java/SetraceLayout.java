package ru.yandex.alice.paskills.common.logging.protoseq;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicLong;

import com.google.protobuf.CodedOutputStream;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.config.Configuration;
import org.apache.logging.log4j.core.config.Node;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginAttribute;
import org.apache.logging.log4j.core.config.plugins.PluginConfiguration;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.layout.AbstractLayout;
import org.apache.logging.log4j.core.time.Instant;
import org.apache.logging.log4j.util.ReadOnlyStringMap;

import ru.yandex.alice.library.rtlog.RtlogEv;
import ru.yandex.alice.library.setrace.Log;

@Plugin(name = "SetraceLayout", category = Node.CATEGORY, elementType = Layout.ELEMENT_TYPE, printObject = true)
public class SetraceLayout extends AbstractLayout<LogEvent> {

    private static final Logger logger = LogManager.getLogger();

    private static final byte[] MAGIC = new byte[]{
            (byte) 0x1F, (byte) 0xF7, (byte) 0xF7, (byte) 0x7E, (byte) 0xBE, (byte) 0xA6, (byte) 0x5E, (byte) 0x9E,
            (byte) 0x37, (byte) 0xA6, (byte) 0xF6, (byte) 0x2E, (byte) 0xFE, (byte) 0xAE, (byte) 0x47, (byte) 0xA7,
            (byte) 0xB7, (byte) 0x6E, (byte) 0xBF, (byte) 0xAF, (byte) 0x16, (byte) 0x9E, (byte) 0x9F, (byte) 0x37,
            (byte) 0xF6, (byte) 0x57, (byte) 0xF7, (byte) 0x66, (byte) 0xA7, (byte) 0x06, (byte) 0xAF, (byte) 0xF7
    };
    private static final byte[] EMPTY_BYTES = {};
    private static final AtomicLong EVENT_INDEX = new AtomicLong();

    private final boolean validateProto;
    private final boolean printDebugToStderr;
    private final boolean addMagicAndLength;

    public SetraceLayout(Configuration configuration, boolean validateProto, boolean printDebugToStderr,
                         boolean addMagicAndLength) {
        super(configuration, EMPTY_BYTES, EMPTY_BYTES);
        this.validateProto = validateProto;
        this.printDebugToStderr = printDebugToStderr;
        this.addMagicAndLength = addMagicAndLength;
    }

    @PluginFactory
    public static SetraceLayout create(
            @PluginConfiguration final Configuration config,
            @PluginAttribute(value = "validateProto") final boolean validateProto,
            @PluginAttribute(value = "printDebugToStderr") final boolean printDebugToStderr,
            @PluginAttribute(value = "addMagicAndLength", defaultBoolean = true) final boolean addMagicAndLength
    ) {
        return new SetraceLayout(config, validateProto, printDebugToStderr, addMagicAndLength);
    }

    @Override
    public byte[] toByteArray(LogEvent event) {
        ReadOnlyStringMap eventContext = event.getContextData();
        final Log.TScenarioLog.Builder scenarioLog = Log.TScenarioLog.newBuilder()
                .setReqId(eventContext.getValue(ThreadContextKey.REQUEST_ID.value()))
                .setActivationId(eventContext.getValue(ThreadContextKey.ACTIVATION_ID.value()))
                .setReqTimestamp(Long.parseLong(eventContext.getValue(ThreadContextKey.REQUEST_TIMESTAMP.value())))
                .setTimestamp(microseconds(event.getInstant()))
                .setFrameId(Long.parseLong(eventContext.getValue(ThreadContextKey.FRAME_ID.value())))
                .setInstanceDescriptor(RtlogEv.InstanceDescriptor.newBuilder()
                        .setHostName(InstanceInfo.INSTANCE.getHostname())
                        .setServiceName(getServiceNameOrDefault(event))
                        .build())
                .setEventIndex(EVENT_INDEX.incrementAndGet());
        Level level = event.getLevel();
        if (LogLevels.ACTIVATION_STARTED.equals(level)) {
            scenarioLog.setActivationStarted(writeActivationStarted(event));
        } else if (LogLevels.ACTIVATION_FINISHED.equals(level)) {
            scenarioLog.setActivationFinished(RtlogEv.ActivationFinished.newBuilder().build());
        } else if (LogLevels.CHILD_ACTIVATION_STARTED.equals(level)) {
            scenarioLog.setChildActivationStarted(writeChildActivationStarted(eventContext));
        } else if (LogLevels.CHILD_ACTIVATION_FINISHED.equals(level)) {
            scenarioLog.setChildActivationFinished(writeChildActivationFinished(eventContext));
        } else {
            scenarioLog.setLogEvent(writeLogEvent(event));
        }
        if (printDebugToStderr) {
            System.err.println("Setrace log:\n" + scenarioLog.build().toString());
        }
        byte[] messageBytes = scenarioLog.build().toByteArray();
        int totalSize = messageBytes.length + (addMagicAndLength ? 4 + MAGIC.length : 0);
        byte[] protoseqBytes = new byte[totalSize];
        CodedOutputStream os = CodedOutputStream.newInstance(protoseqBytes);
        try {
            if (addMagicAndLength) {
                os.writeFixed32NoTag(messageBytes.length);
            }

            os.writeRawBytes(messageBytes);

            if (addMagicAndLength) {
                os.writeRawBytes(MAGIC);
            }
            if (validateProto) {
                os.checkNoSpaceLeft();
            }
        } catch (IOException e) {
            logger.error(e);
        }
        return protoseqBytes;
    }

    private RtlogEv.LogEvent writeLogEvent(LogEvent event) {
        var logEventBuilder = RtlogEv.LogEvent.newBuilder()
                .setSeverity(getSeverity(event))
                .setMessage(event.getMessage().getFormattedMessage());
        if (event.getThrown() != null) {
            StringWriter stringWriter = new StringWriter();
            try (PrintWriter printWriter = new PrintWriter(stringWriter)) {
                event.getThrown().printStackTrace(printWriter);
                logEventBuilder.setBacktrace(stringWriter.toString());
            }
        }

        if (event.getMarker() != null &&
                event.getMarker().hasParents() &&
                (Arrays.asList(event.getMarker().getParents())).contains(Setrace.SETRACE_TAG_MARKER_PARENT)) {
            logEventBuilder.putFields("tags", event.getMarker().getName());
        }
        return logEventBuilder.build();
    }

    private RtlogEv.ActivationStarted writeActivationStarted(LogEvent event) {
        ReadOnlyStringMap context = event.getContextData();
        return RtlogEv.ActivationStarted.newBuilder()
                .setReqTimestamp(Long.parseLong(context.getValue(ThreadContextKey.REQUEST_TIMESTAMP.value())))
                .setReqId(context.getValue(ThreadContextKey.REQUEST_ID.value()))
                .setActivationId(context.getValue(ThreadContextKey.ACTIVATION_ID.value()))
                .setPid((int) InstanceInfo.INSTANCE.getPid())
                .setInstanceDescriptor(RtlogEv.InstanceDescriptor.newBuilder()
                        .setHostName(InstanceInfo.INSTANCE.getHostname())
                        .setServiceName(getServiceNameOrDefault(event))
                        .build())
                .build();
    }

    private RtlogEv.ChildActivationStarted writeChildActivationStarted(ReadOnlyStringMap eventContext) {
        return RtlogEv.ChildActivationStarted.newBuilder()
                .setReqId(eventContext.getValue(ThreadContextKey.REQUEST_ID.value()))
                .setReqTimestamp(Long.parseLong(eventContext.getValue(ThreadContextKey.REQUEST_TIMESTAMP.value())))
                .setActivationId(eventContext.getValue(ThreadContextKey.CHILD_ACTIVATION_ID.value()))
                .setDescription(eventContext.getValue(ThreadContextKey.CHILD_ACTIVATION_DESCRIPTION.value()))
                .build();
    }

    private RtlogEv.ChildActivationFinished writeChildActivationFinished(ReadOnlyStringMap eventContext) {
        return RtlogEv.ChildActivationFinished.newBuilder()
                .setActivationId(eventContext.getValue(ThreadContextKey.CHILD_ACTIVATION_ID.value()))
                .setOk("true".equals(eventContext.getValue(ThreadContextKey.CHILD_ACTIVATION_RESULT_OK.value())))
                .build();
    }

    @Override
    public LogEvent toSerializable(LogEvent event) {
        return event;
    }

    @Override
    public String getContentType() {
        return "application/octet-stream";
    }

    private RtlogEv.ESeverity getSeverity(LogEvent event) {
        return event.getLevel().compareTo(Level.WARN) <= 0
                ? RtlogEv.ESeverity.RTLOG_SEVERITY_ERROR
                : RtlogEv.ESeverity.RTLOG_SEVERITY_INFO;
    }

    private String getServiceNameOrDefault(LogEvent event) {
        String serviceName = event.getContextData().getValue(ThreadContextKey.SERVICE_NAME.value());
        return serviceName != null ? serviceName : "unknown_service";
    }

    private long microseconds(Instant instant) {
        return instant.getEpochSecond() * 1_000_000 + instant.getNanoOfSecond() / 1000;
    }
}
