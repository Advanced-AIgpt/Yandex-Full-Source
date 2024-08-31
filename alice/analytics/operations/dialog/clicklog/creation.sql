
CREATE TABLE analytics.dialogs

(
-- user_info
    uuid String, -- can we fix width?
    user_id String,
    cohort Date,
    this_week UInt8, -- bool, derived from is_new=='1 week',
    app String,
    platform String,
    version String,

--- session_info
    fielddate Date,
    session_start UInt32,  -- timestamp
    session_part UInt8, -- num of session without timeout
    experiments Array(String),
    is_exp_changed UInt8, -- bool

-- request info
    query String,
    reply String,

    req_id String,

    ms_server_time UInt32, -- timestamp of request (server_time)
    ts UInt32, -- timestamp of request (client_time)
    delta UInt16,  -- seconds from previous phrase
    req_ord UInt16,  -- Порядковый номер запроса в пределах сессии, начинается с 1
    req_rev_ord UInt16,  -- То же, что req_ord, но с конца, начинается с 1

    intent String,
    mm_scenario Nullable(String),
    generic_scenario String,
    gc_intent Nullable(String),
    gc_source Nullable(String),
    restored Nullable(String),
    skill_id Nullable(String),
    input_type String,  -- enumerate?

    test_ids Array(UInt32),
    expboxes Nullable(String),

    suggests Array(String),

    cards Nested(
        text String,
        type String,
        card_id Nullable(String),
        intent_name Nullable(String),
        actions Array(String)
    ),

    -- callback properties
    cb_name Nullable(String),
    -- name = form_update
    cb_form_name Nullable(String),
    cb_resubmit Nullable(UInt8),  -- bool
    cb_slots Nullable(String),  -- json object {slot_name: slot_value}
    -- name = on_reset_session
    cb_mode Nullable(String),
    -- on_suggest
    cb_caption Nullable(String),
    cb_suggest_type Nullable(String),
    cb_utterance Nullable(String),
    cb_uri Nullable(String),
    -- name = on_card_action
    cb_action_name Nullable(String),
    cb_req_id Nullable(String),  -- id of request suggested this card
    cb_card_id Nullable(String),
    cb_intent_name Nullable(String),
    -- (cb_uri already set)
    -- name = on_external_button
    cb_btn_data Nullable(String)


) ENGINE = MergeTree()
    PARTITION BY (fielddate)
    ORDER BY (fielddate, uuid, session_start, session_part, req_ord)
    -- SAMPLE BY (fielddate, uuid)
