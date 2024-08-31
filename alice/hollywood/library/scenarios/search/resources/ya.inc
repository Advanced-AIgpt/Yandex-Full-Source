UNION()

OWNER(
    tolyandex
    g:alice_scenarios
)

FROM_SANDBOX(FILE 557102274 OUT_NOAUTO bno.trie)

FROM_SANDBOX(FILE 628681802 OUT windows_fixlist.tar.gz)

FROM_ARCHIVE(windows_fixlist.tar.gz OUT_NOAUTO windows_fixlist.data)
FROM_ARCHIVE(windows_fixlist.tar.gz OUT_NOAUTO windows_fixlist.offsets)

COPY_FILE(alice/bass/data/navigation_fixlist_general.json navigation_fixlist_general.json)
COPY_FILE(alice/bass/data/navigation_fixlist_yandex.json navigation_fixlist_yandex.json)
COPY_FILE(alice/bass/data/navigation_fixlist_turbo.json navigation_fixlist_turbo.json)
COPY_FILE(alice/bass/data/navigation_nativeapps.json navigation_nativeapps.json)

END()
