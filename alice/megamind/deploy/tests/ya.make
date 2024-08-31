OWNER(g:megamind)

EXECTEST()

RUN(
    NAME
    check_unified_agent_config_prod
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/megamind/deploy/nanny/unified_agent_config_prod.yaml
    check-config
)

RUN(
    NAME
    check_dev_unified_agent_config
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/megamind/deploy/nanny/dev_unified_agent_config.yaml
    check-config
)

DEPENDS(
    logbroker/unified_agent/bin
)

DATA(
    arcadia/alice/megamind/deploy/nanny/unified_agent_config_prod.yaml
    arcadia/alice/megamind/deploy/nanny/dev_unified_agent_config.yaml
)

END()