package ru.yandex.alice.paskill.memento_snapshotter;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDate;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

import ru.yandex.inside.yt.kosher.Yt;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.impl.YtConfiguration;
import ru.yandex.inside.yt.kosher.impl.YtUtils;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTree;
import ru.yandex.inside.yt.kosher.transactions.Transaction;
import ru.yandex.inside.yt.kosher.ytree.YTreeNode;
import ru.yandex.yt.ytclient.proxy.request.CreateNode;
import ru.yandex.yt.ytclient.proxy.request.ObjectType;
import ru.yandex.yt.ytclient.tables.ColumnSchema;
import ru.yandex.yt.ytclient.tables.ColumnValueType;

public class MementoSnapshotter {
    private MementoSnapshotter() {
    }

    public static void main(String[] args) {
        if (args.length != 0 && !args[0].equals("-h") && !args[0].equals("--help")) {
            String fromPrefix = args[0];
            String toPrefix = args.length >= 2 ? args[1] : fromPrefix;
            String postFix = args.length >= 3 ? args[2] : LocalDate.now().toString();
            Yt yt = YtUtils.http(
                    YtConfiguration.builder()
                            .withApiHost("hahn.yt.yandex.net")
                            .withToken(getYtToken())
                            .withPortoJava17()
                            .build());
            Transaction transaction = null;
            try {
                transaction = yt.transactions().startAndGet();
                dumpUserSettingsTables(yt, fromPrefix, toPrefix, postFix, false);
                dumpDeviceSettingsTables(yt, fromPrefix, toPrefix, postFix, false);
                dumpUserSettingsTables(yt, fromPrefix, toPrefix, postFix, true);
                dumpDeviceSettingsTables(yt, fromPrefix, toPrefix, postFix, true);
                transaction.commit();
            } catch (Exception e) {
                if (transaction != null) {
                    transaction.abort();
                }
            }
        } else {
            printHelp();
        }
    }

    private static void printHelp() {
        System.out.println(
                "Memento snapshotter\n" +
                        "Usage:\n" +
                        "snapshotter <fromPrefix> <toPrefix> <postFix>\n" +
                        "-h, --help for this message"
        );
    }

    private static void dumpUserSettingsTables(
            Yt yt, String fromPrefix, String toPrefix, String postfix, boolean anonymous
    ) throws IOException {
        Objects.requireNonNull(fromPrefix);
        Objects.requireNonNull(toPrefix);
        Objects.requireNonNull(postfix);
        String tableName = "user_settings" + (anonymous ? "_anonymous" : "");
        String pathForInputUserSettings = fromPrefix + "/" + tableName;
        String pathForOutputUserSettings = toPrefix + "/" + tableName + "/" + postfix;

        YTreeNode schema = YTree.builder().beginAttributes()
                .key("strict").value(true)
                .key("unique_keys").value(false)
                .endAttributes()
                .value(List.of(
                        new ColumnSchema("userId", ColumnValueType.STRING).toYTree(),
                        new ColumnSchema("key", ColumnValueType.STRING).toYTree(),
                        new ColumnSchema("data", ColumnValueType.ANY).toYTree(),
                        YTree.builder()
                                .beginMap()
                                .key("name").value("changedAt")
                                .key("type").value("timestamp")
                                .key("type_v3").value("timestamp")
                                .key("required").value(true)
                                .buildMap()
                )).build();

        yt.cypress().create(new CreateNode(YPath.simple(pathForOutputUserSettings), ObjectType.Table)
                .setRecursive(true)
                .setIgnoreExisting(true)
        );
        yt.tables().alterTable(YPath.simple(pathForOutputUserSettings), Optional.of(false), Optional.of(schema));
        yt.operations()
                .mapAndGetOp(
                        pathForInputUserSettings,
                        pathForOutputUserSettings,
                        new UserSettingsMapper()
                ).awaitAndThrowIfNotSuccess();
    }

    private static void dumpDeviceSettingsTables(
            Yt yt, String fromPrefix, String toPrefix, String postfix, boolean anonymous
    ) throws IOException {
        Objects.requireNonNull(fromPrefix);
        Objects.requireNonNull(toPrefix);
        Objects.requireNonNull(postfix);
        String tableName = "user_device_settings" + (anonymous ? "_anonymous" : "");
        String pathForInputDeviceSettings = fromPrefix + "/" + tableName;
        String pathForOutputDeviceSettings = toPrefix + "/" + tableName + "/" + postfix;

        YTreeNode schema = YTree.builder().beginAttributes()
                .key("strict").value(true)
                .key("unique_keys").value(false)
                .endAttributes()
                .value(List.of(
                        new ColumnSchema("userId", ColumnValueType.STRING).toYTree(),
                        new ColumnSchema("deviceId", ColumnValueType.STRING).toYTree(),
                        new ColumnSchema("key", ColumnValueType.STRING).toYTree(),
                        new ColumnSchema("data", ColumnValueType.ANY).toYTree(),
                        YTree.builder()
                                .beginMap()
                                .key("name").value("changedAt")
                                .key("type").value("timestamp")
                                .key("type_v3").value("timestamp")
                                .key("required").value(true)
                                .buildMap()
                )).build();

        yt.cypress().create(
                new CreateNode(YPath.simple(pathForOutputDeviceSettings), ObjectType.Table)
                        .setRecursive(true)
                        .setIgnoreExisting(true)
        );
        yt.tables().alterTable(YPath.simple(pathForOutputDeviceSettings), Optional.of(false), Optional.of(schema));
        yt.operations()
                .mapAndGetOp(
                        pathForInputDeviceSettings,
                        pathForOutputDeviceSettings,
                        new DeviceSettingsMapper()
                ).awaitAndThrowIfNotSuccess();
    }

    private static String getYtToken() {
        String token = System.getenv("YT_TOKEN");
        if (token != null) {
            return token;
        }
        try {
            Path tokenPath = Paths.get(System.getProperty("user.home"), ".yt", "token");
            try (BufferedReader reader = Files.newBufferedReader(tokenPath)) {
                token = reader.readLine();
            }
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
        if (token == null) {
            throw new IllegalStateException("Missing user yt token");
        }
        return token;
    }
}
