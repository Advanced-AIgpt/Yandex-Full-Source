$ydbEndpoint = Common::GetEnv("YDB_ENDPOINT");
$ydbDb = Common::GetEnv("YDB_DB");
$ydbToken = Common::GetEnv("YDB_TOKEN");
$eventsParser = RTLogScenarioLogUdf::ParseScenarioLog($ydbEndpoint, $ydbDb, $ydbToken, "");
SELECT * FROM (
    SELECT * FROM (
        SELECT $eventsParser(TableRow()) AS `Records` FROM Input
    ) FLATTEN BY `Records`
) FLATTEN COLUMNS;
