#include <string.h>
#include <sys/system_properties.h>

const char *get_hardware_vendor() {
	static char hardware[64];
	static int inited = 0;
	if (inited)
		return hardware;

	__system_property_get("ro.hardware", hardware);
	inited = 1;

	return hardware;
}

int is_mtk_device() {
	const char *device = get_hardware_vendor();

	if(strncmp(device, "mt", 2) == 0)
		return 1;

	return 0;
}

int is_gt9308_cmcc() {
	char hardware[128];

	__system_property_get("ro.build.fingerprint", hardware);

	if(strncmp(hardware, "samsung/m0zm/m0cmcc:4.3/JSS15J/I9308ZMUBNI1", sizeof("samsung/m0zm/m0cmcc:4.3/JSS15J/I9308ZMUBNI1") - 1) ==0)
		return 1;

	return 0;
}


