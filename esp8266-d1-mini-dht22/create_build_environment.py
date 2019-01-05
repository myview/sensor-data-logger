#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import json

with open('config.json', mode='r') as config_file:

    cfg = json.loads(config_file.read())

    """
    Expecting Configuration file like:
    {
        "WirelessSSID"  : "YourSSID",
        "WirelessWPA2"  : "YourWPA2Password",
        "MQTTServer"    : "192.168.0.1",
        "MQTTTport"     : 1883,
        "MQTTTopicName" : "house/sensor/",
        "SensorID"      : "02",
        "IntervalMS"    : 180000
    }
    """

    print(f"-DWLAN_SSID=\'{cfg['WirelessSSID']}\'")
    print(f"-DWLAN_WPA2={cfg['WirelessWPA2']}")
    print(f"-DMQTT_IP={cfg['MQTTServer']}")
    print(f"-DMQTT_PORT={cfg['MQTTTport']}")
    print(f"-DMQTT_TOPIC={cfg['MQTTTopicName']}")
    print(f"-DID={cfg['SensorID']}")
    print(f"-DINTERVAL={cfg['IntervalMS']}")
