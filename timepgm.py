import paho.mqtt.client as mqtt #import the client1
from datetime import timezone
import time
import datetime

broker_address="localhost"
port = 1883
#broker_address="iot.eclipse.org"
print("creating new instance")
client = mqtt.Client("P1") #create new instance
print("connecting to broker")
client.connect(broker_address,port) #connect to broker
print("connected to:", broker_address)
while True:
    dt = datetime.datetime.now(timezone.utc)
    print(dt)
    time.sleep(1)
    print("Publishing message to topic", "utc_time")
    client.publish("utc_time", str(dt))