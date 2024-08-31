# Как запустить
**1. Сделать ssh-туннель до беты бульбазавра, где есть доступ до api.social.yandex.ru**

Про туннели: https://wiki.yandex-team.ru/users/cramen/moi-zametki/ssh-tunnels/

Адрес беты бульбазавра: `fzd5rbvwncto5xmq.sas.yp-c.yandex.net`

Итоговая команда: `ssh -L 9999:api.social.yandex.ru:443 fzd5rbvwncto5xmq.sas.yp-c.yandex.net
`

**2. Добавить `api.social.yandex.ru` в `/etc/hosts`**

Строчка в /etc/hosts: `127.0.0.1 api.social.yandex.ru `

**3. Задать SOCIALISM_URL=`https://api.social.yandex.ru:9999`**

**4. Запустить бинарник, указав ему адрес базы в YDB.**

Как результат создастся файлик output.txt, в котором будут нужные логи.
Фильтровать нужно по `msg: UserDeviceTypes`
