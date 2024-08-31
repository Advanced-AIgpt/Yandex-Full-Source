include:
- salt-minion

{% set pythons3 = ['python3.6', 'python3.7'] %}
# NOTE: python2.7 is last in the list
# to have higher priority as alternative of python
# because some system utilities uses /usr/bin/python shebang
{% set pythons = pythons3 + ['python2.7'] %}
{% set python3_alternatives = [] %}
{% set python_alternatives = [] %}

{% for python in pythons %}

{{ python }}:
  pkg.latest:
  - require:
    - test: apt-daily-killed

{{ python }}-alternative:
  alternatives.install:
  - name: python
  - link: /usr/bin/python
  - path: /usr/bin/{{ python }}
  - priority: {{ 10 * loop.index }}
  - require:
    - pkg: {{ python }}

{% do python_alternatives.append({'alternatives': '{{ python }}-alternative'}) %}

{% endfor %}

{% for python in pythons3 %}

{{ python }}-3-alternative:
  alternatives.install:
  - name: python3
  - link: /usr/bin/python3
  - path: /usr/bin/{{ python }}
  - priority: {{ 10 * loop.index }}
  - require:
    - pkg: {{ python }}

{% do python3_alternatives.append({'alternatives': '{{ python }}-3-alternative'}) %}

{% endfor %}

python-auto-alternatives:
  alternatives.auto: 
  - name: python

python3-auto-alternatives:
  alternatives.auto: 
  - name: python3
