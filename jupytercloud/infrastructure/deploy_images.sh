#!/usr/bin/env bash
set -o errexit -o nounset -o xtrace -o pipefail

dir="${BASH_SOURCE[0]%/*}/"
ya="$dir/../../ya"
echo "$dir" "$ya"

"$ya" package --docker --docker-repository jupyter-cloud --docker-push \
    "images/salt-master/pkg.json" \
    "images/jupyter-cloud-idm/pkg.json" \
    "images/traefik-proxy/pkg.json" \
    "images/jupyterhub/pkg.json"
