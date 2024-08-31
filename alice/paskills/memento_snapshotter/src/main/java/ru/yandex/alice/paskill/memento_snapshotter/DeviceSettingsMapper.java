package ru.yandex.alice.paskill.memento_snapshotter;

import java.io.IOException;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectReader;
import com.fasterxml.jackson.databind.json.JsonMapper;
import com.google.protobuf.Any;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.util.JsonFormat;

import ru.yandex.alice.memento.proto.MementoApiProto;
import ru.yandex.alice.memento.scanner.KeyMappingScanner;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTree;
import ru.yandex.inside.yt.kosher.impl.ytree.builder.YTreeBuilder;
import ru.yandex.inside.yt.kosher.operations.OperationContext;
import ru.yandex.inside.yt.kosher.operations.Statistics;
import ru.yandex.inside.yt.kosher.operations.Yield;
import ru.yandex.inside.yt.kosher.operations.map.Mapper;
import ru.yandex.inside.yt.kosher.ytree.YTreeMapNode;
import ru.yandex.inside.yt.kosher.ytree.YTreeNode;
import ru.yandex.ysonjsonconverter.YsonJsonConverter;

public class DeviceSettingsMapper implements Mapper<YTreeMapNode, YTreeMapNode> {
    private final KeyMappingScanner keyMappingScanner;
    private final JsonFormat.Printer printer;
    private final ObjectReader reader;

    public DeviceSettingsMapper() throws IOException {
        keyMappingScanner = new KeyMappingScanner();
        printer = JsonFormat.printer();
        reader = new JsonMapper().reader();
    }

    @Override
    public void map(YTreeMapNode entries, Yield<YTreeMapNode> y, Statistics statistics, OperationContext context) {
        YTreeBuilder builder = YTree.mapBuilder()
                .key("userId").value(entries.getString("user_id"))
                .key("deviceId").value(entries.getString("device_id"))
                .key("key").value(entries.getString("key"))
                .key("changedAt").unsignedValue(entries.getLong("changed_at"));
        YTreeNode deviceSettingData = convertDeviceSettingData(
                entries.getBytes("data"),
                entries.getString("key"));
        if (deviceSettingData != null) {
            builder.key("data").value(deviceSettingData);
        }
        y.yield(builder.buildMap());
    }

    @SuppressWarnings("DuplicatedCode")
    private YTreeNode convertDeviceSettingData(byte[] data, String key) {
        MementoApiProto.EDeviceConfigKey deviceConfigKey = keyMappingScanner.deviceConfigEnumByKey(key);
        try {
            Any any = Any.parseFrom(data);
            GeneratedMessageV3 message = any.unpack(keyMappingScanner.getClassForKey(deviceConfigKey));
            JsonNode jsonNode = reader.readTree(printer.print(message));
            return YsonJsonConverter.json2yson(new YTreeBuilder(), jsonNode).build();
        } catch (Throwable e) {
            return null;
        }
    }
}
