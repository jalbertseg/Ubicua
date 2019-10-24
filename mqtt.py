import paho.mqtt.client as mqtt
import os, urllib.parse
import datetime
from pymongo import MongoClient

# Define event callbacks
def on_connect(client, userdata, flags, rc):
    print(("rc: " + str(rc)))

def on_message(client, obj, msg):
    print(str(datetime.datetime.now()) + ": " + msg.topic + " " + str(msg.payload))

    """ Guardamos todo en una bbdd de mongodb"""
    receiveTime=datetime.datetime.now()
    message=msg.payload.decode("utf-8")
    isfloatValue=False
    try:
        # Convert the string to a float so that it is stored as a number and not a string in the database
        val = float(message)
        isfloatValue=True
    except:
        isfloatValue=False

    if isfloatValue:
        print(str(receiveTime) + ": " + msg.topic + " " + str(val))
        post={"time":receiveTime,"topic":msg.topic,"value":val}
    else:
        print(str(receiveTime) + ": " + msg.topic + " " + message)
        post={"time":receiveTime,"topic":msg.topic,"value":message}

    collection.insert_one(post)




def on_publish(client, obj, mid):
    print(("mid: " + str(mid)))

def on_subscribe(client, obj, mid, granted_qos):
    print(("Subscribed: " + str(mid) + " " + str(granted_qos)))

def on_log(client, obj, level, string):
    print(string)

mqttc = mqtt.Client()

# Assign event callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_publish = on_publish
mqttc.on_subscribe = on_subscribe

# Uncomment to enable debug messages
mqttc.on_log = on_log

# Parse CLOUDMQTT_URL (or fallback to localhost)
url_str = os.environ.get('CLOUDMQTT_URL', 'mqtt://192.168.2.178:1883')
url = urllib.parse.urlparse(url_str)
topic = url.path[1:] or 'sensor/#'

# Connect
mqttc.username_pw_set("admin", "mypw91885")
mqttc.connect(url.hostname, url.port)

# Start subscribe, with QoS level 0
mqttc.subscribe(topic, 0)

mongoClient = MongoClient()
db= mongoClient.SensorData
collection=db.home_data


# Continue the network loop, exit when an error occurs
rc = 0
while rc == 0:
    rc = mqttc.loop()
print(("rc: " + str(rc)))
