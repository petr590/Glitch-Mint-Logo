#!/bin/bash
# Запускает фоновый процесс debug/glitch-mint-logo и останавливает его через указанное время
# Если время не указано, то через 10 секунд

[ -n "$1" ] && time="$1" || time=10
[ -n "$2" ] && dir="$2" || dir=debug

sudo "$dir/glitch-mint-logo" --config "$dir/config" > /tmp/stdout 2> /tmp/stderr &&\
sleep "$time" &&\
sudo "$dir/glitch-mint-logo" --config "$dir/config" --stop >> /tmp/stdout 2>> /tmp/stderr &&\
sleep 0.5 # Ждём завершения процесса после отправки SIGTERM

echo -e "\nstdout:"
cat /tmp/stdout
echo -e "\nstderr:"
cat /tmp/stderr
