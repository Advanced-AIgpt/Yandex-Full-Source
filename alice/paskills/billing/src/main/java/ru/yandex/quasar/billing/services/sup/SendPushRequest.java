package ru.yandex.quasar.billing.services.sup;

import java.util.Collections;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
class SendPushRequest {
    private static final ThrottlePolicies DEFAULT_THROTTLE_POLICIES =
            new ThrottlePolicies("quasar_default_install_id", "quasar_default_device_id");

    private final List<String> receiver;
    private final int ttl = 7200;
    private final Notification notification;
    private final PushData data;
    @JsonProperty("throttle_policies")
    private final ThrottlePolicies throttlePolicies = DEFAULT_THROTTLE_POLICIES;
    private final String project;

    static SendPushRequest create(String receiver, String title, String text, String pushUri, String tag,
                                  String project) {
        return new SendPushRequest(Collections.singletonList(receiver), new Notification(title, text),
                new PushData(pushUri, tag), project);
    }

    @Data
    private static class Notification {
        private final String title;
        private final String body;
    }

    @Data
    private static class PushData {
        @JsonProperty("push_action")
        private final String pushAction = "uri";
        @JsonProperty("push_id")
        private final String pushId = "sup";
        @JsonProperty("push_uri")
        private final String pushUri;
        private final String tag;
    }

    @Data
    private static class ThrottlePolicies {
        @JsonProperty("install_id")
        private final String installId;
        @JsonProperty("device_id")
        private final String deviceid;

    }

}
