# disable tensorflow garbage output

import logging
logging.getLogger('tensorflow').disabled = True
