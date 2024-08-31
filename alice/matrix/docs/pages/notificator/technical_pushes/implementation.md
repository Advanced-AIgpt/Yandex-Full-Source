# Как работает отправка технических пушей

{% note info %}

Документация будет сделана в рамках [этого тикета](https://st.yandex-team.ru/ZION-153).

{% endnote %}

## Как работает /delivery/push {#delivery_push}

@startuml

database "YDB" {
    [Connections]
    [Directives]
}
cloud "Webwocket connections (aka session)" {
    [Uniproxy]
    [Balancer]
    [User device]
}

[Client] -r-> [Notificator] : "HTTP request\nAdd and send technical push"
[Notificator] -r-> [Directives] : "YDB request\nSave technical push to database"
[Notificator] -r-> [Connections] : "YDB request\nGet ip of uniproxy with active session"
[Notificator] -d-> [Uniproxy] : "HTTP request\nTry to send technical push asap if session exists"
[Uniproxy] -d-> [Balancer] : "Push to websocket\nSend technical push"
[Balancer] -l-> [User device] : "Push to websocket\nProxy technical push"

@enduml
