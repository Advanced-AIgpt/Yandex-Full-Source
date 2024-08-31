**NB:** чтобы поменять версию `traefik-proxy`, надо поправить `traefik-proxy/Dockerfile
` и `jupyterhub/Dockerfile`!

Сейчас:
`./build-ya-packages.sh`

Возможно, в будущем:
1) `pip install -i https://pypi.yandex-team.ru/simple/ releaser-cli[all]`
2) `./build-all.sh`
3) `docker push registry.yandex.net/jupyter-cloud/salt-master`
4) `releaser deploy -i registry.yandex.net/jupyter-cloud/salt-master -v latest -q int -p jupyter -a salt -e development -c master`
