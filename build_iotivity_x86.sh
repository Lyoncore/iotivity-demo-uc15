if [ -z "$DEMOROOT" ]; then
	echo "Please 'source env-setup' first"
	exit 1
fi

if [ -n "$1" ]; then
	re='^[0-9]+$'
	if [[ $1 =~ $re ]] ; then
		cd ${IOTIVITY_DIR} && scons -j $1
		snappy=$2
	else
		cd ${IOTIVITY_DIR} && scons
		snappy=$1
	fi
else
	cd ${IOTIVITY_DIR} && scons
fi

if [ "$snappy" == "snappy" ]; then
	mkdir -p ${DEMOROOT}/snappy/iotivity-server/lib
	mkdir -p ${DEMOROOT}/snappy/iotivity-server/lib/x86_64-linux-gnu
	mkdir -p ${DEMOROOT}/snappy/iotivity-server/magic-bin/x86_64-linux-gnu
	cp -f ${IOTIVITY_DIR}/out/linux/x86_64/release/*.so ${DEMOROOT}/snappy/iotivity-gateway/lib/x86_64-linux-gnu/
	cp -f ${IOTIVITY_DIR}/out/linux/x86_64/release/resource/examples/democlient ${DEMOROOT}/snappy/iotivity-gateway/magic-bin/x86_64-linux-gnu/
	cd ${DEMOROOT}/snappy/iotivity-gateway && snapcraft clean && rm -f *.snap && snapcraft
fi
