package ru.yandex.alice.memento.ydb;

import java.time.Duration;

import com.yandex.ydb.table.query.Params;
import com.yandex.ydb.table.transaction.TxControl;

import ru.yandex.alice.paskills.common.ydb.YdbClient;

@SuppressWarnings("HideUtilityClassConstructor")
public class YdbTestUtils {
    public static void clearTable(YdbClient client, String tableName) {
        client.execute("clear_table_" + tableName, Duration.ofSeconds(10), session ->
                session.executeDataQuery("delete from " + tableName + ";",
                        TxControl.serializableRw(),
                        Params.empty(),
                        client.keepInQueryCache()));
    }
}
