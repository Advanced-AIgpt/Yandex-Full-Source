package ru.yandex.alice.memento.controller;

import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.protobuf.Any;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.TypeRegistry;
import com.google.protobuf.util.JsonFormat;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.test.context.ActiveProfiles;

import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.proto.UserConfigsProto.TMorningShowTopicsConfig;
import ru.yandex.alice.memento.proto.UserConfigsProto.TNewsConfig;
import ru.yandex.alice.memento.settings.SettingsStorage;
import ru.yandex.alice.memento.storage.ydb.TestYdbConfiguration;
import ru.yandex.alice.memento.tvm.TestTvmConfiguration;
import ru.yandex.alice.paskills.common.ydb.YdbClient;
import ru.yandex.alice.protos.data.scenario.music.Topic.TTopic;

@SpringBootTest(classes = {TestTvmConfiguration.class, TestYdbConfiguration.class})
@ActiveProfiles("ut")
@Disabled
public class LoadTestingDataPrepare {
    @Autowired
    private YdbClient ydbClient;
    @Autowired
    private SettingsStorage settingsStorage;

    private JsonFormat.Printer printer;

    @BeforeEach
    void setUp() {
        printer = JsonFormat.printer()
                .usingTypeRegistry(TypeRegistry.newBuilder()
                        .add(TNewsConfig.getDescriptor())
                        .add(TMorningShowTopicsConfig.getDescriptor())
                        .build()
                );
    }

    @Test
    @Disabled
    void setup100UsersForGetters() {
        //YdbTestUtils.clearTable(ydbClient, "user_settings");

        TNewsConfig newsConfig = TNewsConfig.newBuilder().setDefaultSource(
                "abcdefghijklmnopqrstuvwxyz0123456789".repeat(100)).build();

        int lim = 1_000_000;

        var pool = Executors.newFixedThreadPool(50);
        var futures = new ArrayList<CompletableFuture<?>>(lim);

        var doneCnt = new AtomicInteger();
        var init = Map.of(MementoApiProto.EConfigKey.CK_NEWS, Any.pack(newsConfig));
        for (int i = 0; i < lim; i += 5) {
            var i2 = i;
            futures.add(CompletableFuture.runAsync(() ->
                    settingsStorage.updateUserSettings(String.valueOf(i2), init, false), pool)
                    .whenComplete((__, e) -> {
                        var done = doneCnt.incrementAndGet();
                        if (done % 1000 == 0) {
                            System.out.println("first. i=" + done);
                        }
                    })
            );

        }

        CompletableFuture.allOf(futures.toArray(CompletableFuture[]::new)).join();

        var initial = Map.of(
                MementoApiProto.EConfigKey.CK_NEWS,
                Any.pack(newsConfig),
                MementoApiProto.EConfigKey.CK_MORNING_SHOW_TOPICS,
                Any.pack(TMorningShowTopicsConfig.newBuilder()
                        .addTopics(TTopic.newBuilder().setPodcast("podcast1").build())
                        .build())
        );

        var futures2 = new ArrayList<CompletableFuture<?>>(lim);
        var doneCnt2 = new AtomicInteger();
        for (int i = 1; i < lim + 1; i += 5) {
            var i2 = i;
            futures2.add(
                    CompletableFuture.runAsync(() ->
                                    settingsStorage.updateUserSettings(String.valueOf(i2), initial, false),
                            pool)
                            .whenComplete((__, e) -> {
                                var done = doneCnt2.incrementAndGet();
                                if (done % 1000 == 0) {
                                    System.out.println("second. i=" + done);
                                }
                            })
            );
        }

        CompletableFuture.allOf(futures2.toArray(CompletableFuture[]::new)).join();

    }

    @Test
    @Disabled
    void t1() throws InvalidProtocolBufferException {
        TNewsConfig newsConfig = TNewsConfig.newBuilder().setDefaultSource(
                "abcdefghijklmnopqrstuvwxyz0123456789".repeat(100)).build();

        var c = MementoApiProto.TReqChangeUserObjects.newBuilder()
                .addUserConfigs(MementoApiProto.TConfigKeyAnyPair.newBuilder()
                        .setKey(MementoApiProto.EConfigKey.CK_NEWS)
                        .setValue(Any.pack(TNewsConfig.newBuilder()
                                .setDefaultSource("abcdefghijklmnopqrstuvwxyz0123456789".repeat(100))
                                .build())
                        )
                ).addUserConfigs(MementoApiProto.TConfigKeyAnyPair.newBuilder()
                        .setKey(MementoApiProto.EConfigKey.CK_MORNING_SHOW)
                        .setValue(Any.pack(TMorningShowTopicsConfig.newBuilder().build()))
                ).build();

        var s = printer.print(c);
    }
}
