namespace NJokerLightConfig;

struct TConfig {
    endpoint : string (required, cppname = Endpoint);
    database : string (required, cppname = Database);

    threads : ui16 (default = 5, cppname = Threads);
    port : ui16 (default = 80, cppname = Port);
    source_timeout : duration (default = "5s", cppname = SourceTimeout);
};
