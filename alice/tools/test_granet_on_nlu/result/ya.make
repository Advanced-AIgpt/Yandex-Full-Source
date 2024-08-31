OWNER(g:karalink)
  
PY2_PROGRAM(output_results)

PY_SRCS(
    MAIN output_results.py
)

PEERDIR(
    contrib/python/pandas
    contrib/python/click
)

END()
