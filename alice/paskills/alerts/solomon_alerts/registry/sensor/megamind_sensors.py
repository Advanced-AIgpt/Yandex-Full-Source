from library.python.monitoring.solo.objects.solomon.sensor import Sensor

dialogovo_scenario_response_type_is_error = Sensor(
    project='megamind',
    cluster='prod',
    service='server',
    scenario_name='DIALOGOVO',
    name='scenario.protocol.responses_per_second',
    request_type='*',
    host='cluster',
    response_type='error*',
)

dialogovo_scenario_response_total = Sensor(
    project='megamind',
    cluster='prod',
    service='server',
    scenario_name='DIALOGOVO',
    name='scenario.protocol.responses_per_second',
    request_type='*',
    host='cluster',
)
