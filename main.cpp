/**
 * Copyright (c) 2015 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

 
#include "mbed.h"
#include "XBeeLib.h"
#if defined(ENABLE_LOGGING)
#include "DigiLoggerMbedSerial.h"

using namespace DigiLog;
#endif

#define REMOTE_NODE_ADDR64_MSB  ((uint32_t)0x0013A200)

//#error "Replace next define with the LSB of the remote module's 64-bit address (SL parameter)"
#define REMOTE_NODE_ADDR64_LSB  ((uint32_t)0x0013A20041B3C6CF)

#define REMOTE_NODE_ADDR64      UINT64(REMOTE_NODE_ADDR64_MSB, REMOTE_NODE_ADDR64_LSB)

#define PLAY_DURATION 7000
#define MAX_VALUE 4000
#define DATA_DIFF 2750
                                    
using namespace XBeeLib;

int SendTimeout = 0;

Serial *log_serial;

DigitalOut led(PC_13);

// read the input on analog pin 0:

AnalogIn FR(PA_0);
static int sensorFRValue = FR.read();
// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
static float voltageFR = 0.0;

AnalogIn Rain(PA_7);
static int sensorRainValue = Rain.read();
static float voltageRain = 0.0;

char data[] = "";

static void send_data_to_coordinator(XBeeZB& xbee)
{
    // формируем текст сообщения
    sprintf(data, "%.2f,%.2f", voltageFR, voltageRain);
    //sprintf(data, "%.2f", voltageRain);
    
    const uint16_t data_len = strlen(data);

    const TxStatus txStatus = xbee.send_data_to_coordinator((const uint8_t *)data, data_len);
    if (txStatus == TxStatusSuccess)
        log_serial->printf("send_data_to_coordinator OK\r\n");
    else
        log_serial->printf("send_data_to_coordinator failed with %d\r\n", (int) txStatus);
}

static void send_broadcast_data(XBeeZB& xbee)
{
    const char data[] = "Hi, everyone!";
    const uint16_t data_len = strlen(data);

    const TxStatus txStatus = xbee.send_data_broadcast((const uint8_t *)data, data_len);
    if (txStatus == TxStatusSuccess)
        log_serial->printf("send_broadcast_data OK\r\n");
    else
        log_serial->printf("send_broadcast_data failed with %d\r\n", (int) txStatus);
}

static void send_data_to_remote_node(XBeeZB& xbee, const RemoteXBeeZB& RemoteDevice)
{
    const char data[] = "Hi, Daniil!";
    const uint16_t data_len = strlen(data);

    const TxStatus txStatus = xbee.send_data(RemoteDevice, (const uint8_t *)data, data_len);
    if (txStatus == TxStatusSuccess)
        log_serial->printf("send_data_to_remote_node OK\r\n");
    else
        log_serial->printf("send_data_to_remote_node failed with %d\r\n", (int) txStatus);
}

static void send_explicit_data_to_remote_node(XBeeZB& xbee, const RemoteXBeeZB& RemoteDevice)
{
    char data[] = "send_explicit_data_to_remote_node";
    const uint16_t data_len = strlen(data);
    const uint8_t dstEP = 0xE8;
    const uint8_t srcEP = 0xE8;
    const uint16_t clusterID = 0x0011;
    const uint16_t profileID = 0xC105;

    const TxStatus txStatus = xbee.send_data(RemoteDevice, dstEP, srcEP, clusterID, profileID, (const uint8_t *)data, data_len);
    if (txStatus == TxStatusSuccess)
        log_serial->printf("send_explicit_data_to_remote_node OK\r\n");
    else
        log_serial->printf("send_explicit_data_to_remote_node failed with %d\r\n", (int) txStatus);
}

int main()
{
    log_serial = new Serial(DEBUG_TX, DEBUG_RX);
    log_serial->baud(9600);
    //log_serial->printf("Sample application to demo how to send unicast and broadcast data with the XBeeZB\r\n\r\n");
    log_serial->printf(XB_LIB_BANNER);

#if defined(ENABLE_LOGGING)
    new DigiLoggerMbedSerial(log_serial, LogLevelInfo);
#endif

    XBeeZB xbee = XBeeZB(RADIO_TX, RADIO_RX, RADIO_RESET, NC, NC, 9600);

    RadioStatus radioStatus = xbee.init();
    MBED_ASSERT(radioStatus == Success);

    /* Wait until the device has joined the network */
    log_serial->printf("Waiting for device to join the network: ");
    while (!xbee.is_joined()) {
        wait_ms(1000);
        log_serial->printf(".");
    }
    log_serial->printf("OK\r\n");

    const RemoteXBeeZB remoteDevice = RemoteXBeeZB(REMOTE_NODE_ADDR64);
    
    while (true)
    {  
        sensorFRValue = FR.read()*100;
        voltageFR = (sensorFRValue+0.0) * (5.0 / 1024.0)+0.0;
        
        sensorRainValue = Rain.read()*100;
        voltageRain = sensorRainValue+0.0;
        
        led = 1;
        if (SendTimeout == 299)
        {
            send_data_to_coordinator(xbee);
            SendTimeout = 0;
        }
        printf("OK\r\n");
        wait_ms(500);
        led = 0;
        wait_ms(500);
        SendTimeout++;
    }

    delete(log_serial);
}

void send_temp_and_hum()
{
    
}
