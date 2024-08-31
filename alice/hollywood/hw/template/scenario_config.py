filename = "{{ScenarioName}}.pb.txt"


template = """
{%- set host = "scenarios.hamster.alice.yandex.net" -%}
{%- set host = "vins.alice.yandex.net" if config_type == "production" else host -%}
Name: "{{ScenarioName}}"
Description: "Сценарий {{ScenarioName}}. Примеры запросов: 'Включи {{ScenarioName}}', 'Вруби {{ScenarioName}}'"
Languages: [
    L_RUS
]
DataSources: [
    {
        Type: USER_LOCATION
    }
]
AcceptedFrames: [
    "{{frame_name}}"
]
Handlers: {
    BaseUrl: "http://{{host}}/{{scenario_name}}/"
    OverrideHttpAdapterReqId: true
    RequestType: AppHostPure
    GraphsPrefix: "{{scenario_name}}"
}

Enabled: False

Responsibles {
    Logins: "{{username}}"
    {%- for name in abc_services %}
    AbcServices {
        Name: "{{name}}"
    }
    {%- endfor %}
}

"""
