#!/bin/bash
if [ -n "$1" ]; then
	time="$1"
else
	time=10
fi

sudo ./build/glitch-mint-logo --config build/config > /tmp/stdout 2> /tmp/stderr &&\
sleep "$time" &&\
sudo ./build/glitch-mint-logo --config build/config --stop >> /tmp/stdout 2>> /tmp/stderr &&\
sleep 0.5 # Ждём завершения процесса после отправки SIGTERM

echo -e "\nstdout:"
cat /tmp/stdout
echo -e "\nstderr:"
cat /tmp/stderr