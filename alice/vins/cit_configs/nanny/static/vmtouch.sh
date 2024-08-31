set -e

./vmtouch -l -v -f $(cut -f 2 -d ':' < links)
