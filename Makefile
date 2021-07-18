SQLpgm:
	gcc -o SQLpgm SQLex.c -L./vish1/sqlite-amalgamation -lsqlite3 -L./vish1/paho.mqtt.c/build/output -lpaho-mqtt3a
