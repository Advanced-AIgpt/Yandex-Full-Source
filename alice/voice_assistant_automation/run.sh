#!/bin/bash
export HOME=$BSCONFIG_IDIR
export ANDROID_SDK_ROOT="${HOME}/Android/Sdk/"
export ANDROID_AVD_HOME="${HOME}/.android/avd"

PORT_NUM=5582

# Kill running emulator if exists
python kill_emulator.py

# Start emulator
echo -e "\n\nStarting emulator...\n"
$ANDROID_SDK_ROOT/platform-tools/adb devices

$ANDROID_SDK_ROOT/emulator/emulator-headless -port ${PORT_NUM} -avd Pixel_2_API_28 -http-proxy zomb-voice-analytics:zEuKZwQ1RBXnjMAOk3lca86PvTbNhe0J@animals.search.yandex.net:4004 -delay-adb -camera-back none -camera-front none -accel on -gpu host -memory 4096 -cores 4 -restart-when-stalled -ranchu -no-sim -no-passive-gps -skip-adb-auth -no-audio -gpu swiftshader_indirect -no-boot-anim -no-cache -no-snapstorage -no-snapshot-save -no-snapshot-load -skin 360x640 -verbose -qemu -lcd-density 160 1>&2 &
sleep 60

echo "Following devices available:"
$ANDROID_SDK_ROOT/platform-tools/adb devices

echo -e "\n\nWaiting for emulator...\n"

$ANDROID_SDK_ROOT/platform-tools/adb -s emulator-${PORT_NUM} wait-for-device shell 'while [[ -z $(getprop sys.boot_completed) ]]; do sleep 1; done;'
sleep 60

echo "Following devices available:"
$ANDROID_SDK_ROOT/platform-tools/adb devices

echo -e "\n\nTuning emulator...\n"

# Turn Skia renderer on for better performance
$ANDROID_SDK_ROOT/platform-tools/adb root shell "su && setprop debug.hwui.renderer skiagl && stop && start"

# Disable animations
$ANDROID_SDK_ROOT/platform-tools/adb shell settings put global window_animation_scale 0
$ANDROID_SDK_ROOT/platform-tools/adb shell settings put global transition_animation_scale 0 
$ANDROID_SDK_ROOT/platform-tools/adb shell settings put global animator_duration_scale 0

#popd # Getting back to $HOME
cd $HOME

# Enable write to disk
echo -e "\n\nSetting app permissions...\n"
$ANDROID_SDK_ROOT/platform-tools/adb shell pm grant com.yandex.assistantautomation android.permission.READ_EXTERNAL_STORAGE
$ANDROID_SDK_ROOT/platform-tools/adb shell pm grant com.yandex.assistantautomation android.permission.WRITE_EXTERNAL_STORAGE

# Download sessions
yt download --proxy hahn //home/voice-speechbase/yashrk/scripts/script.txt > $HOME/script.txt

# Prepare launch
echo -e "\n\nCopying script to emulator...\n" >>/proc/self/fd/2
$ANDROID_SDK_ROOT/platform-tools/adb shell rm /sdcard/*.png >>/proc/self/fd/2
$ANDROID_SDK_ROOT/platform-tools/adb shell 'echo "0" > /sdcard/query-number' >>/proc/self/fd/2
$ANDROID_SDK_ROOT/platform-tools/adb push $HOME/script.txt /sdcard >>/proc/self/fd/2

# Run test
echo -e "\n\nRun assistant...\n"
t_before=$(date +%s)
$ANDROID_SDK_ROOT/platform-tools/adb shell am instrument -w -r   -e class com.yandex.assistantautomation.AssistantInstrumentedTest#useGoogleAssistant com.yandex.assistantautomation.test/android.support.test.runner.AndroidJUnitRunner
t_after=$(date +%s)
spent=$((t_after-t_before))
echo "Time spent: $spent sec."

# Get results
echo -e "\n\nLoading results...\n"
for i in `$ANDROID_SDK_ROOT/platform-tools/adb shell ls /sdcard | grep ".png"`; do \
    $ANDROID_SDK_ROOT/platform-tools/adb pull /sdcard/$i $1; \
done

# Upload results to YT
echo -e "\n\nUploading results to YT...\n"
FILENAME=`date +%F_%R`_google_screenshots.tar.gz
FILENAME=`echo $FILENAME | sed -e "s/:/_/"`
tar -czf $FILENAME script*
cat $FILENAME | yt upload --proxy hahn //home/voice-speechbase/yashrk/misc/$FILENAME

# Save logs
timeout 5 $ANDROID_SDK_ROOT/platform-tools/adb shell logcat > $HOME/emulator.log

# Clear workdir from screenshots
rm $HOME/*.png

# Kill running emulator if exists
python kill_emulator.py

echo -e "\n\nDone!\n"
