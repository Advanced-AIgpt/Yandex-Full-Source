ABOUT:

https://clubs.at.yandex-team.ru/assistant/3930

BUILD:

`docker build -t registry.yandex.net/alice/iot/alice_speaker_bot:latest app && docker push registry.yandex.net/alice/iot/alice_speaker_bot`<br>

INFRASTRUCTURE:
1. hosted at https://deploy.yandex-team.ru/projects/alice-speaker-bot<br>
2. balanced via https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/asb.iot.yandex.net/show/<br>
