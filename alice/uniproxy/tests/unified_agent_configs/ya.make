OWNER(g:voicetech-infra)

EXECTEST()

RUN(
    NAME
    check_unified_agent_config_beta
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/beta/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_prod
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/prod/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_split_beta_apphost
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/split/beta/apphost/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_split_beta_cuttlefish
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/split/beta/cuttlefish/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_split_beta_wsproxy
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/split/beta/wsproxy/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_split_main_apphost
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/split/main/apphost/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_split_main_cuttlefish
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/split/main/cuttlefish/configs/unified_agent.yaml
    check-config
)

RUN(
    NAME
    check_unified_agent_config_split_main_wsproxy
    unified_agent
    --config
    ${ARCADIA_ROOT}/alice/uniproxy/configs/split/main/wsproxy/configs/unified_agent.yaml
    check-config
)

DEPENDS(
    logbroker/unified_agent/bin
)

DATA(
    arcadia/alice/uniproxy/configs/beta/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/prod/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/beta/apphost/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/beta/cuttlefish/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/beta/wsproxy/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/events/apphost/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/events/cuttlefish/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/events/wsproxy/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/main/apphost/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/main/cuttlefish/configs/unified_agent.yaml
    arcadia/alice/uniproxy/configs/split/main/wsproxy/configs/unified_agent.yaml
)

END()
