LIBRARY()

OWNER(
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    search__ask__ar.nlg
    search__ask__ru.nlg
    search__common__ar.nlg
    search__common__ru.nlg
    search__factoid_call_ar.nlg
    search__factoid_call_ru.nlg
    search__factoid_src_ar.nlg
    search__factoid_src_ru.nlg
    search__serp_ar.nlg
    search__serp_ru.nlg
    search__show_on_map_ar.nlg
    search__show_on_map_ru.nlg
    search_ar.nlg
    search_ru.nlg
    search_factoid_div_cards_ar.nlg
    search_factoid_div_cards_ru.nlg
    serp_gallery__ar.nlg
    serp_gallery__ru.nlg
    skill_discovery/skill_discovery_ar.nlg
    skill_discovery/skill_discovery_ru.nlg
    wizards/images/images_search_gallery_div1_ar.nlg
    wizards/images/images_search_gallery_div1_ru.nlg
)

END()
