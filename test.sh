#!/usr/bin/env bash

HWMONPATH=/sys/devices/platform/coretemp.0/hwmon/hwmon0

echo -e "\e[1;34mtempbgline \e[1;36m-v\e[0m"
echo -e "\e[1;31m ↳ \n\e[0m$(./tempbgline -v)\e[0m"

echo -e "\e[1;34mtempbgline\e[0m"
echo -e "\e[1;31m ↳ [\e[1;34m$(./tempbgline)\e[1;31m]\e[0m"

echo -e "\e[1;34mtempbgline \e[1;36m-m $HWMONPATH\e[0m"
echo -e "\e[1;31m ↳ [\e[1;34m$(./tempbgline -m $HWMONPATH)\e[1;31m]\e[0m"

echo -e "\e[1;34mtempbgline -m $HWMONPATH \e[1;36m-a\e[0m"
echo -e "\e[1;31m ↳ [\e[1;34m$(./tempbgline -m $HWMONPATH -a)\e[1;31m]\e[0m"


