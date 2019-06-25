import paho.mqtt.client as mqtt

def on_connect(client,userdata,flags,rc):
	print "Connected with result cod",str(rc)
	client.subscribe("myTopic")

def on_message(client,userdata,msg):
	print "\n",msg.topic,"",str(msg.payload)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message#on_message callback set

client.connect("127.0.0.1",1883,60)
client.loop_forever()
