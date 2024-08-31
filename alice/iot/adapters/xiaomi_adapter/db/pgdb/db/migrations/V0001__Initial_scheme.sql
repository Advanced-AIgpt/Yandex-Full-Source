-- xiaomi part
CREATE SCHEMA xiaomi;

CREATE TABLE xiaomi.external_users
(
    external_user_id text NOT NULL,
    user_ids         text NOT NULL,
    subscription_key text NOT NULL,

    CONSTRAINT pk_external_users PRIMARY KEY (external_user_id)
);

-- broker part
CREATE SCHEMA iot;

CREATE TYPE iot.task_state AS ENUM ('READY', 'RUNNING', 'DONE', 'FAILED');

CREATE TABLE iot.tasks
(
    group_key      text           NOT NULL,
    id             UUID           NOT NULL,
    name           text           NOT NULL,
    state          iot.task_state NOT NULL,
    payload        JSONB          NOT NULL,
    created_time   timestamp      NOT NULL,
    scheduled_time timestamp      NOT NULL,
    updated_time   timestamp      NOT NULL,
    retry_left     int            NOT NULL DEFAULT 0,
    last_error     text,
    merge_key      text,

    CONSTRAINT pk_tasks PRIMARY KEY (group_key, id)
);

CREATE INDEX ix_tasks_name_state_scheduled_time_updated_time ON iot.tasks (name, state, scheduled_time, updated_time);
CREATE UNIQUE INDEX ix_tasks_name_group_key_merge_id ON iot.tasks (name, group_key, merge_key) WHERE state='READY';
