namespace NBASSRequest;

struct TForm {
    struct TSlot {
        name        : string (required, != "");
        optional    : bool (required);
        source_text : any;
        type        : string (required, != "");
        value       : any (required);
    };

    name  : string (required, != "");
    slots : [TSlot];
};

struct TMeta {
    struct TDeviceState {
        struct TRemindersState {
            struct TItem {
                id       : string (required, cppname = Id);
                text     : string (required, cppname = Text);
                epoch    : string (required, cppname = Epoch);
                timezone : string (required, cppname = TimeZone);
                origin   : string (cppname = Origin);
            };
            list : [ TItem ] (cppname = List);
        };
        struct TTimer {
            struct TDirective {
                name : string(required);
            };

            timer_id          : string (required, cppname = Id);
            currently_playing : bool (required, cppname = IsPlaying);
            paused            : bool (required, cppname = IsPaused);
            start_timestamp   : ui64 (required, cppname = StartAt);
            remaining         : ui64 (required, cppname = Remaining);
            duration          : ui64 (cppname = Duration);
            timestamp         : ui64 (cppname = Timestamp);
            directives        : [ TDirective ] (cppname = Directives);
        };
        struct TMusicCurrentlyPlayingInfo {
            track_info : any;
            track_id : string;
        };
        struct TPlayer {
            pause : bool (default = false);
            timestamp : double;
        };
        struct TBluetoothConnection {
            name : string;
        };
        struct TScreen {
            supported_screen_resolutions (cppname = SupportedScreenResolutions) : [ string ]; // (allowed = [ "video_format_SD", "video_format_HD", "video_format_UHD" ])
            hdcp_level (cppname = HdcpLevel): string (allowed = [ "current_HDCP_level_none", "current_HDCP_level_1X", "current_HDCP_level_2X" ]);
            dynamic_ranges (cppname = DynamicRanges): [ string ]; //(allowed = [ "dynamic_range_SDR", "dynamic_range_HDR10", "dynamic_range_HDR10Plus", "dynamic_range_DV", "dynamic_range_HLG" ])
        };
        sound_level : i64 (cppname = SoundLevel, default = -1);
        sound_muted : bool (cppname = SoundMuted, default = false);
        bluetooth : struct {
            currently_playing : TMusicCurrentlyPlayingInfo;
            player : TPlayer;
            current_connections : [ TBluetoothConnection ];
            last_play_timestamp : double;
        };
        struct TAudioPlayerCurrentlyPlayingInfo {
            stream_id : string;
            title : string;
            subtitle : string;
        };
        struct TAudioPlayer {
            player_state : string; // For EPlayerState see https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/common/device_state.proto?rev=r8073122#L130
            offset_ms : i32;
            current_stream : TAudioPlayerCurrentlyPlayingInfo;
            scenario_meta : { string -> string };
            last_play_timestamp : double;
            duration_ms : i32;
            last_stop_timestamp : double;
            played_ms : i32;
        };
        struct TMusic {
            currently_playing : TMusicCurrentlyPlayingInfo;
            session_id : string;
            playlist_owner : string;
            player : TPlayer;
            last_play_timestamp : double;
        };
        audio_player : TAudioPlayer;
        music : TMusic;
        video : struct {
            current_screen : string;
            screen_state : any;
            tv_interface_state : any;
            currently_playing : any;
            view_state: any; // webview scenario state (json)
            page_state: any; // temporary fix for ALICE-7246
        };
        radio : struct {
            currently_playing : struct {
                radio_id : string;
                radio_title : string;
            };
            playlist_owner: string;
            player : TPlayer;
            last_play_timestamp : double;
        };
        last_watched : any; // NBassApi::TLastWatchedState
        is_tv_plugged_in : bool (default = false);

        // TODO (@vi002): this is for backward-compatibility and must be removed.
        alarms_state : string (cppname = AlarmsStateObsolete);
        alarm_state (cppname = AlarmsState) : struct {
            icalendar : string (cppname = ICalendar);
            currently_playing : bool;
            sound_alarm_setting (cppname = SoundAlarmSetting) : struct {
                type : string;
                info : any;
            };
            max_sound_level : i64 (cppname = MaxSoundLevel, default = 7);
        };

        device_reminders : TRemindersState (cppname = DeviceReminders);

        timers : struct {
            active_timers : [ TTimer ];
        };
        device_id : string;
        device_model : struct {
            model : string;
            manufacturer : string;
        };
        device_config : struct {
            content_settings : string;
            child_content_settings : string;
        };

        struct TNavigatorState {
            struct TGeoPoint {
                lat : double (required);
                lon : double (required);
            };

            struct TUserAddress {
                lat : double (required);
                lon : double (required);
                name : string;
                arrival_points : [ TGeoPoint ];
            };

            struct TUserSettings {
                avoid_tolls : bool;
            };

            struct TSearchOptions {
                span: struct {
                    south_west: TGeoPoint (required);
                    north_east: TGeoPoint (required);
                };
            };

            search_options: TSearchOptions;

            user_favorites : [ TUserAddress ] (cppname = UserAddresses);
            home : TUserAddress;
            work : TUserAddress;

            user_settings : TUserSettings (cppname = UserSettings);

            current_route : struct {
                points : [ TGeoPoint ];
                distance_to_destination : double;
                arrival_timestamp : i64;
                time_to_destination : i64;
                time_in_traffic_jam : i64;
                distance_in_traffic_jam: double;
            };

            available_voice_ids : [ string ] (cppname = AvailibleVoices);

            // Ad-hoc states for navigator (i.e. "waiting_for_route_confimation")
            states : [ string ] (cppname = States);

        };

        automotive: struct {
            screen_state : string;

            audio: struct {
                current_source: string;
            };
        };

        struct TMediaSource {
            name : string;
        };

        uma: struct {
            active_media_source: struct {
                name : string;
            };

            available_media_sources: [ TMediaSource ];
        };

        fm_radio : struct {
            region_id : i32;
        };

        car_options: struct {
            type  : string;
            model : string;
            vendor: string;
        };

        installed_apps : any (cppname = InstalledApps);
        is_default_assistant : bool (default = false);

        navigator: TNavigatorState (cppname = NavigatorState);

        tanker : struct {
            xPayment: string;
        };

        battery : struct {
            percent: i32;
        };

        struct TTvSetState {
            struct TTvInput {
                name: string;
                custom_name: string;
                id: i32;
            };
            inputs : [ TTvInput ] (cppname = Inputs);
        };

        tv_set: TTvSetState (cppname = TvSetState);

        multiroom : struct {
            mode : string;
            multiroom_session_id : string;
            music : TMusic;
        };


        screen : TScreen (cppname = Screen);

        struct TSubscriptionState {
            subscription : string (allowed = ["unknown", "none", "yandex_subscription"]);
        };

        subscription_state : TSubscriptionState (cppname = SubscriptionState);

        struct TTandemState {
            connected : bool;
        };

        tandem_state : TTandemState (cppname = TandemState);
    };

    struct TBiometricsScore {
        score : double (required);
        user_id  : string (required, != "");
    };

    struct TBiometricsScoresWithMode {
       mode: string (required, != "");
       scores: [TBiometricsScore] (required);
    };

    struct TBiometricsScores {
        status : string;
        request_id : string;
        scores_with_mode : [TBiometricsScoresWithMode];
        // TODO (@thefacetak): this is for backward-compatibility and must be removed.
        scores : [TBiometricsScore];
    };

    struct TBiometrySimpleClassification {
        classname : string;
        tag : string;
    };

    struct TBiometryClassification {
        status : string;
        simple : [TBiometrySimpleClassification];
    };

    struct TLocation {
        lon : double (required);
        lat : double (required);
        accuracy: double (default = 0);
        accurancy: double (default = 0);
        recency: i64 (default = 0);
    };

    struct TPermission {
        status: bool;
        name: string;
    };

    struct TClientInfo {
        app_id : string;
        app_version : string;
        device_id : string;
        device_manufacturer : string;
        device_model : string;
        platform : string;
        os_version : string;
    };

    struct TClientFeatures {
        supported : [ string ];
        unsupported : [ string ];
    };

    struct TEnvironmentState {
        struct TEndpoint {
            capabilities : [ any ] (cppname = Capabilities);
        };
        endpoints : [ TEndpoint ];
    };

    struct TTandemEnvironmentState {
        struct TTandemEnvironmentDeviceInfo {
            application : TClientInfo;
            supported_features : [ string ] (cppname = SupportedFeatures);
            device_state : TDeviceState (cppname = DeviceState);
        };

        struct TEnvironmentGroupInfo {
            struct TEnvironmentGroupDeviceInfo {
                id : string;
                platform : string;
                role : string (allowed = ["unknown_role", "leader", "follower"]);
            };

            type : string (allowed = ["unknown_group", "tandem", "stereopair"]);
            devices : [ TEnvironmentGroupDeviceInfo ];
        };

        devices : [ TTandemEnvironmentDeviceInfo ];
        groups : [ TEnvironmentGroupInfo ];
    };

    biometrics_scores : TBiometricsScores;
    biometry_classification: TBiometryClassification;
    client_id : string (default = "telegram/1.0 (none none; none none)");
    client_ip : string (cppname = ClientIP, default = "127.0.0.1");
    client_info : TClientInfo;
    client_features : TClientFeatures;
    cookies : [ string ];
    megamind_cookies : string;
    device_state : TDeviceState (cppname = DeviceState);
    // it is a separate tab if contains anything (size > 0)
    dialog_id : string;
    environment_state : TEnvironmentState (cppname = EnvironmentState);
    tandem_environment_state : TTandemEnvironmentState (cppname = TandemEnvironmentState);
    epoch : ui64 (required);
    server_time_ms : ui64;
    event : any; // NAlice::TEvent
    experiments : any;
    memento : any;
    personal_data : any (cppname = PersonalData);
    filtration_level : ui8 (cppname = FiltrationLevel);
    is_banned: bool (cppname = IsBanned);
    has_image_search_granet: bool (default = false);
    lang : string (default = "ru-RU");
    user_lang : string (default = "ru");
    location : TLocation;
    megamind_cgi_string : string;
    process_id : string (cppname = ProcessId);
    region_id : ui32 (cppname = RegionId);
    request_start_time : ui64; // Can be used in tests to set a deterministic request start time.
    screen_scale_factor : double (cppname = ScreenScaleFactor, default = 2);
    tld : string (allowed = [ "ru", "ua", "by", "kz", "com.tr", "com" ]);
    tz : string (required, cppname = TimeZone);
    uid : ui64 (cppname = UID);
    user_agent : string (cppname = RawUserAgent, default = "");
    utterance : string (cppname = Utterance); // FIXME do it mandatory
    utterance_data : any;
    asr_utterance : string;
    uuid : string (cppname = UUID, != "");
    yandex_uid : string (cppname = YandexUID);
    device_id : string;
    request_id : string;
    rng_seed : string;
    end_of_utterance: bool;
    permissions: [TPermission];
    pure_gc: bool(cppname = PureGC, default = false);
    // video_gallery_limit works only for yabro
    video_gallery_limit : ui8 (cppname = VideoGalleryLimit, default = 5);
    voice_session : bool (cppname = VoiceSession);
    suppress_form_changes : bool (cppname = SuppressFormChanges, default = false);
    forbid_web_search: bool (cppname = ForbidWebSearch, default = false);
    is_weekly_promo_available: bool (default = false);
    music_from: string;
    is_porn_query: bool (cppname = IsPornQuery, default = false); // currenly available only in video protocol scenario
};

struct TSetupRequest {
    forms : [ TForm ];
    meta  : TMeta (required);
};

struct TRequest {
    struct TAction {
        name : string (required, != "");
        data : any;
    };

    struct TBlock {
        type             : string (required, != "");
        suggest_type     : string;
        attention_type   : string;
        button_type      : string;
        command_type     : string;
        command_sub_type : string;
        card_template    : string;
        form_update      : TForm;
        data             : any;
        web_responses    : any;
    };

    struct TSessionState {
        last_user_info_timestamp : ui64 (cppname = LastUserInfoTimestamp);
        user_info_music_pronounce_timestamps : [ui64] (cppname = UserInfoMusicPronounceTimestamps);
        user_info_no_pronounce_count: i64 (cppname = UserInfoNoPronounceCount);
    };

    form    : TForm;
    action  : TAction;
    meta    : TMeta (required);
    blocks  : [TBlock];
    session_state : TSessionState (cppname = Session);
    setup_responses : any;
    data_sources : { i32 -> any };
};
