{% if 'settings' in pillar %}
  {% set settings = (pillar['settings'] or '') | load_json %}
{% else %}
  {% set settings = {} %}
{% endif %}

{% if (
  'taxi_dmp_kernel' in settings and settings['taxi_dmp_kernel'] or
  'yandex_taxi_atlas_dashboards' in settings and settings['yandex_taxi_atlas_dashboards']
) %}

{% for repo in pillar['taxi_repos'] %}
taxi-repo-{{ repo | replace(' ', '-') }}:
  pkgrepo.managed:
  - name: "deb http://yandex-taxi-bionic.dist.yandex.ru/yandex-taxi-{{repo}}/all/"
  - file: /etc/apt/sources.list.d/taxi.list
{% endfor %}

{% else %}

{% for repo in pillar['taxi_repos'] %}
taxi-repo-{{ repo | replace(' ', '-') }}:
  test.nop: []
{% endfor %}

{% endif %}

taxi-repo-bionic-unstable:
  pkgrepo.managed:
  - name: "deb http://yandex-taxi-bionic.dist.yandex.ru/yandex-taxi-bionic unstable/all/"
  - file: /etc/apt/sources.list.d/taxi.list
  - disabled: True
