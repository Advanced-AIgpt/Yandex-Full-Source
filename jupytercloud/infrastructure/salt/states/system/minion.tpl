---
master:
{%- for item in masters %}
  - {{ item }}
{%- endfor %}
ipv6: True
log_level: DEBUG
rejected_retry: True
master_alive_interval: 60
random_master: True
