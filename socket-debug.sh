#!/bin/bash
# Запускает фоновый процесс ./build/glitch-mint-logo и останавливает его через указанное время
# Если время не указано, то через 10 секунд

[ -n "$1" ] && time="$1" || time=10

readarray -t services < resources/services.list

sudo ./build/glitch-mint-logo --config build/config > /tmp/stdout 2> /tmp/stderr

period="$(bc <<< "scale=3; $time / ${#services[@]}")"

for service in "${services[@]}"; do
	sleep "$period"
	sudo ./build/glitch-mint-logo --config build/config --service-loaded "$service" >> /tmp/stdout 2>> /tmp/stderr
done

sudo ./build/glitch-mint-logo --config build/config --stop >> /tmp/stdout 2>> /tmp/stderr
sleep 0.5 # Ждём завершения процесса после отправки SIGTERM

echo -e "\nstdout:"
cat /tmp/stdout
echo -e "\nstderr:"
cat /tmp/stderr
