IMPORT YsonUtils SYMBOLS $string_field, $any_field;

$isUniproxySessionLogEvent = ($row) -> {
    $event = Yson::ParseJson($row.EventItem.Event);
    $fields = $any_field($event, "Fields");
    return $row.EventItem.ServiceName == "uniproxy" AND
           $row.EventItem.EventType == "LogEvent" AND
           $string_field($fields, "funcName") == "SESSION" AND
           $string_field($fields, "name") == "uniproxy.session";
};

SELECT * FROM (
  SELECT EventItem FROM Input
  WHERE EventItem IS NOT NULL AND NOT $isUniproxySessionLogEvent(TableRow())
) FLATTEN COLUMNS;
