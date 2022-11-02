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

//#include "VL6180.h"
#include "ultrasonic.h"
 
#include "mbed.h"
#include "XBeeLib.h"
#if defined(ENABLE_LOGGING)
#include "DigiLoggerMbedSerial.h"
#include "DHT11.h"

Ticker t1;
 
void send_temp_and_hum();

using namespace DigiLog;
#endif

#define REMOTE_NODE_ADDR64_MSB  ((uint32_t)0x0013A200)

//#error "Replace next define with the LSB of the remote module's 64-bit address (SL parameter)"
#define REMOTE_NODE_ADDR64_LSB  ((uint32_t)0x0013A20041B3C6CF)

#define REMOTE_NODE_ADDR64      UINT64(REMOTE_NODE_ADDR64_MSB, REMOTE_NODE_ADDR64_LSB)

#define PLAY_DURATION 7000
#define MAX_VALUE 4000
#define DATA_DIFF 2750

void dist(int distance)
{
    
}

//VL6180 rf(I2C_SDA, I2C_SCL); //I2C sda and scl
ultrasonic mu(D7, D9, 1, 1, &dist);    //Set the trigger pin to D7 and the echo pin to D9
                                        //have updates every .1 seconds and a timeout after 1
                                        //second, and call dist when the distance changes
                                        
DHT11 d(D13);

using namespace XBeeLib;

int reading;

Serial *log_serial;

char data[] = "";

static float Temper = 0.0, Humi = 0.0;

int cnt = 0;

//XBeeZB xbee = XBeeZB(RADIO_TX, RADIO_RX, RADIO_RESET, NC, NC, 9600);

static void send_data_to_coordinator(XBeeZB& xbee)
{
    // формируем текст сообщения
    //sprintf(data, "%.2f", reading);
    //sprintf(data, "{Gdn:{DHT:{Temp:%.2f,Hum:%.2f}}}", Temper, Humi);
    sprintf(data, "%.2f,%.2f,%d", Temper, Humi, reading);
    
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
    log_serial->printf("Sample application to demo how to send unicast and broadcast data with the XBeeZB\r\n\r\n");
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
    
    //t1.attach(&send_temp_and_hum, 0.5);
    
    // начинаем считывать данные с датчика
    bool sent = false;
    bool here = false;
    char sendData = '\0';
    //float prevVal = DATCHIK_MAX;
    
    int prev_data = MAX_VALUE;
    int curr_data = prev_data;
 
    mu.startUpdates();//start mesuring the distance
   
    while(true)
    {   
        mu.checkDistance();
        curr_data = mu.getCurrentDistance();
        
        wait_ms(2000);
        if(cnt < 300) cnt++;
        else cnt = 0;
        
        int error = 0;      
        
        if (/*curr_data <= 5000*/1) // исключаем "мусорные" показатели датчика
        {
            printf("Distance changed to %d cm     %d\r\n", curr_data/10, cnt);
            
            if (((curr_data + prev_data)/2 < 750) && (!here))
            {
                here = true;
                reading = 1;
                
                error = d.readData();
                Temper = d.readTemperature();
                Humi = d.readHumidity();
                
                send_data_to_coordinator(xbee);
                
                wait_ms(PLAY_DURATION);
            }
            else if (((curr_data + prev_data)/2 > 1000) && (here))
            {
                here = false;
                reading = 0;
                
                error = d.readData();
                Temper = d.readTemperature();
                Humi = d.readHumidity();
                
                send_data_to_coordinator(xbee);
                
                wait_ms(PLAY_DURATION);
            }
            prev_data = curr_data;
        }
        
        if (cnt == 149)
        {
            reading = 3;
            
            error = d.readData();
            Temper = d.readTemperature();
            Humi = d.readHumidity();
                
            send_data_to_coordinator(xbee);        
        }  
    }
    delete(log_serial);
}

void send_temp_and_hum()
{
    XBeeZB xbee = XBeeZB(RADIO_TX, RADIO_RX, RADIO_RESET, NC, NC, 9600);
    
    reading = 3;
    
    d.readData();
    Temper = d.readTemperature();
    Humi = d.readHumidity(); 
    
    //send_data_to_coordinator(xbee);
}
