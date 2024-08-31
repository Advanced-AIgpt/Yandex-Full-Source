OWNER(g:hollywood)

EXECTEST()

RUN(
    NAME
    check_unified_agent_config_prod_shared_prod
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/hollywood/scripts/nanny_files/unified_agent_config_prod.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_prod_shared_dev
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/hollywood/scripts/nanny_files/unified_agent_config.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_prod_common_prod
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/hollywood/shards/common/prod/nanny_files/unified_agent_config_prod.yaml
    check-config
)

DEPENDS(
    logbroker/unified_agent/bin
)

DATA(
    arcadia/alice/hollywood/scripts/nanny_files/unified_agent_config_prod.yaml
    arcadia/alice/hollywood/scripts/nanny_files/unified_agent_config.yaml
    arcadia/alice/hollywood/shards/common/prod/nanny_files/unified_agent_config_prod.yaml
)

END()