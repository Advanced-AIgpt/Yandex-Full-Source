from .datasync import Datasync
from .megamind import Megamind
from .laas import Laas
from .blackbox import Blackbox
from .memento import Memento
from .mds import Mds
from .cachalot import Cachalot
from .quasar_iot import QuasarIot


# here are default mock functions for the given backends
STANDARD_MOCKS = {
    # HTTP
    "VOICE__DATASYNC": Datasync.auto,
    "CUTTLEFISH_MEGAMIND_URL": Megamind.auto,
    "LAAS__VOICE": Laas.auto,
    "BLACKBOX__VOICE": Blackbox.auto,
    "MEMENTO_PROXY": Memento.auto,
    "VOICE__MDS_STORE_HTTP": Mds.auto,
    # gRPC
    "IOT__USER_INFO": QuasarIot.process,
    "VOICE__CACHALOT": Cachalot.process,
}
