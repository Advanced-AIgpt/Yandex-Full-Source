namespace NShooterConfig;

struct TConfig {
    // Mocked YDb is mainly for Kinopoisk (and smth else)
    struct TYdb {
        endpoint : string (required, cppname = Endpoint);
        database : string (required, cppname = Database);
    };

    struct TNannyJokerServer {
        cluster_name   : string (required, cppname = ClusterName);
        endpoint_set_id : string (required, cppname = EndpointSetId);
    };

    struct TPlainJokerServer {
        host : string (required, cppname = Host);
        port : ui16 (required, cppname = Port);
    };

    struct TJokerConfig {
        server_type   : string (required, cppname = ServerType); // 'nanny' or 'plain'
        server : any (required, cppname = Server); // a TNannyJokerServer or a TPlainJokerServer
        session_settings : string (default = "", cppname = SessionSettings); // CGI-formatted string of session settings
        session_id : string (required, cppname = SessionId);
    };

    struct TServersSettings {
        bass_threads : ui16 (default = 20, cppname = BassThreads);
        vins_workers : ui16 (default = 10, cppname = VinsWorkers);
        vins_current_timestamp : ui64 (default = 1582146000, cppname = VinsCurrentTimestamp); // 2020-02-20
    };

    struct TRunSettings {
        package_path : string (required, cppname = PackagePath);
        logs_path : string (default = "", cppname = LogsPath);
        responses_path : string (default = "", cppname = ResponsesPath);
        config : string (default = "rc", cppname = Config);

        enabled_experiments : [string] (cppname = EnabledExperiments);
        disabled_experiments : [string] (cppname = DisabledExperiments);

        increase_timeouts : bool (default = true, cppname = IncreaseTimeouts);

        enable_idle_mode : bool (default = false, cppname = EnableIdleMode); // do not stop after shooting
        enable_perf_mode : bool (default = false, cppname = EnablePerfMode); // don't save responses, only times
        dont_close : bool (default = false, cppname = DontClose); // don't close apps after running

        enable_hollywood_mode : bool (default = false, cppname = EnableHollywoodMode); // shoot hollywood
        enable_hollywood_bass_mode : bool (default = false, cppname = EnableHollywoodBassMode); // shoot hollywood bass
    };

    yav_secret_id     : string (cppname = YavSecretId);
    ydb               : TYdb (cppname = Ydb);
    joker_config      : TJokerConfig (cppname = JokerConfig);
    servers_settings  : TServersSettings (cppname = ServersSettings);

    runs_settings : [TRunSettings] (required, cppname = RunsSettings);
    requests_path : string (default = "", cppname = RequestsPath);

    requests_limit : ui32 (default = 1000000, cppname = RequestsLimit);
    threads : ui16 (default = 5, cppname = Threads); // requests per second
    vmtouch : bool (default = false, cppname = Vmtouch);
};
