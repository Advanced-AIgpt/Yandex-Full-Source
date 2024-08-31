package ru.yandex.alice.paskill.dialogovo.processor;

import java.net.URI;
import java.util.Base64;
import java.util.List;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Data;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import ru.yandex.alice.kronstadt.core.ScenarioMeta;
import ru.yandex.alice.kronstadt.core.directive.CallbackDirective;
import ru.yandex.alice.kronstadt.core.directive.Directive;
import ru.yandex.alice.paskill.dialogovo.midleware.DialogovoRequestContext;

@Component
public class DirectiveToDialogUriConverter {

    private final ObjectMapper objectMapper;
    private final DialogovoRequestContext dialogovoRequestContext;

    public DirectiveToDialogUriConverter(ObjectMapper objectMapper,
                                         DialogovoRequestContext dialogovoRequestContext) {
        this.objectMapper = objectMapper;
        this.dialogovoRequestContext = dialogovoRequestContext;
    }

    public URI convertDirectives(List<CallbackDirective> directives) throws JsonProcessingException {
        return convertDirectives(directives, null);
    }

    public URI convertDirectives(
            List<CallbackDirective> directives,
            @Nullable String dialogId
    ) throws JsonProcessingException {
        UriComponentsBuilder builder = UriComponentsBuilder.newInstance();
        builder.scheme("dialog");
        builder.host("");
        if (dialogId != null) {
            builder.queryParam("dialog_id", dialogId);
        }
        if (!directives.isEmpty()) {

            var transformedDirectives = directives.stream()
                    .map(this::wrapCallbackDirective)
                    .collect(Collectors.toList());

            String jsonSerializedDirectives = objectMapper.writeValueAsString(transformedDirectives);
            builder.queryParam("directives", jsonSerializedDirectives);
        } else {
            // URI to be valid with empty host needs a question mark. so we put an empty parameter
            builder.queryParam("");
        }
        return builder.build().toUri();
    }

    public CallbackDirectiveWrapper wrapCallbackDirective(CallbackDirective callbackDirective) {

        Directive annotation = callbackDirective.getClass().getAnnotation(Directive.class);
        ScenarioMeta scenario = dialogovoRequestContext.getScenario();

        ObjectNode node = objectMapper.valueToTree(callbackDirective);
        node.put("@scenario_name", scenario != null ? scenario.getMegamindName() : "Dialogovo");

        return new CallbackDirectiveWrapper(annotation.value(), annotation.ignoreAnswer(), node);
    }

    public String wrapCallbackDirectiveToBase64(CallbackDirective callbackDirective) {
        CallbackDirectiveWrapper wrapper = wrapCallbackDirective(callbackDirective);
        try {
            var jsonBytes = objectMapper.writeValueAsBytes(List.of(wrapper));
            return Base64.getEncoder().encodeToString(jsonBytes);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    @Data
    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    public static class CallbackDirectiveWrapper {
        private final String name;
        //@JsonProperty("@scenario_name")
        //private final String scenarioName;
        private final String type = "server_action";
        @JsonProperty("ignore_answer")
        private final boolean ignoreAnswer;
        private final ObjectNode payload;
    }

}
