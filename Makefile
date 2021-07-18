SQLpgm:
	gcc -o SQLpgm SQLex.c -L../sqlite-amalgamation -lsqlite3 -L../paho.mqtt.c/build/output -lpaho-mqtt3a
