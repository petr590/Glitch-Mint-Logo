#!/bin/bash
# Запускает фоновый процесс ./build/glitch-mint-logo и останавливает его через указанное время
# Если время не указано, то через 10 секунд

[ -n "$1" ] && { time="$1"; shift; } || time=10
[ -n "$1" ] && { dir="$1"; shift; } || dir=debug

readarray -t services < resources/services.list

sudo "$dir/glitch-mint-logo" --config "$dir/config" "$@" > /tmp/stdout 2> /tmp/stderr

period="$(bc <<< "scale=3; 0.5 * $time / ${#services[@]}")"

for service in "${services[@]}"; do
	sleep "$period"
	sudo "$dir/glitch-mint-logo" --config "$dir/config" --service-loaded "$service" >> /tmp/stdout 2>> /tmp/stderr
done

sleep "$(bc <<< "scale=3; 0.5 * $time")"

sudo "$dir/glitch-mint-logo" --config "$dir/config" --stop >> /tmp/stdout 2>> /tmp/stderr
sleep 0.5 # Ждём завершения процесса после отправки SIGTERM

echo -e "\nstdout:"
cat /tmp/stdout
echo -e "\nstderr:"
cat /tmp/stderr
