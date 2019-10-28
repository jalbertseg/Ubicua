#### Library imports
# MQTT library
import paho.mqtt.client as mqtt
# OS library
import os, urllib.parse
import datetime
# Mongo database library
from pymongo import MongoClient

# Define event callbacks
def on_connect(client, userdata, flags, rc):
    ### print data into console
    print(("rc: " + str(rc)))

def on_message(client, obj, msg):
    ### print data into console
    print(str(datetime.datetime.now()) + ": " + msg.topic + " " + str(msg.payload))

    """ Save data into mongo database """
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
        ### print data into console
        print(str(receiveTime) + ": " + msg.topic + " " + str(val))
        ### Prepare mqtt message to save into database
        post={"time":receiveTime,"topic":msg.topic,"value":val}
    else:
        ### print data into console
        print(str(receiveTime) + ": " + msg.topic + " " + message)
        ### Prepara mqtt message to save into database
        post={"time":receiveTime,"topic":msg.topic,"value":message}

    collection.insert_one(post)

def on_publish(client, obj, mid):
    ### print data into console
    print(("mid: " + str(mid)))

def on_subscribe(client, obj, mid, granted_qos):
    ### print data into console
    print(("Subscribed: " + str(mid) + " " + str(granted_qos)))

def on_log(client, obj, level, string):
    ### print data into console
    print(string)


### Object MQTT Client 
mqttc = mqtt.Client()

# Assign event callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_publish = on_publish
mqttc.on_subscribe = on_subscribe

# Enable debug messages into console
mqttc.on_log = on_log

# Parse CLOUDMQTT_URL (or fallback to localhost)
url_str = os.environ.get('CLOUDMQTT_URL', 'mqtt://192.168.2.178:1883')
url = urllib.parse.urlparse(url_str)
### Subscription topic
topic = url.path[1:] or 'sensor/#'

# Connect
mqttc.username_pw_set("admin", "mypw91885")
mqttc.connect(url.hostname, url.port)

# Start subscribe, with QoS level 0
mqttc.subscribe(topic, 0)

### Object Mongo Object 
mongoClient = MongoClient()
db= mongoClient.SensorData
collection=db.home_data

# Continue the network loop, exit when an error occurs
rc = 0
while rc == 0:
    rc = mqttc.loop()
print(("rc: " + str(rc)))