{% for ver in ('2.7', '3.7') %}
{% set pip_bin = "/usr/local/bin/pip" + ver %}

python{{ver}}-jsonschema-removed:
  pip.removed:
  - name: jsonschema
  - bin_env: {{pip_bin}}

{% endfor %}
