# NB: this file is not included at init.sls;
# to avoid restarting jupyterhub process without users special notification
# 'jupyterhub-service' state does not have 'restart: True' flag
include:
- .service

jupyterhub-service-restart:
  module.run:
  - name: service.restart
  - m_name: jupyterhub-user
