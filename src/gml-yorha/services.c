#include "module.h"
#include "../util.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SERVICES (sizeof(services) / sizeof(services[0]))


running_str_t running_strings[MAX_RUNNING_STRINGS];
size_t running_strings_len;


typedef struct {
	const char* name;
	const char* message;
	const char* message_ru;
	int used;
} service_info_t;


// При любых изменениях синхронизировать с resources/services.list
static service_info_t services[] = {
	{
		.name = "systemd-journald.service",
		.message = "Journal: started",
		.message_ru = "Журнал: загружен",
	},

	{
		.name = "systemd-udevd.service",
		.message = "Device manager: initialized",
		.message_ru = "Диспетчер устройств: инициализирован",
	},

	{
		.name = "systemd-timesyncd.service",
		.message = "Time synchronization: established",
		.message_ru = "Синхронизация времени: установлена",
	},

	{
		.name = "systemd-logind.service",
		.message = "Login manager: active",
		.message_ru = "Менеджер авторизации: активен",
	},

	{
		.name = "dbus.service",
		.message = "IPC bus: initialized",
		.message_ru = "Шина IPC: инициализирована",
	},

	{
		.name = "keyboard-setup.service",
		.message = "Keyboard: connected",
		.message_ru = "Клавиатура: подключена",
	},

	{
		.name = "network.target",
		.message = "Network protocols: activated",
		.message_ru = "Сетевые протоколы: активированы",
	},

	{
		.name = "cron.service",
		.message = "Task scheduler: ready",
		.message_ru = "Планировщик задач: готов",
	},

	{
		.name = "polkit.service",
		.message = "Permissions: granted",
		.message_ru = "Разрешения: предоставлены",
	},

	{
		.name = "remote-fs.target",
		.message = "Remote filesystems: mounted",
		.message_ru = "Удалённые ФС: подключены",
	},
};



extern const char* socket_path; // Инициализируется в другом месте
static int socket_fd = -1;

void init_socket(void) {
	printf("init_socket()\n");
	unlink(socket_path);

	create_dirs(socket_path);

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_path);
	socklen_t len = offsetof(struct sockaddr_un, sun_path) + strlen(socket_path) + 1;

	int ret = bind(socket_fd, (struct sockaddr*) &addr, len);
	if (ret < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	ret = listen(socket_fd, SERVICES);
	if (ret) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	
	
	printf("Listen: %s\n", socket_path);
	printf("'%s' %s\n", socket_path, access(socket_path, F_OK) == 0 ? "exists" : "not exists");
	fflush(stdout);
	
	int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags >= 0) {
    	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
	}
}

static void add_running_str(service_info_t* service) {
	if (!service->used && running_strings_len < MAX_RUNNING_STRINGS) {
		service->used = 1;
		running_strings[running_strings_len++].str = service->message;
	}
}

void read_from_socket(void) {
	char buffer[256];

	for (;;) {
		int client_fd = accept(socket_fd, NULL, NULL);
		if (client_fd < 0) break;

		int n = read(client_fd, buffer, sizeof(buffer) - 1);
		if (n < 0) break;

		buffer[n] = '\0';
		printf("Read from socket: '%s'\n", buffer);

		for (int i = 0; i < SERVICES; i++) {
			if (strcmp(services[i].name, buffer) == 0) {
				add_running_str(&services[i]);
				break;
			}
		}

		close(client_fd);
	}
}

void cleanup_socket(void) {
	if (socket_fd >= 0) {
		close(socket_fd);
		socket_fd = -1;
	}

	unlink(socket_path);
}