import paho.mqtt.client as mqtt

"""
mqttc = mqtt.Client("python_pub")
mqttc.connect("127.0.0.1",1883)
mqttc.publish("myTopic","1,2,3,4,5 !@@#$%%^&")
"""


def sendMessage(msg):
	mqttc = mqtt.Client("python_pub")
	mqttc.connect("192.168.0.12",1883)
	mqttc.publish("myTopic",msg)


