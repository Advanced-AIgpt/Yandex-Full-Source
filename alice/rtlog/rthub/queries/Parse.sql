$eventsParser = RTLogUdf::ParseEvents();
SELECT * FROM (
    SELECT * FROM (
        SELECT $eventsParser(TableRow()) AS `Records` FROM Input
    ) FLATTEN BY `Records`
) FLATTEN COLUMNS;
