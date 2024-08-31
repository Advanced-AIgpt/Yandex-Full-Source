{% for unwanted_motd in [
    "10-help-text", "50-motd-news", "92-unattended-upgrades"
] %}

disable_motd_{{unwanted_motd}}:
  file.managed:
  - name: /etc/update-motd.d/{{unwanted_motd}}
  - mode: 644

{% endfor %}

enable_jupytercloud_motd:
  file.managed:
  - name: /etc/update-motd.d/90-jupytercloud
  - source: salt://system/jupyter_motd.sh
  - mode: 755

