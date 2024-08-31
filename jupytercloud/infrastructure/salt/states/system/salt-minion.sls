{% if grains.get('jupyter_cloud_environment') == 'testing' %}
{% set masters = [
    "salt-iva.beta.jupyter.yandex-team.ru",
    "salt-myt.beta.jupyter.yandex-team.ru",
    "salt-sas.beta.jupyter.yandex-team.ru"
] %}
{% elif grains.get('jupyter_cloud_environment') == 'production' %}
{% set masters = [
    "salt-iva.jupyter.yandex-team.ru",
    "salt-myt.jupyter.yandex-team.ru",
    "salt-sas.jupyter.yandex-team.ru"
] %}
{% endif %}

manage-salt-minions:
  file.managed:
  - name: /etc/salt/minion
  - source: salt://system/minion.tpl
  - mode: 644
  - template: jinja
  - context:
      masters: {{ masters }}
