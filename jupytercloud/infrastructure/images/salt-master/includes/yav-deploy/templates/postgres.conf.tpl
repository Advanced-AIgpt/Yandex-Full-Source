postgres:
  user: 'robot_jupyter_cloud'
  pass: '{{ db_password }}'
  db: '{{ postgres_db }}'
  host: '{{ postgres_url }}'
  port: 6432

postgres_cache.host: '{{ postgres_url }}'
postgres_cache.password: '{{ db_password }}'

returner.pgjsonb.host: '{{ postgres_url }}'
returner.pgjsonb.pass: '{{ db_password }}'
