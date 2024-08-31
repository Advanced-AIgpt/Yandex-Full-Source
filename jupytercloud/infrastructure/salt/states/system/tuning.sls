{% set reserved_space = '5' -%}
reserve-fs-space:
  cmd.run:
  - name: 'tune2fs -m {{ reserved_space }} /dev/vda1'
  - onlyif: test "$(tune2fs -l /dev/vda1 | awk '/^Reserved block count:/ { print $4 }')" = "0"


{% set file_descriptors = '16384' -%}
global-file-descriptors:
  file.managed:
  - name: '/etc/security/limits.d/nofile.conf'
  - contents: '*       soft    nofile  {{ file_descriptors }}'
  - makedirs: True

systemd-file-descriptors:
  file.managed:
  - name: '/etc/systemd/system.conf.d/nofile.conf'
  - contents: 'DefaultLimitNOFILE={{ file_descriptors }}'
  - makedirs: True

# i put it in here because it file called at spawn
new-certs:
  pkg.latest:
  - pkgs:
    - ca-certificates
    - libgnutls30
  - order: 1
  - refresh: False

new-certs2:
  pkg.latest:
  - pkgs:
    - ca-certificates
    - libgnutls30
  - order: 2
  - require:
    - pkg: new-certs
