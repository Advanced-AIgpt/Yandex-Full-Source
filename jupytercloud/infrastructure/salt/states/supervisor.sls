supervisor-package:
  pkg.installed:
  - name: supervisor

supervisor:
  service.running: 
  - enable: True
  - reload: True
  - require:
    - pkg: supervisor-package
