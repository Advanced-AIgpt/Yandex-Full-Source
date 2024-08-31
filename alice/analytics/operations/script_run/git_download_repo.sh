#!/bin/bash

set -xeuo pipefail

# --------------------------------------------------------------------------------------------------
key=$1
url=$2
commit=$3
remove_git_files=$4
output=$5


# --------------------------------------------------------------------------------------------------
echo -e '#!/bin/bash\nssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $*' > good_ssh
chmod +x good_ssh
cmd=$(readlink -f good_ssh)

ssh-add "$key"

export GIT_SSH="$cmd"
export GIT_SSH_COMMAND="$cmd"


# --------------------------------------------------------------------------------------------------
git clone "$url" repo
cd repo

# --------------------------------------------------------------------------------------------------
if [[ ! -z "$commit" ]]; then
  git checkout "$commit"
fi

# --------------------------------------------------------------------------------------------------
if [[ "$remove_git_files" == "true" ]]; then
  rm -rf '.git'
fi

# --------------------------------------------------------------------------------------------------

tar -czvf "$output" .
