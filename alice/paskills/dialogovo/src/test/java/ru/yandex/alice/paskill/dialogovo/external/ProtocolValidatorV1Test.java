package ru.yandex.alice.paskill.dialogovo.external;

import java.io.IOException;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

import ru.yandex.alice.paskill.dialogovo.external.v1.ProtocolValidatorV1;

@Disabled("until proper implementation of ProtocolValidator")
public class ProtocolValidatorV1Test {
    private ProtocolValidator validator = new ProtocolValidatorV1();

    @ParameterizedTest(name = "validateJson #{index} file: [{arguments}]")
    @ValueSource(strings = {
            "invalid_end_session_1",
            "invalid_end_session_2",
            "invalid_end_session_3",
            "invalid_session_1",
            "invalid_text_1",
            "invalid_text_2",
            "invalid_text_3",
            "invalid_text_4",
            "invalid_version_1",
            "invalid_version_2",
            "invalid_version_3",
            "invalid_version_4",
            "valid_buttons_1",
            "valid_1",
            "valid_2",
    })
    void validateJson(String input) throws IOException {
        // var isValid = input.split("_")[0].equals("valid");

        // var json = ResourceUtils.getStringResource("validation/" + input + ".json");
        // var res = validator.validate(json);

        // assertEquals(isValid, res.isValid());
    }
}
