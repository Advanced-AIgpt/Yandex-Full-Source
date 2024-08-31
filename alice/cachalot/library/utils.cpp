#include <alice/cachalot/library/utils.h>


namespace NCachalot {


double MillisecondsSince(const TInstant startTime) {
    return (TInstant::Now() - startTime).MillisecondsFloat();

}


} //  namespace NCachalot
