remove_static_grains:
  local.file.remove:
    - tgt: {{ data['id'] }}
    - args:
      - path: /etc/salt/grains

sync_grains:
  local.saltutil.sync_grains:
    - tgt: {{ data['id'] }}
