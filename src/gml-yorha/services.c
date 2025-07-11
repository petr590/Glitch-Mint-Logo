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
	const char* const name;
	const wchar_t* const message;
	const wchar_t* const message_ru;
	int used;
} service_info_t;


// При любых изменениях синхронизировать с resources/services.list
static service_info_t services[] = {
	{
		.name = "systemd-journald.service",
		.message = L"Journal: started",
		.message_ru = L"Журнал: загружен",
	},

	{
		.name = "systemd-udevd.service",
		.message = L"Device manager: initialized",
		.message_ru = L"Диспетчер устройств: инициализирован",
	},

	{
		.name = "systemd-timesyncd.service",
		.message = L"Time synchronization: established",
		.message_ru = L"Синхронизация времени: установлена",
	},

	{
		.name = "systemd-logind.service",
		.message = L"Login manager: active",
		.message_ru = L"Менеджер авторизации: активен",
	},

	{
		.name = "dbus.service",
		.message = L"IPC bus: initialized",
		.message_ru = L"Шина IPC: инициализирована",
	},

	{
		.name = "keyboard-setup.service",
		.message = L"Keyboard: connected",
		.message_ru = L"Клавиатура: подключена",
	},

	{
		.name = "network.target",
		.message = L"Network protocols: activated",
		.message_ru = L"Сетевые протоколы: активированы",
	},

	{
		.name = "cron.service",
		.message = L"Task scheduler: ready",
		.message_ru = L"Планировщик задач: готов",
	},

	{
		.name = "polkit.service",
		.message = L"Permissions: granted",
		.message_ru = L"Разрешения: предоставлены",
	},

	{
		.name = "remote-fs.target",
		.message = L"Remote filesystems: mounted",
		.message_ru = L"Удалённые ФС: подключены",
	},
};



extern const char* socket_path; // Инициализируется в другом месте
static int socket_fd = -1;
static int is_ru = 0;

void init_socket(void) {
	is_ru = strcmp(getenv("LANG"), "ru_RU.UTF-8") == 0;

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
	fflush(stdout); // printf буферизует текст, и до systemd он доходит намного позже
	
	int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags >= 0) {
    	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
	}
}

static void add_running_str(service_info_t* service) {
	if (!service->used && running_strings_len < MAX_RUNNING_STRINGS) {
		service->used = 1;
		running_strings[running_strings_len++].str = is_ru ? service->message_ru : service->message;
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