/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "MQTTClientMbedOs.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed_events.h"
#include "NTPClient.h"
#include "EthernetInterface.h"

#define MQTT_SERVER_HOST_NAME   "test.mosquitto.org" 
#define MQTT_SERVER_PORT        (1883)
#define MQTT_SERVER_USER        ""
#define MQTT_SERVER_PASSWORD    ""
#define MQTT_DEVICE_ID          "JustForDemo"
#define MQTT_PUB_TOPIC          "aaeon_lyd/gutc/to_broker"
#define MQTT_SUB_TOPIC          "aaeon_lyd/gutc/from_broker"

#define LED_ON                  (0)
#define LED_OFF                 (1)

// Blinking rate in milliseconds
#if (MBED_VERSION >= MBED_ENCODE_VERSION(6, 2, 0))
#define BLINKING_RATE  500ms
#else
#define BLINKING_RATE  500
#endif

/* Flag to be set when a message needs to be published, i.e. BUTTON is pushed. */
static volatile bool isPublish = false;
/* Flag to be set when received a message from the server. */
static volatile bool isMessageArrived = false;
/* Buffer size for a receiving message. */
const int MESSAGE_BUFFER_SIZE = 1024;
/* Buffer for a receiving message. */
char messageBuffer[MESSAGE_BUFFER_SIZE];

// Function prototypes
void handleMqttMessage(MQTT::MessageData& md);
void handleButtonRise();

int main()
{
    mbed_trace_init();
    // Initialise the digital pin LED_RED/LED_BLUE/LED_GREEN as an output
    DigitalOut led_red(LED_RED);
    DigitalOut led_blue(LED_BLUE);
    DigitalOut led_green(LED_GREEN);

    NetworkInterface* network = NULL;
    EthernetInterface* eth = NULL;
    #if 0
    eth = new EthernetInterface();
    network = eth;
    #endif
    network = NetworkInterface::get_default_instance();
    if (!network) {
        printf("Unable to open network interface.\r\n");
        return -1;
    }

    nsapi_error_t net_status = NSAPI_ERROR_NO_CONNECTION;
    while ((net_status = network->connect()) != NSAPI_ERROR_OK) {
        printf("Unable to connect to network (%d). Retrying...\r\n", net_status);
    }

    #if (MBED_VERSION >= MBED_ENCODE_VERSION(5, 15, 0))
        SocketAddress sktAddr;
        while ((net_status = network->get_ip_address(&sktAddr)) != NSAPI_ERROR_OK) {
            printf("Unable to get IP address (%d). Retrying..\r\n", net_status);
        };
        printf("Connected to the network successfully. IP address: %s\r\n", sktAddr.get_ip_address());
    #else
        printf("Connected to the network successfully. IP address: %s\r\n", network->get_ip_address());
        printf("\r\n");
    #endif

    // sync the real time clock (RTC)
    {
        NTPClient ntp(network);
        ntp.set_server("time.google.com", 123);
        time_t now = ntp.get_timestamp();
        if (now <= 0) {
            printf("Failed to retrieve the time from time.google.com:123\r\n");
            return 1;
        }
        set_time(now);
        printf("Time is now %s", ctime(&now));
    }

    TCPSocket *socket = new TCPSocket;
    printf("Connecting to host %s:%d ...\r\n", MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
    {
        nsapi_error_t ret = socket->open(network);
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not open socket! Returned %d\n", ret);
            return -1;
        }
        #if (MBED_VERSION >= MBED_ENCODE_VERSION(5, 15, 0))
        {
            SocketAddress sa;
            network->gethostbyname(MQTT_SERVER_HOST_NAME, &sa);
            sa.set_port(MQTT_SERVER_PORT);
            ret = socket->connect(sa);
        }
        #else
        ret = socket->connect(MQTT_SERVER_HOST_NAME, MQTT_SERVER_PORT);
        #endif
        if (ret != NSAPI_ERROR_OK) {
            printf("Could not connect! Returned %d\n", ret);
            return -1;
        }
    }

    /* Establish a MQTT connection. */
    MQTTClient* mqttClient = new MQTTClient(socket);
    printf("MQTT client is connecting to the service ...\r\n");
    {
        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.MQTTVersion = 4; // 3 = 3.1 4 = 3.1.1
        data.clientID.cstring = (char*)MQTT_DEVICE_ID;
        data.username.cstring = (char*)MQTT_SERVER_USER;
        data.password.cstring = (char*)MQTT_SERVER_PASSWORD;

        int rc = mqttClient->connect(data);
        if (rc != MQTT::SUCCESS) {
            printf("ERROR: rc from MQTT connect is %d\r\n", rc);
            return -1;
        }
    }
    printf("Client connected.\r\n");
    printf("\r\n");


    const char *mqtt_topic_pub  = MQTT_PUB_TOPIC;
    const char *mqtt_topic_sub  = MQTT_SUB_TOPIC;
    
    // Enable button 1 for publishing a message.
    InterruptIn btn1(BUTTON1);
    btn1.rise(handleButtonRise);

    /* Subscribe a topic. */
    bool isSubscribed = false;
    printf("Client is trying to subscribe a topic \"%s\".\r\n", mqtt_topic_sub);
    {
        int rc = mqttClient->subscribe(mqtt_topic_sub, MQTT::QOS0, handleMqttMessage);
        if (rc != MQTT::SUCCESS) {
            printf("ERROR: rc from MQTT subscribe is %d\r\n", rc);
            return -1;
        }
        isSubscribed = true;
    }
    printf("Client has subscribed a topic \"%s\".\r\n", mqtt_topic_sub);
    printf("\r\n");

    while (true) {
        led_red = !led_red;
        led_green = LED_OFF;
        led_blue = LED_OFF;

        /* Client is disconnected. */
        if(!mqttClient->isConnected()){
            break;
        }
        /* Waits a message and handles keepalive. */
        if(mqttClient->yield(100) != MQTT::SUCCESS) {
            break;
        }
        /* Received a message. */
        if(isMessageArrived) {
            isMessageArrived = false;
            printf("\r\nMessage arrived:\r\n%s\r\n", messageBuffer);
            led_green = LED_ON;
        }

        if (isPublish) {
            printf("button1 fire\r\n");
            isPublish = false;
            static unsigned int id = 0;
            static unsigned int count = 0;
            led_blue = LED_ON;

            MQTT::Message message;
            message.retained = false;
            message.dup = false;
            const size_t len = 128;
            char buf[len];
            snprintf(buf, len, "Message #%d from %s.", count, MQTT_DEVICE_ID);
            message.payload = (void*)buf;
            message.qos = MQTT::QOS0;
            message.id = id++;
            message.payloadlen = strlen(buf);

            // Publish a message.
            printf("\r\nPublishing message to the topic %s:\r\n%s\r\n", mqtt_topic_pub, buf);
            int rc = mqttClient->publish(mqtt_topic_pub, message);
            if(rc != MQTT::SUCCESS) {
                printf("ERROR: rc from MQTT publish is %d\r\n", rc);
            }
            printf("Message published.\r\n");

            count++;
        }
        ThisThread::sleep_for(BLINKING_RATE);
    }
    printf("The client has disconnected.\r\n");

    if(mqttClient) {
        if(isSubscribed) {
            mqttClient->unsubscribe(mqtt_topic_sub);
            mqttClient->setMessageHandler(mqtt_topic_sub, 0);
        }
        if(mqttClient->isConnected())
            mqttClient->disconnect();
        delete mqttClient;
    }

    if(socket) {
        socket->close();
        delete socket;
    }

    if(network) {
        network->disconnect();
        // network is not created by new.
    }
    if (eth) {
        delete eth;
    }
}

/*
 * Callback function called when a message arrived from server.
 */
void handleMqttMessage(MQTT::MessageData& md)
{
    // Copy payload to the buffer.
    MQTT::Message &message = md.message;
    memcpy(messageBuffer, message.payload, message.payloadlen);
    messageBuffer[message.payloadlen] = '\0';

    isMessageArrived = true;
}

/*
 * Callback function called when button is pushed.
 */
void handleButtonRise() {
    isPublish = true;
}