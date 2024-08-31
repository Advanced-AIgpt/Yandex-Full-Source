#pragma once

#include <util/generic/string.h>
#include <util/generic/deque.h>
#include <util/stream/output.h>

namespace NAlice {

class TDialogHistory {
public:
    static constexpr size_t MAX_TURNS_COUNT = 4;

    struct TDialogTurn {
        TString Request;
        TString RewrittenRequest;
        TString Response;
        TString ScenarioName;
        ui64 ServerTimeMs;
        ui64 ClientTimeMs;

        inline auto TieMembers() const {
            return std::tie(Request, RewrittenRequest, Response, ScenarioName, ServerTimeMs, ClientTimeMs);
        }
    };
    TDialogHistory() = default;
    TDialogHistory(const TDeque<TDialogTurn>& dialogTurns);

    void PushDialogTurn(TDialogTurn&& dialogTurn);

    const TDeque<TDialogTurn>& GetDialogTurns() const {
        return DialogTurns;
    }

private:
    TDeque<TDialogTurn> DialogTurns;
};

inline bool operator==(const TDialogHistory::TDialogTurn& l, const TDialogHistory::TDialogTurn& r) {
    return l.TieMembers() == r.TieMembers();
}

inline IOutputStream& operator<<(IOutputStream& out, const TDialogHistory::TDialogTurn& dialogTurn) {
    return out << "{" << dialogTurn.Request << "\", \"" << dialogTurn.RewrittenRequest << "\", \"" <<
        dialogTurn.Response << "\", \"" << dialogTurn.ScenarioName << "\", " << dialogTurn.ServerTimeMs <<
        dialogTurn.ClientTimeMs << "}";
}

} // namespace NAlice
