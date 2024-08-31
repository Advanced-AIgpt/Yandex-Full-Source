namespace NJokerConfig;

struct TConfig {
    struct TServer {
        HttpPort (required) : ui16;
        HttpThreads : ui16 (required, default = 10);
        SourceTimeout : duration (default = "1s");
    };

    struct TIdent {
        Project : string (required);
        Session : string;
    };

    struct TBackendConfig {
        Type : string (required, != "");
        Params : any;
    };

    struct TS3BackendConfig {
        struct TCredentials {
            KeyId     : string (required, != "");
            KeySecret : string (required, != "");
        };
        Bucket      : string (required, != "");
        Credentials : TCredentials;
        Host        : string (required, != "");
        Timeout     : duration (required, default = "20ms");
    };

    struct TFSBackendConfig {
        Dir : string (required, != "");
    };

    Server  : TServer (required);
    Ident   : TIdent;
    Backend : TBackendConfig (required);
    WorkDir : string (required, != "");
    RequestsHistorySize : ui16 (default = 500);
    StubIdGenerator : struct {
        SkipHeader     : [ string ];
        SkipCGI        : [ string ];

        SkipAllHeaders : bool (default = false);
        SkipAllCGIs    : bool (default = false);
        SkipBody       : bool (default = false);
    };
};
