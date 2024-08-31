salt-minion-service-restart:
  module.run:
  - name: service.restart
  - m_name: salt-minion
