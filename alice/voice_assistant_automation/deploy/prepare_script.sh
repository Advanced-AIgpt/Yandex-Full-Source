#!/bin/bash
AVD_ARCHIVE=avd-4.tar.gz

export HOME=$BSCONFIG_IDIR

# Setup tunyproxy

tar -xJf tinyproxy-3.tar.xz
sudo cp -R tmp /

# Setup emulator

export ANDROID_SDK_ROOT="${HOME}/Android/Sdk/"
export ANDROID_AVD_HOME="${HOME}/.android/avd"

mkdir $HOME/Android
mkdir $HOME/Android/Sdk
sudo cp -R /opt/android-sdk/* $HOME/Android/Sdk/
sudo cp -R /home/user/Android/Sdk/* $HOME/Android/Sdk/

declare ME=$(whoami)
sudo -E chown $ME -R /dev/kvm

# From salavat's script

sudo -E mkdir "${HOME}/.android"
sudo -E unzip -qq android_28_google_apis.zip -d "${HOME}/.android"
sudo -E sed -i 's@/root@'"$HOME"'@' ${HOME}/.android/avd/android_28.ini
sudo -E sed -i 's@/root@'"$HOME"'@' ${HOME}/.android/avd/android_28.avd/hardware-qemu.ini

declare me=$(whoami)
sudo -E chown $me -R /dev/kvm

# Permissions for ~/.android

sudo chown -R $ME $HOME/.android/
sudo -E chmod -R 777 ${HOME}/.android

# Unpack emulator

echo -e "\n\nUnpacking emulator...\n"

#mkdir $HOME/.android/
# For created in porto layer
#sudo cp /home/user/.android/adbkey $HOME/.android/adbkey
#sudo cp /home/user/.android/adbkey.pub $HOME/.android/adbkey.pub
# For created in porto layer; for Salavat's layer
sudo cp adbkey $HOME/.android/adbkey
sudo cp adbkey.pub $HOME/.android/adbkey.pub
# For copied from yashrk's laptop
# sudo cp adbkey $HOME/.android/adbkey
# sudo cp adbkey.pub $HOME/.android/adbkey.pub
mv $AVD_ARCHIVE $HOME/.android/
pushd $HOME/.android/
tar -xzf $AVD_ARCHIVE

sed -i 's@/home/user@'"$HOME"'@' $HOME/.android/avd/Pixel_2_API_28.ini
sed -i 's@/home/user@'"$HOME"'@' $HOME/.android/avd/Pixel_2_API_28.avd/hardware-qemu.ini

