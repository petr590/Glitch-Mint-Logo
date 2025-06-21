#include "module.h"
#include <systemd/sd-bus.h>
#include <assert.h>

#define SD_BUS_SLOTS 1
#define MATCH_STRING_PATTERN "type='signal',"\
		"sender='org.freedesktop.systemd1',"\
		"path='%s',"\
		"interface='org.freedesktop.DBus.Properties',"\
		"member='PropertiesChanged'"

static const char* services[SD_BUS_SLOTS] = {
	"/org/freedesktop/systemd1/unit/network_2etarget",
};

static const char* messages[SD_BUS_SLOTS] = {
	"Network: connected",
};

sd_bus* bus_ptr;
static sd_bus_slot* bus_slots[SD_BUS_SLOTS] = {};

static int on_properties_changed(sd_bus_message* msg, void* userdata, sd_bus_error* ret_error) {
	const char* interface;
	sd_bus_message_enter_container(msg, SD_BUS_TYPE_STRING, &interface);
	sd_bus_message_exit_container(msg);

	/* Читаем параметры сигнала PropertiesChanged:
	   signature: sa{sv}as
	   - interface name (string)
	   - changed properties (dict<string,variant>)
	   - invalidated properties (array of string)
	*/

	const char* iface;
	int ret = sd_bus_message_read(msg, "s", &iface);
	if (ret < 0) {
		fprintf(stderr, "SD-Bus: Failed to read interface: %s\n", strerror(-ret));
		return ret;
	}

	if (strcmp(iface, "org.freedesktop.systemd1.Unit") != 0) { // Не интересующий нас интерфейс
		return 0;
	}

	sd_bus_message_enter_container(msg, SD_BUS_TYPE_ARRAY, "{sv}");
    
	while (sd_bus_message_enter_container(msg, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0) {
		const char* key;
		ret = sd_bus_message_read(msg, "s", &key);
		if (ret < 0) break;

		if (strcmp(key, "ActiveState") == 0) {
			const char* value;
			ret = sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, "s");
			if (ret < 0) {
				fprintf(stderr, "SD-Bus: Failed to enter variant container: %s\n", strerror(-ret));
				return ret;
			}

			ret = sd_bus_message_read(msg, "s", &value);
			if (ret < 0) {
				fprintf(stderr, "SD-Bus: Failed to read string from variant: %s\n", strerror(-ret));
				sd_bus_message_exit_container(msg);
				return ret;
			}

			sd_bus_message_exit_container(msg);

			printf("Service state changed: %s\n", value);

			if (strcmp(value, "active") == 0) {
				running_strings[running_strings_len++].str = (const char*) userdata;
			}

		} else {
			sd_bus_message_skip(msg, "v");
		}

		sd_bus_message_exit_container(msg); // dict entry
	}

	sd_bus_message_exit_container(msg); // array

	return 0;
}


static void init_sd_bus_slot(int index) {
	const size_t buffer_len = sizeof(MATCH_STRING_PATTERN) - sizeof("%s") + strlen(services[index]) + 1; // '\0'
	char buffer[buffer_len];
	snprintf(buffer, buffer_len, MATCH_STRING_PATTERN, services[index]);

	assert(buffer[buffer_len - 1] == '\0');

	int ret = sd_bus_add_match(bus_ptr, &bus_slots[index], buffer, on_properties_changed, &messages[index]);
	if (ret < 0) {
		fprintf(stderr, "SD-Bus: Failed to add match: %s\n", strerror(-ret));
		exit(-ret);
	}
}

static void init_sd_bus(void) {
    sd_bus_open_system(&bus_ptr);

	for (int i = 0; i < SD_BUS_SLOTS; i++) {
		init_sd_bus_slot(i);
	}
}

static void unref_sd_bus(void) {
	for (int i = 0; i < SD_BUS_SLOTS; i++) {
		if (bus_slots[i]) {
			sd_bus_slot_unref(bus_slots[i]);
			bus_slots[i] = NULL;
		}
	}

	if (bus_ptr) { sd_bus_unref(bus_ptr); bus_ptr = NULL; }
}