include:
- salt-minion

{% set kernels = ['arcadia_default_py2', 'arcadia_default_py3'] %}

arcadia_kernels:
  sandbox.arcadia_kernels:
  - name: /usr/local/share/jupyter/kernels
  - kernels: {{kernels}}
  - require:
    - pip: sandbox-client

{% if grains.get('jupyter_cloud_user') %}

{% for kernel in kernels %}

no_user_kernel_{{kernel}}:
  file.absent:
  - name: /home/{{grains['jupyter_cloud_user']}}/.local/share/jupyter/kernels/{{kernel}}

{% endfor %}

{% endif %}
