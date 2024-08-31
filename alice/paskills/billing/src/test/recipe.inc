BUILD_ONLY_IF(OS_LINUX OS_DARWIN ARCH_X86_64)
IF(NOT YA_IDE_IDEA)
    IF(OS_LINUX)
        DATA(
            # PostgreSQL bin 13.5.0 for linux
            sbr://2693545078
        )
        PEERDIR(
            contrib/libs/libpq
        )
    ELSEIF(OS_DARWIN)
        DATA(
            # PostgreSQL bin 13.5.0 for Mac OS
            sbr://2693199232
        )
    ENDIF()
    #INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)
    USE_RECIPE(antiadblock/postgres_local/recipe/recipe --port 0 --user bob --db_name billing)
    DEPENDS(
        antiadblock/postgres_local/recipe
    )
ENDIF()
