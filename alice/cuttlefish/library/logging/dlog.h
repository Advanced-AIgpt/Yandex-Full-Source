#pragma once

// Following code was written to compile with flags
// DISABLE_DLOG or DLOG_TO_FILE defined in ya.conf or ya make command line.
// So we can disable DLOG or redirect it to file.
// But it seems that flags from ya.conf or ya make command line
// do not trigger any changes in compilation.
// Thus we still have to change this file manually if necessary.

// By default DLOG writes to the Cerr.
// And I (paxakor) recommend to keep it this way in order to have
// the chance to debug flapping tests by logs from CI.

// Uncomment following lines if necessary but do not commit this changes.
// #define DISABLE_DLOG
// #define DLOG_TO_FILE "/tmp/dlog.txt"


#ifndef NDEBUG
    #ifndef DISABLE_DLOG
        #include <util/stream/output.h>
        #include <util/datetime/base.h>
        #include <util/system/src_location.h>

        #ifdef DLOG_TO_FILE
            #include <util/generic/singleton.h>
            #include <util/stream/buffered.h>
            #include <util/stream/file.h>
            #include <util/stream/fwd.h>

            class TFileCerr : public TFileOutput {
            public:
                TFileCerr()
                    : TFileOutput(DLOG_TO_FILE)
                {
                }

                static TFileCerr& Instance() {
                    return *Singleton<TFileCerr>();
                }
            };

            #define DLOG_STREAM TFileCerr::Instance()

        #else

            #define DLOG_STREAM Cerr

        #endif

        #define DLOG_IMPL(MSG, PREFIX) \
            DLOG_STREAM << '[' << PREFIX << TInstant::Now().ToStringLocal() << ", " << __LOCATION__ << "] " \
                        << MSG << Endl;

        #define DLOG(MSG) DLOG_IMPL(MSG, "DLOG ")

    #endif
#endif

#ifndef DLOG
    #define DLOG_IMPL(MSG, PREFIX)
    #define DLOG(MSG)
#endif
