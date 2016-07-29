if [ -z "$DEMOROOT" ]; then
	echo "Please 'source env-setup' first"
	exit 1
fi

if [ -z "$1" ]; then
	echo "Need to specify TARGET_ARCH in the first parameter, supported architecture:"
	echo "armeabi"
	echo "armeabi-v7a"
	echo "x86"
	echo "x86_64"
	exit 1
else
	cd ${IOTIVITY_DIR} && scons TARGET_OS=android TARGET_ARCH=$1 ANDROID_HOME=/home/gerald/Android/Sdk ANDROID_NDK=/home/gerald/Android/android-ndk-r10e ANDROID_GRADLE=/home/gerald/android-studio/gradle/gradle-2.8/bin/gradle
fi
