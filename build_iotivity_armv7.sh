if [ -z "$DEMOROOT" ]; then
	echo "Please 'source env-setup' first"
	exit 1
fi

cd ${IOTIVITY_DIR} && scons TARGET_ARCH=armeabi-v7a-hard TARGET_OS=linux examples

if [ "$1" == "snappy" ]; then
	mkdir -p ${DEMOROOT}/snappy/iotivity-gateway/lib
	mkdir -p ${DEMOROOT}/snappy/iotivity-gateway/lib/arm-linux-gnueabihf
	mkdir -p ${DEMOROOT}/snappy/iotivity-gateway/magic-bin/arm-linux-gnueabihf
	mkdir -p ${DEMOROOT}/snappy/iotivity-server/lib
	cp -f ${IOTIVITY_DIR}/out/linux/armeabi-v7a-hard/release/*.so ${DEMOROOT}/snappy/iotivity-gateway/lib/arm-linux-gnueabihf/
	cp -f ${IOTIVITY_DIR}/out/linux/armeabi-v7a-hard/release/resource/examples/democlient ${DEMOROOT}/snappy/iotivity-gateway/magic-bin/arm-linux-gnueabihf/
	cp -f ${IOTIVITY_DIR}/out/linux/armeabi-v7a-hard/release/*.so ${DEMOROOT}/snappy/iotivity-server/lib/
	cp -f ${IOTIVITY_DIR}/out/linux/armeabi-v7a-hard/release/resource/examples/demoserver ${DEMOROOT}/snappy/iotivity-server/
	cd ${DEMOROOT}/snappy/iotivity-gateway && rm -f *.snap && snapcraft
	cd ${DEMOROOT}/snappy/iotivity-server && rm -f *.snap && snapcraft
	cd ${DEMOROOT}/snappy/grovepi-python-server && rm -f *.snap && snapcraft
fi
