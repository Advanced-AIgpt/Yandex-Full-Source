RED='\033[0;31m'
NORMAL='\033[0m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'

if [ $# -eq 0 ] 
  then
    filename=$(whoami)_$(date +"%m-%d-%Y_%H-%M-%S")
  else
    filename=$1 
fi

if  [ $# -gt 1 ]
  then
    printf "${RED}Wrong input argument${NORMAL}\n"
    exit 1
fi

screencap_out=$( adb shell screencap -p /sdcard/$filename.png 2>&1 )
if [[ $? -eq 0 ]];
  then
    printf "\r screencap ok"
  else
    printf "\r${RED}Aborted with: $screencap_out ${NORMAL}\n"
    exit 1
fi

pull_out=$( adb pull /sdcard/$filename.png )
if [[ $? -eq 0 ]];
  then
    printf "\r pull ok"
  else
    printf "\r${RED}Aborted with: $pull_out ${NORMAL}\n"
  exit 1
fi

printf "\r${GREEN}Done! File saved with name $filename ${NORMAL}\n"

rm_out=$( adb shell rm /sdcard/$filename.png )
if [[ $? -ne 0 ]];
  then
    printf "${YELLOW}WARN: sceenshot was not deleted from device: $rm_out ${NORMAL}\n"  
fi
