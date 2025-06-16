#!/bin/sh
if [ -n "$1" ]; then
	time="$1"
else
	time=10
fi

sudo ./build/glitch-mint-logo --config build/config && sleep "$time" && sudo ./build/glitch-mint-logo --config build/config --stop
