apt-get update

apt-get install -y memcached

apt-get autoremove -y --force-yes
apt-get clean

rm -rf /var/lib/apt
rm -rf /var/cache

rm -rf /tmp/*
