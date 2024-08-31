from alice.boltalka.telegram_bot.lib.module import get_module_dict


from alice.boltalka.telegram_bot.lib.nlgsearch_http_source import NlgsearchHttpSource, NlgsearchHttpSourceWithEntity, NlgsearchRanker
from alice.boltalka.telegram_bot.lib.interests_ranker import InterestsRanker
from alice.boltalka.telegram_bot.lib.lstm_memory import LSTMMemoryReranker
from alice.boltalka.telegram_bot.lib.rpc_source import RpcSource
from alice.boltalka.telegram_bot.lib.replier import Replier

MODULES = get_module_dict([
    NlgsearchHttpSource,
    NlgsearchHttpSourceWithEntity,
    RpcSource,
    NlgsearchRanker,
    InterestsRanker,
    LSTMMemoryReranker,
    Replier
])
