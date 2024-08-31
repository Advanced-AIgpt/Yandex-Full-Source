misc-devops-packages:
  pkg.latest:
  - pkgs:
    - apt-file
    - binutils
    - curl
    - dnsutils
    - file
    - iputils-ping
    - iputils-tracepath
    - lsb-release
    - lsof
    - ltrace
    - ncdu
    - net-tools
    - netcat-openbsd
    - openssh-client
    - psmisc
    - rsync
    - strace
    - telnet
    - traceroute
    - wget
    - dirmngr

shell-packages:
  pkg.latest:
  - pkgs:
    - csh
    - fish
    - zsh

top-packages:
  pkg.latest:
  - pkgs:
    # iotop is broken on our python configuration
    # - iotop
    # - atop
    - htop

man-packages:
  pkg.latest:
  - pkgs:
    - man-db
    - manpages
    - manpages-dev

archive-packages:
  pkg.latest:
  - pkgs:
    - bzip2
    - zip
    - xzdec
    - xz-utils
    - unzip

editor-packages:
  pkg.latest:
  - pkgs:
    - vim
    - nano
    - less

terminal-packages:
  pkg.latest:
  - pkgs:
    - mc
    - tmux
    - screen

vcs-packages:
  pkg.latest:
  - pkgs:
    - subversion
    - git
    - git-svn

dev-packages:
  pkg.latest:
  - pkgs:
    - autoconf
    - automake
    - autotools-dev
    - build-essential
    - cmake
    - gdb
    - geoip-database
    - geoip-bin
    - glibc-doc
    - pkg-config
