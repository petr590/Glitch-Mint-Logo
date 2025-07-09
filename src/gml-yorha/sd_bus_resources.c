#include "module.h"
#include "util.c"
#include <systemd/sd-bus.h>
#include <assert.h>

#define MATCH_PATTERN "type='signal',"\
		"sender='org.freedesktop.systemd1',"\
		"path='%s',"\
		"interface='org.freedesktop.DBus.Properties',"\
		"member='PropertiesChanged'"

#define SYSTEMD_OBJECTS (sizeof(systemd_objects) / sizeof(systemd_objects[0]))

typedef struct {
	const char* name;
	const char* path;
	const char* message;
	sd_bus_slot* bus_slot;
	int used;
} systemd_object_t;


sd_bus* bus_ptr;

static systemd_object_t systemd_objects[] = {
	{
		.name = "systemd-journald.service",
		.path = "/org/freedesktop/systemd1/unit/systemd_2djournald_2eservice",
		.message = "Journal: started",
	},

	{
		.name = "systemd-udevd.service",
		.path = "/org/freedesktop/systemd1/unit/systemd_2dudevd_2eservice",
		.message = "Device manager: initialized",
	},

	{
		.name = "network.target",
		.path = "/org/freedesktop/systemd1/unit/network_2etarget",
		.message = "Network protocols: activated",
	},

	{
		.name = "systemd-timesyncd.service",
		.path = "/org/freedesktop/systemd1/unit/systemd_2dtimesyncd_2eservice",
		.message = "Time synchronization: established",
	},

	{
		.name = "dbus.service",
		.path = "/org/freedesktop/systemd1/unit/dbus_2eservice",
		.message = "IPC bus: initialized",
	},

	{
		.name = "systemd-logind.service",
		.path = "/org/freedesktop/systemd1/unit/systemd_2dlogind_2eservice",
		.message = "Login manager: active",
	},

	{
		.name = "cron.service",
		.path = "/org/freedesktop/systemd1/unit/cron_2eservice",
		.message = "Scheduled tasks: ready",
	},

	{
		.name = "cron.service",
		.path = "/org/freedesktop/systemd1/unit/cron_2eservice",
		.message = "Scheduled tasks: ready",
	},

	{
		.name = "polkit.service",
		.path = "/org/freedesktop/systemd1/unit/polkit_2eservice",
		.message = "Permissions: granted",
	},

	{
		.name = "remote-fs.target",
		.path = "/org/freedesktop/systemd1/unit/remote_2dfs_2etarget",
		.message = "Remote filesystems: mounted",
	},

	{
		.name = "graphical.target",
		.path = "/org/freedesktop/systemd1/unit/graphical_2etarget",
		.message = "Graphical interface: loaded",
	},
};


static void add_running_str(systemd_object_t* object) {
	if (!object->used && running_strings_len < RUNNING_STRINGS) {
		object->used = 1;
		running_strings[running_strings_len++].str = object->message;
	}
}


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
	sd_bus_message_read(msg, "s", &iface);

	if (strcmp(iface, "org.freedesktop.systemd1.Unit") != 0) { // Не интересующий нас интерфейс
		return 0;
	}

	sd_bus_message_enter_container(msg, SD_BUS_TYPE_ARRAY, "{sv}");
    
	while (sd_bus_message_enter_container(msg, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0) {
		const char* key;
		sd_bus_message_read(msg, "s", &key);

		if (strcmp(key, "ActiveState") == 0) {
			const char* value;
			sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, "s");
			sd_bus_message_read(msg, "s", &value);
			sd_bus_message_exit_container(msg);

			if (strcmp(value, "active") == 0) {
				systemd_object_t* object = (systemd_object_t*) userdata;

				printf("Service %s activated\n", object->name);
				add_running_str(object);
			}

		} else {
			sd_bus_message_skip(msg, "v");
		}

		sd_bus_message_exit_container(msg); // dict entry
	}

	sd_bus_message_exit_container(msg); // array

	return 0;
}

static int is_object_active(systemd_object_t* object) {
	sd_bus_message* msg;

	SDBUS_EXIT_IF_ERROR(sd_bus_call_method(bus_ptr,
		"org.freedesktop.systemd1",        // сервис
		object->path,                      // объект
		"org.freedesktop.DBus.Properties", // интерфейс
		"Get",                             // метод
		NULL,
		&msg,
		"ss",                              // сигнатура аргументов
		"org.freedesktop.systemd1.Unit",   // интерфейс, где находится свойство
		"ActiveState"                      // имя свойства
	));
	
	const char* value;
	sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, "s");
	sd_bus_message_read(msg, "s", &value);
	sd_bus_message_exit_container(msg);
	
	int res = (strcmp(value, "active") == 0);
	sd_bus_message_unref(msg);
	return res;
}


static void add_sd_bus_match(systemd_object_t* object) {
	const char* path = object->path;

	const size_t buffer_len = sizeof(MATCH_PATTERN) - sizeof("%s") + strlen(path) + 1; // '\0'

	char buffer[buffer_len];
	snprintf(buffer, buffer_len, MATCH_PATTERN, path);

	assert(buffer[buffer_len - 1] == '\0');
	
	SDBUS_EXIT_IF_ERROR(sd_bus_add_match(
		bus_ptr, &object->bus_slot,
		buffer, on_properties_changed, object
	));
}


static void init_systemd_object(systemd_object_t* object) {
	if (is_object_active(object)) {
		printf("Service %s already active\n", object->name);
		add_running_str(object);

	} else {
		add_sd_bus_match(object);
	}
}

void init_sd_bus(void) {
	SDBUS_EXIT_IF_ERROR(sd_bus_open_system(&bus_ptr));

	for (int i = 0; i < SYSTEMD_OBJECTS; i++) {
		init_systemd_object(&systemd_objects[i]);
	}
}

void cleanup_sd_bus(void) {
	for (int i = 0; i < SYSTEMD_OBJECTS; i++) {
		sd_bus_slot** slot_ptr = &systemd_objects[i].bus_slot;

		if (*slot_ptr) {
			sd_bus_slot_unref(*slot_ptr);
			*slot_ptr = NULL;
		}
	}

	if (bus_ptr) {
		sd_bus_unref(bus_ptr);
		bus_ptr = NULL;
	}
}