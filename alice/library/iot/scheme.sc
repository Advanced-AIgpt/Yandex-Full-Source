namespace NIOTScheme;


struct TParsingHypothesis {
    struct TAction {
        struct TContext {
            device_type : string (cppname = DeviceType);
        };

        struct TArgInfo {
            type : string (required);
            unit : string;
            allowed_values : [any] (cppname = AllowedValues);
        };

        type                     : string (required);
        instance                 : string;
        value                    : any;
        default_value            : any (cppname = DefaultValue);
        allowed_values           : [any] (cppname = AllowedValues);
        relative                 : string (cppname = RelativeChange);
        default_context          : TContext (cppname = DefaultContext);
        allowed_arg              : TArgInfo (cppname = AllowedArgument);
        allowed_arg_variants     : [TArgInfo] (cppname = AllowedArgumentVariants);
        required_arg             : TArgInfo (cppname = RequiredArgument);
        required_arg_variants    : [TArgInfo] (cppname = RequiredArgumentVariants);
        nlg                      : any (cppname = NLG);
        applicable_types         : [string] (cppname = ApplicableTypes);
        require_device_as_target : bool (cppname = RequireDeviceAsTarget, default = false);
        request_type             : string (cppname = RequestType, default = "action");
        target                   : string (default = "capability");
        require_time             : bool (cppname = RequireTime, default = false);
        flags                    : [string];

        // Extra-value, added to the hypothesis-priority, to make some actions more important than the others
        bonus                    : i64 (cppname = Bonus);

        require_smart_speakers  : bool (cppname = RequireSmartSpeakersAmongTargetDevices, default = false);
        replace_value_with      : string (cppname = ReplaceValueWith);
        bonus_if_matched_room   : i64 (cppname = BonusIfMatchedRoom);
        require_value_with_relative : bool (cppname = RequireValueWithRelative, default = false);
    };

    struct TArgs {
        num             : [i64] (cppname = Numbers);
        color           : [string] (cppname = Colors);
        thermostat_mode : [string] (cppname = ThermostatModes);
    };

    struct TFsts {
        TIME     : [any] (cppname = Times);
        DATETIME : [any] (cppname = Datetimes);
        DATETIME_RANGE : [any] (cppname = DatetimeRanges);
    };

    struct TRawEntity {
        struct TExtra {
            is_synonym         : bool (default = false, cppname = IsSynonym);
            is_close_variation : bool (default = false, cppname = IsCloseVariation);
            ids                : [string];
            is_exact           : bool (default = false, cppname = IsExact);
        };

        value : any (required);
        type  : string (required);
        text  : string (required);
        key   : string (required);
        start : i64 (required);
        end   : i64 (required);
        extra : TExtra (required);
    };

    group              : [string] (cppname = Groups);
    room               : [string] (cppname = Rooms);
    household          : [string] (cppname = HouseholdIds);
    device             : [string] (cppname = Devices);
    device_type        : [string] (cppname = DeviceTypes);
    action             : [TAction] (cppname = Actions);
    instance           : [TAction] (cppname = Instances);
    mode               : [TAction] (cppname = Modes);
    arg                : TArgs (cppname = Arguments);
    fst                : TFsts (cppname = Fsts);
    unit               : [string] (cppname = Units);
    scenario           : [string] (cppname = Scenarios);
    triggered_scenario : [string] (cppname = TriggeredScenarios);
    common             : [any] (cppname = Common);
    room_type          : [any] (cppname = RoomTypes);
    raw_entities       : [TRawEntity] (cppname = RawEntities);
    special_mark       : [string] (cppname = SpecialMarks);
};
