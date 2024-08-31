#include <alice/bitbucket/pynorm/normalize/normalize.h>
#include <alice/bitbucket/pynorm/normalize/reverse-normalize.h>
#include <library/python/ctypes/syms.h>

BEGIN_SYMS()

SYM(norm_data_read)
SYM(norm_data_free)
SYM(norm_model_version)
SYM(norm_data_get_normalizer_names)
SYM(norm_run)
SYM(norm_run_with_whitelist)
SYM(norm_run_with_blacklist)
SYM(free)

SYM(reverse_normalize)

END_SYMS()
