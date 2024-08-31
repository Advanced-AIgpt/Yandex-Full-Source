include:
- salt-minion

r-base:
  pkg.latest:
  - pkgs:
    - r-base
    - r-base-dev
  - require:
    - pkgrepo: r-repo

{% for package in ('IRkernel', 'formatR') %}
r-{{package}}-package:
  cmd.run:
  - name: echo "install.packages('{{package}}', repos='https://cloud.r-project.org')" | R --vanilla
  - hide_output: True
  - unless:
    - echo 'library({{package}})' | R --vanilla
  - require:
    - pkg: r-base
{% endfor %}

r-kernel:
  cmd.run:
  - name: echo "IRkernel::installspec(prefix='/usr/local')" | R --vanilla
  - hide_output: True
  - unless:
    - ls /usr/local/share/jupyter/kernels/ir/kernel.js
  - require:
    - cmd: r-IRkernel-package
