#pragma once

#include <util/generic/variant.h>

#include <functional>

namespace NAlice::NNlg {

class TValue;

class TCaller {
public:
    // TODO(a-square): using std::any might be worthwhile,
    // but right now it's too complicated and not useful enough
    //
    // NOTE(a-square): 4 parameters limit is completely arbitrary, I've seen 2 max in the wild
    using TCaller0 = std::function<void(IOutputStream&)>;
    using TCaller1 = std::function<void(IOutputStream&, const TValue&)>;
    using TCaller2 = std::function<void(IOutputStream&, const TValue&, const TValue&)>;
    using TCaller3 = std::function<void(IOutputStream&, const TValue&, const TValue&, const TValue&)>;
    using TCaller4 = std::function<void(IOutputStream&, const TValue&, const TValue&, const TValue&, const TValue&)>;

    using TData = std::variant<TCaller0, TCaller1, TCaller2, TCaller3, TCaller4>;

public:
    template <typename TSignature>
    explicit TCaller(std::function<TSignature>&& callback)
        : Data(std::move(callback)) {
    }

    template <size_t N>
    const std::variant_alternative_t<N, TData>& Get() const {
        return std::get<N>(Data);
    }

private:
    std::variant<TCaller0, TCaller1, TCaller2, TCaller3, TCaller4> Data;
};

}  // namespace NAlice::NNlg
