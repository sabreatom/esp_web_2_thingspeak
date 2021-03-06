#*****************************************************************
# get_sensor_data.py - script which reads sensor value and sends to ThingSpeak
#
# Author: Aleksandrs Maklakovs
#
# Purpose: Update Thingspeak channel with new temperature/humidity values.
#
# Usage: 1. run ESP8266 with sensor WEB server
#	 2. modify network and ThingSpeak configuration values
#	 3. using Cron periodically run this scipt
#
# Return:    0 if finished OK
#            1 failed
#*****************************************************************

#!/usr/bin/env python

import httplib
import urllib
import sys
import datetime

#--------------------------------------------------------------
#Config values:
TEMP_STR = "Temperature: " #temperature template to compare with
HUM_STR = "humidity: " #humidity template to compare with
CH_API_KEY = "****************" #replace with Thingspeak write key

#Write log file function:
def log_wr(msg):
    log_f = open('sensor_log.txt','a') # Open log file:
    log_f.write('[' + str(datetime.datetime.now()) + '] ' + msg + '\n')
    log_f.close()

#Get data from server:
try:
    conn_sens = httplib.HTTPConnection("192.168.1.10",80)   #replace with sensor's IP address
except:
    print "Couldn't connect to sensor's web server."
    log_wr("Couldn't connect to sensor's web server.")
    sys.exit(1)

conn_sens.request("GET", "/")
r1 = conn_sens.getresponse()
if ((r1.status == 200) and (r1.reason == "OK")):
    sensor_data = r1.read()
else:
    print "Something wrong with http reply"
    log_wr("Something wrong with http reply")
    conn_sens.close()
    sys.exit(1)

conn_sens.close()

#Get temperature and humidity values from data:
t_ptr = sensor_data.find(TEMP_STR) + len(TEMP_STR)
h_ptr = sensor_data.find(HUM_STR) + len(HUM_STR)
temp_val = ""
hum_val = ""

while (sensor_data[t_ptr].isdigit()):
    temp_val += sensor_data[t_ptr]
    t_ptr += 1

while (sensor_data[h_ptr].isdigit()):
    hum_val += sensor_data[h_ptr]
    h_ptr += 1

#-------------------------------------------------------------
#Send data to Thing speak:

params = urllib.urlencode({'field1': int(temp_val), 'field2': int(hum_val),'key':CH_API_KEY})
headers = {"Content-type": "application/x-www-form-urlencoded","Accept": "text/plain"}

try:
    conn_cloud = httplib.HTTPConnection("api.thingspeak.com:80")
except:
    print "Couldn't connect to ThingSpeak server."
    log_wr("Couldn't connect to ThingSpeak server.")
    sys.exit(1)

conn_cloud.request("POST", "/update", params, headers)
response_cloud = conn_cloud.getresponse()

if ((response_cloud.status == 200) and (response_cloud.reason == "OK")):
    print "Data sent to ThingSpeak."
    log_wr("Data sent to ThingSpeak.")
    dump = response_cloud.read()
    conn_cloud.close()
    sys.exit(0)
else:
    print "Data not sent to ThingSpeak, something wrong during transfer."
    log_wr("Data not sent to ThingSpeak, something wrong during transfer.")
    conn_cloud.close()
    sys.exit(1)
