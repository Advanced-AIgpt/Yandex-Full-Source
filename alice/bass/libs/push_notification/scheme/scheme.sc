namespace NBassPushNotification;

struct TCallbackData {
    uuid : string (cppname = UUId);
    did : string (required, != "", cppname = DId);
    uid : string (required, != "", cppname = UId);
    client_id : string (required, != "");
    form_data : any (cppname = FormData);
};

struct TQuasarTextAction {
    text_actions : [string] (required);
};

struct TQuasarMMSemanticFrameAnalytics {
    product_scenario : string (required, != "");
    origin : string (required, != "");
    purpose : string (required, != "");
};

struct TQuasarMMSemanticFrameAction {
    typed_semantic_frame : any (required, cppname = TypedSemanticFrame);
    utterance : string (required);
    analytics : TQuasarMMSemanticFrameAnalytics;
};

struct TQuasarRepeatPhraseAction {
    phrase : string (required, != "");
};

struct TQuasarPlayVideoAction {
    play_uri : string (required, !="");
    provider_name : string (required, !="");
    provider_item_id : string (required, !="");
};

struct TApiRequest {
    event : string (required, != "");
    service : string (required, != "");
    service_data : any (required);
    callback_data : string (required);
};
