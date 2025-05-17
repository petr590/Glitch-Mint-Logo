#!/bin/sh
if [ -n "$1" ]; then
	time="$1"
else
	time=10
fi

sudo ./build/glitch-mint-logo && sleep "$time" && sudo ./build/glitch-mint-logo --stop
