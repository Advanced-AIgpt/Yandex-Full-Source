GO_PROGRAM(db_filler)

OWNER(g:alice_iot)

SRCS(
    fanout.go
    generator.go
    main.go
    rand.go
)

END()
