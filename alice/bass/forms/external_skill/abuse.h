#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NBASS {
namespace NExternalSkill {

/** It collects json string nodes and then send them to the abuse api
 * to find if there are offencive text. Then change phrases with special markers.
 */
class TAbuse {
public:
    /** Adds a json node to check for offencive phrases.
     * It uses the internal ability of NSc::TValue that copying nodes (without Clone())
     * has the same value.
     * @param[in|out] node is used to get string from and change it if it has offencive text.
     * @param[in] isForTTS is a flag which states that the given node will be used for TTS.
     */
    void AddString(NSc::TValue node, bool isForTTS);

    /** Do the real job. Request abuse API and then parse the answer and replace json nodes
     * which have been collected by the AddString() if they have offencive texts.
     */
    bool Substitute(TContext& ctx);

private:
    struct TDescr {
        TDescr(bool isForTts, NSc::TValue node)
            : IsForTTS(isForTts)
            , Node(node)
        {
        }

        bool IsForTTS;
        // Actually this is a ref to string in the skill's answer,
        // changing this value affects the value in skill's json.
        NSc::TValue Node;
    };
    THashMap<TString, TVector<TDescr>> Strings;
};

} // namespace NExternalSkill
} // namespace NBASS
