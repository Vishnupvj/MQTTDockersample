FROM ubuntu:latest
WORKDIR /vish1
WORKDIR /vish1
RUN apt update &&\
apt install -y mosquitto &&\
apt install -y mosquitto-clients &&\
apt update &&\
apt install -y python3.8 &&\
apt install -y python3-pip &&\
apt-get install -y libsqlite3-dev &&\
apt install -y git &&\
apt update &&\
pip3 install paho-mqtt &&\
git clone https://github.com/eclipse/paho.mqtt.c.git &&\
git clone https://github.com/azadkuh/sqlite-amalgamation &&\
apt-get install -y libssl-dev&&\
cd ./paho.mqtt.c &&\
make &&\
make install &&\
apt install -y sqlite &&\
apt-get install -y  sqlite3 &&\
apt install -y supervisor
WORKDIR /vish1
RUN git clone https://github.com/Vishnupvj/MQTTDockersample.git
WORKDIR /vish1/MQTTDockersample
RUN make
RUN cp mosquitto.conf /etc/mosquitto/mosquitto.conf
VOLUME /vish1
CMD ["supervisord","-n","-c","supervisord.conf"]
