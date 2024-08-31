include:
- .taxi_repos

{% if 'settings' in pillar %}
  {% set settings = (pillar['settings'] or '') | load_json %}
{% else %}
  {% set settings = {} %}
{% endif %}

{% if 'taxi_dmp_kernel' in settings and settings['taxi_dmp_kernel'] %}

yandex-environment-development:
  pkg.latest: []

taxi_dmp_kernel:
  pkg.latest:
  - pkgs:
    - yandex-taxi-dmp-jupyter
    - yandex-taxi-dmp-deps-py3
  - require:
    {% for repo in pillar['taxi_repos'] %}
    - pkgrepo: taxi-repo-{{ repo | replace(' ', '-') }}
    {% endfor %}
    - pkgrepo: taxi-repo-bionic-unstable
    - pkg: yandex-environment-development
  
{% else %}
taxi_dmp_kernel:
  test.nop: []
{% endif %}
