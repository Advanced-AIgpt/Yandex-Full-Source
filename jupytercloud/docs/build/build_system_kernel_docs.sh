#!/usr/bin/env bash

set -o errexit -o noclobber -o pipefail -o xtrace;

# to prevent echo-command from logging we wrap command with {set +x; ...; set -x} 2>/dev/null
# because bootstrap script itself invoked with `bash -x`
{
	set +x;
	if [ -z "$SSH_KEY" ]
	then
		echo "Empty variable SSH_KEY";
		exit 1;
	fi

	echo "$SSH_KEY" > /srv/ssh.key;
	chmod 0600 /srv/ssh.key;
	set -x;
} 2> /dev/null

export SVN_SSH="ssh -v -i /srv/ssh.key";

WORK_DIR="/srv/build_docs";

mkdir -p $WORK_DIR;

svn co svn+ssh://robot-jupyter-cloud@arcadia.yandex.ru/arc/trunk/arcadia/jupytercloud/docs/build/BuildSystemKernelDocs.ipynb $WORK_DIR;

rm /srv/ssh.key

touch $WORK_DIR/output.ipynb;
touch $WORK_DIR/log;

{
	set +e;

	/usr/local/share/jupyter/kernels/arcadia_default_py3/arcadia_default_py3 \
		run-notebook \
		--notebook-path=$WORK_DIR/BuildSystemKernelDocs.ipynb \
		--output-path=$WORK_DIR/output.ipynb \
		--log-level=DEBUG \
		--log-file=$WORK_DIR/log;

	success=$?;

	set -e;
}

cat $WORK_DIR/log;

exit $success;
