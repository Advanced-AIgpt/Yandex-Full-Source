#!/bin/bash

set -xeuo pipefail

# --------------------------------------------------------------------------------------------------
key=$1
repo=$2
branch=$3
output_json=$4

# --------------------------------------------------------------------------------------------------
echo -e '#!/bin/bash\nssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $*' > good_ssh
chmod +x good_ssh
cmd=$(readlink -f good_ssh)

ssh-add "$key"

export GIT_SSH="$cmd"
export GIT_SSH_COMMAND="$cmd"

COMMIT=$(git ls-remote -h $repo $branch | cut -f 1)

echo "{\"repo\":\"$repo\",\"commit\":\"$COMMIT\"}" > $output_json
