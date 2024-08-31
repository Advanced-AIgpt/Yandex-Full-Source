
CREATE TABLE analytics.toloka_quality

(
    fielddate Date,
    app Nullable(String),
    action Nullable(String),
    intent Nullable(String),
    probability Nullable(String),
    url Nullable(String),
    uuid String,
    context_0 Nullable(String),
    context_1 Nullable(String),
    context_2 Nullable(String),
    reply Nullable(String)

) ENGINE = MergeTree()
    PARTITION BY (fielddate)
    ORDER BY (fielddate, uuid)
