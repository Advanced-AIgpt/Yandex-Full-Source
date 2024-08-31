package ru.yandex.alice.memento.controller;

import java.util.Map;

import com.google.protobuf.Any;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.test.context.ActiveProfiles;

import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.UserConfigsProto.TMorningShowSkillsConfig;
import ru.yandex.alice.memento.proto.UserConfigsProto.TMorningShowSkillsConfig.TSkillProvider;
import ru.yandex.alice.memento.settings.SettingsStorage;
import ru.yandex.alice.memento.storage.ydb.TestYdbConfiguration;
import ru.yandex.alice.memento.tvm.TestTvmConfiguration;

@SpringBootTest(classes = {TestTvmConfiguration.class, TestYdbConfiguration.class})
@ActiveProfiles("ut")
@Disabled
public class ManualInsert {
    @Autowired
    private SettingsStorage settingsStorage;

    @Test
    void createEntry() {
        settingsStorage.updateUserSettings("221498803",
                Map.of(MementoApiProto.EConfigKey.CK_MORNING_SHOW_SKILLS,
                        Any.pack(TMorningShowSkillsConfig.newBuilder()
                                .addSkillProviders(TSkillProvider.newBuilder()
                                        .setSkillSlug("c07d154a-skyeng-dev")
                                        .build())
                                .build()
                        )
                ),
                false
        );
    }
}
