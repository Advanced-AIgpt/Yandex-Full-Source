LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/common_nlg
)

# THIS MODULE IS DEPRECATED!
# Contains nlg from vins.
# Use it only for bass scenarios.
COMPILE_NLG(
    cards/common.nlg
    cards/gallery/common.nlg
    cards/gallery/extended_gallery.nlg
    cards/gallery/gallery.nlg
    common.nlg
    common_ru.nlg
    market_login.nlg
    market_orders_status_ru.nlg
    suggests/beru_activation.nlg
    suggests/open_blue.nlg
)

END()
