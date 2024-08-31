#!/usr/bin/env bash
set -o errexit -o nounset -o xtrace -o pipefail

dir="${BASH_SOURCE[0]%/*}/"
ya="$dir/../../ya"
echo "$dir" "$ya"

base_images="images/build-image/pkg.json images/deploy-app/pkg.json"
docker_conf="--docker --docker-repository jupyter-cloud --docker-network=host --docker-push"

"$ya" package ${docker_conf} --custom-version latest "$@" ${base_images}
"$ya" package ${docker_conf} "$@" ${base_images}
