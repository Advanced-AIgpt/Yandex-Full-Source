namespace NBassApi;

struct TPlayerRewindCommand {
    type : string (allowed = ["forward", "backward", "absolute"]);
    amount : double;
};
