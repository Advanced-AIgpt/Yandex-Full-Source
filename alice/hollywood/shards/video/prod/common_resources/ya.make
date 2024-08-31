UNION()

OWNER(g:smarttv)

FROM_SANDBOX(FILE 3380941241 AUTOUPDATED geodata OUT_NOAUTO geodata6.bin)

COPY_FILE(alice/hollywood/shards/all/prod/common_resources/nlg_translations.json nlg_translations.json)

END()
