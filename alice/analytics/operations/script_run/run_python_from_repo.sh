#!/bin/bash

set -xeu

ssh_key=$1
repo_url=$2
branch=$3
script_path=$4


# ====== prepare ssh env ======

echo -e '#!/bin/bash\nssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $*' > good_ssh
chmod +x good_ssh
cmd=$(readlink -f good_ssh)

ssh-add "$ssh_key"

export GIT_SSH="$cmd"
export GIT_SSH_COMMAND="$cmd"


# ===== download repo =====

git archive --format=zip --remote=$repo_url $branch > "repo.zip"
unzip "repo.zip" -x "nirvana/*"


# ====== run script ======

export PYTHONPATH=${PYTHONPATH}:$PWD

#TODO: python -> job environment
python "$script_path"
