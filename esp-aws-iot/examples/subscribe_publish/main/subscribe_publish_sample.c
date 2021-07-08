/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Additions Copyright 2016 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
/**
 * @file subscribe_publish_sample.c
 * @brief simple MQTT publish and subscribe on the same topic
 *
 * This example takes the parameters from the build configuration and establishes a connection to the AWS IoT MQTT Platform.
 * It subscribes and publishes to the same topic - "test_topic/esp32"
 *
 * Some setup is required. See example README for details.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

static const char *TAG = "subpub";

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

//#define EXAMPLE_WIFI_SSID	"D3377"
//#define EXAMPLE_WIFI_PASS "12345678"
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;


/* CA Root certificate, device ("Thing") certificate and device
 * ("Thing") key.

   Example can be configured one of two ways:

   "Embedded Certs" are loaded from files in "certs/" and embedded into the app binary.

   "Filesystem Certs" are loaded from the filesystem (SD card, etc.)

   See example README for more details.
*/
#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)

static const char * DEVICE_CERTIFICATE_PATH = CONFIG_EXAMPLE_CERTIFICATE_PATH;
static const char * DEVICE_PRIVATE_KEY_PATH = CONFIG_EXAMPLE_PRIVATE_KEY_PATH;
static const char * ROOT_CA_PATH = CONFIG_EXAMPLE_ROOT_CA_PATH;

#else
#error "Invalid method for loading certs"
#endif

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    ESP_LOGI(TAG, "Subscribe callback");
    ESP_LOGI(TAG, "%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)params->payload);
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
    ESP_LOGW(TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == pClient) {
        return;
    }

    if(aws_iot_is_autoreconnect_enabled(pClient)) {
        ESP_LOGI(TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            ESP_LOGW(TAG, "Manual Reconnect Successful");
        } else {
            ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}

void aws_iot_task(void *param) {
    char cPayload[100];

    int32_t i = 0;

    IoT_Error_t rc = FAILURE;

    AWS_IoT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    IoT_Publish_Message_Params paramsQOS0;
    IoT_Publish_Message_Params paramsQOS1;

    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;

#if defined(CONFIG_EXAMPLE_EMBEDDED_CERTS)
    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

#elif defined(CONFIG_EXAMPLE_FILESYSTEM_CERTS)
    mqttInitParams.pRootCALocation = ROOT_CA_PATH;
    mqttInitParams.pDeviceCertLocation = DEVICE_CERTIFICATE_PATH;
    mqttInitParams.pDevicePrivateKeyLocation = DEVICE_PRIVATE_KEY_PATH;
#endif

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

#ifdef CONFIG_EXAMPLE_SDCARD_CERTS
    ESP_LOGI(TAG, "Mounting SD card...");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
    };
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card VFAT filesystem. Error: %s", esp_err_to_name(ret));
        abort();
    }
#endif

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        abort();
    }

    /* Wait for WiFI to show as connected */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in the menuconfig of the example */
    connectParams.pClientID = CONFIG_AWS_EXAMPLE_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(TAG, "Connecting to AWS...");
    do {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if(SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    } while(SUCCESS != rc);

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client,  true);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }

    const char *TOPIC = "esp32/2";
    const int TOPIC_LEN = strlen(TOPIC);

    ESP_LOGI(TAG, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, TOPIC, TOPIC_LEN, QOS0, iot_subscribe_callback_handler, NULL);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        abort();
    }

    sprintf(cPayload, "%s : %d ", "hello from SDK", i);

	typedef struct __attribute__((aligned(4), packed)) {
		uint16_t counter;
		uint16_t user_id;
		uint16_t channel_code;
		uint16_t eeg_data_len;
		int16_t eeg_data[1017];
	} aws_msg_t;
	
	/*aws_msg_t msg;
	msg.counter = 1;
	msg.user_id = 1;
	msg.channel_code = 1;
	msg.eeg_data_len = 1017;
	int16_t data[] = {38,48,51,44,48,56,56,41,20,-3,-9,-1,6,5,6,17,24,38,56,57,52,52,48,37,39,51,51,40,37,34,27,25,23,21,25,37,43,45,53,57,60,71,73,67,58,51,40,40,53,72,88,89,88,92,88,81,92,117,124,119,123,140,144,140,147,150,154,152,140,131,122,122,133,148,162,169,171,162,146,135,139,154,162,154,145,150,164,166,156,140,134,150,168,156,123,107,112,107,103,103,109,128,136,130,115,96,86,100,124,131,100,64,56,76,105,128,118,93,66,48,44,48,53,53,48,33,33,41,56,70,72,69,69,86,96,80,67,67,69,60,53,52,53,54,54,54,61,70,68,68,71,72,69,70,77,76,66,60,71,68,48,19,19,50,67,58,49,39,34,42,67,80,70,59,55,38,24,37,59,70,54,26,16,11,19,33,41,41,34,27,25,34,32,5,-12,-1,29,41,27,9,4,7,8,-3,-11,-9,-2,-6,-22,-35,-34,-17,-7,-12,-12,2,10,4,5,10,18,16,12,24,26,21,20,20,25,28,25,21,10,-11,-37,-42,-34,-13,5,16,10,-2,-21,-34,-25,-4,4,-9,-29,-40,-35,-22,-14,-8,-2,0,-11,-22,-25,-38,-53,-54,-52,-52,-49,-38,-38,-41,-41,-37,-35,-36,-30,-29,-44,-52,-37,-17,-10,-23,-37,-35,-20,-6,2,4,7,19,22,21,21,9,-3,0,1,-4,-10,-11,-5,-2,2,10,18,18,11,17,21,7,-7,0,16,12,5,3,4,3,1,5,17,19,9,2,2,2,-2,-7,-17,-22,-27,-22,-13,-12,-13,-5,1,10,24,27,27,33,42,40,21,-1,-8,-4,1,-3,-21,-38,-43,-41,-35,-22,-8,-6,-6,4,18,34,56,71,70,67,69,69,60,48,38,39,44,38,18,6,20,42,54,81,98,84,53,37,43,49,22,43,80,97,102,114,120,113,98,85,89,107,114,108,102,86,83,100,117,121,116,102,76,64,67,67,65,68,82,96,100,90,84,96,108,109,99,81,59,45,38,23,6,-5,1,12,12,4,-17,-40,-51,-53,-62,-60,-53,-55,-73,-99,-119,-120,-99,-67,-55,-70,-71,-72,-68,-54,-50,-46,-37,-20,-3,0,-5,4,22,37,49,51,52,52,59,65,58,57,58,52,42,41,33,19,11,17,34,41,22,-1,-18,-36,-43,-35,-36,-61,-78,-76,-70,-66,-60,-62,-67,-60,-52,-52,-50,-33,-5,5,-2,-11,-13,-26,-43,-62,-87,-102,-93,-82,-87,-92,-84,-59,-43,-34,-29,-44,-54,-51,-54,-51,-40,-34,-33,-35,-35,-42,-51,-49,-35,-22,-21,-24,-18,-8,-4,-7,-12,-20,-35,-55,-71,-72,-65,-53,-42,-49,-54,-44,-24,-13,-35,-59,-70,-59,-42,-36,-17,-1,4,6,10,27,41,51,53,38,17,7,4,44,52,35,26,29,36,23,5,4,5,-1,-9,-8,3,13,17,18,25,48,70,69,57,58,59,54,55,57,54,44,42,64,82,91,90,75,53,38,50,59,53,52,59,60,57,61,69,75,88,112,132,144,148,149,151,147,137,131,133,139,139,129,115,105,72,55,57,67,68,57,45,40,40,38,33,37,44,52,68,74,69,54,38,40,52,58,58,45,44,65,86,92,104,116,119,114,103,103,117,137,134,104,72,54,58,73,88,83,57,44,56,85,103,102,90,82,85,91,86,76,60,43,53,70,66,56,65,85,88,86,80,60,49,33,23,37,61,81,80,66,59,64,64,56,52,51,48,33,11,0,-6,-7,-2,4,11,22,37,54,69,80,80,77,82,80,70,52,39,41,48,48,51,58,75,81,76,77,86,84,72,69,72,72,73,73,73,88,103,104,98,85,71,59,52,53,54,48,24,17,20,28,28,27,32,29,24,27,48,66,74,68,53,40,38,42,38,20,4,-1,3,4,1,3,7,13,27,37,35,26,22,23,23,26,35,42,52,57,49,35,36,38,40,48,51,40,36,51,71,86,96,92,86,86,97,90,89,80,72,92,117,124,123,133,144,136,130,119,117,119,112,92,77,68,64,66,66,64,65,68,74,67,60,74,88,83,67,59,68,64,41,19,5,8,13,7,2,1,8,11,12,17,19,21,19,12,9,4,4,8,16,24,35,25,20,38,61,72,75,69,52,38,40,52,65,76,82,66,40,20,16,19,21,16,-2,-17,-7,17,32,40,44,37,27,21,23,27,29,25,20,22,26,22,25,29,17,2,-6,-5,-2,-9,-11,-9,-4,4,17,29,33,28,28,35,40,58,82,96,92,92,103,118,136,146,145,134,117,105,91,72,66,69,76,77,67,50,32,13,10,17,20,32,43,38,32,32,38,41,49,58,64,55,49,52,60,67,60,65,74,75,72,73,88,89,75,65,65,70,70,58,43,41,56,75,87,72,50,42,52,60,69,71,60,51,45,52,64,75,84,83,81};
    memcpy(msg.eeg_data,data,msg.eeg_data_len*2);
	printf("%d\n",sizeof(msg));
	
    paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) &msg;
    paramsQOS0.isRetained = 0;
	paramsQOS0.payloadLen = sizeof(msg);
	
    aws_msg_t msg1;
	msg1.counter = 2;
	msg1.user_id = 1;
	msg1.channel_code = 1;
	msg1.eeg_data_len = 889;
    int16_t data1[] = {83,74,65,65,66,55,43,25,18,20,26,33,41,38,28,36,55,60,56,58,72,85,76,69,74,83,86,88,97,112,114,109,99,76,54,50,66,86,97,99,96,86,82,73,69,70,70,60,65,71,69,64,57,54,56,58,50,20,-20,-56,-74,-69,-57,-57,-60,-69,-75,-71,-59,-45,-24,3,13,10,8,10,5,7,8,-3,-22,-28,-20,1,3,-10,-18,-12,-19,-23,-25,-36,-45,-43,-37,-23,-4,4,-2,-3,-3,-2,5,8,7,5,16,29,34,35,36,48,68,80,80,75,68,69,77,89,105,100,80,64,56,59,64,57,49,42,48,60,75,77,70,61,45,28,24,28,29,34,40,50,42,27,16,4,1,6,12,16,20,24,34,44,54,58,53,40,24,11,4,-4,-14,-34,-56,-76,-77,-68,-53,-44,-38,-35,-30,-30,-30,-25,-6,18,29,27,23,27,41,50,51,57,57,51,55,57,58,53,37,20,9,12,29,49,49,37,28,37,57,76,83,74,68,50,25,9,13,39,71,103,121,130,132,136,146,163,184,188,171,152,152,164,166,155,139,131,133,134,137,136,122,112,109,116,125,138,139,130,116,113,123,144,162,168,165,149,124,114,114,118,61,51,48,49,43,45,53,55,39,25,26,35,41,41,37,27,21,18,22,25,13,-6,-14,-5,10,21,11,9,7,1,7,36,59,57,33,8,4,21,41,53,48,27,9,9,20,21,17,12,9,12,25,25,12,2,-4,-6,-6,2,3,-9,-21,-20,-12,3,-8,-17,-13,-8,-9,-12,-19,-26,-36,-52,-61,-58,-49,-41,-37,-33,-12,11,8,-2,5,20,26,26,18,11,20,27,29,17,-1,-2,5,28,52,50,28,8,7,17,9,-13,-37,-52,-57,-59,-56,-45,-34,-25,-24,-23,-23,-27,-24,-23,-34,-36,-33,-30,-34,-24,-51,-54,-50,-39,-23,-3,11,16,25,34,33,36,38,40,35,28,27,33,39,40,40,48,53,55,59,53,44,50,41,19,-6,-22,-41,-56,-72,-77,-82,-90,-103,-113,-117,-120,-123,-123,-130,-152,-161,-154,-153,-162,-173,-185,-189,-190,-189,-189,-180,-169,-177,-193,-188,-164,-133,-118,-124,-134,-122,-105,-103,-105,-104,-101,-90,-81,-81,-89,-101,-108,-109,-110,-104,-84,-55,-41,-40,-42,-45,-44,-43,-42,-46,-59,-67,-73,-92,-99,-72,-53,-51,-41,-24,-12,-13,-21,-35,-44,-44,-27,-11,-9,-11,-14,-24,-26,-17,5,17,5,-9,-13,-14,-21,-28,-36,-44,-60,-39,-25,-22,-20,-14,1,17,22,20,25,38,40,27,16,16,27,36,25,10,16,27,34,20,-5,-22,-12,8,23,20,20,20,20,23,19,13,11,22,44,49,27,8,1,1,8,19,18,18,24,27,27,18,10,18,21,11,11,24,39,43,43,56,76,88,75,81,92,104,109,113,113,112,112,113,116,120,124,131,132,134,135,134,135,148,152,156,161,156,152,152,155,160,157,150,137,136,140,145,140,135,122,113,103,104,103,96,86,76,66,58,66,75,81,76,71,73,81,82,80,75,71,71,72,72,74,82,88,77,58,48,43,34,26,28,36,44,51,41,45,69,92,98,91,85,66,53,57,64,69,67,56,57,69,67,51,34,22,25,49,73,83,81,76,85,92,102,108,108,112,101,76,59,56,56,52,49,50,45,45,58,84,102,115,118,121,135,144,147,147,136,16,19,26,35,37,28,21,20,24,35,39,41,41,36,32,33,39,28,16,16,20,24,28,37,41,42,37,29,35,38,36,35,34,38,48,57,64,55,49,67,87,91,87,81,65,52,53,53,42,44,68,90,101,91,82,81,81,66,38,23,6,-10,-25,-38,-41,-22,-3,7,18,27,34,41,54,59,60,64,71,73,64,67,86,108,113,103,97,100,96,86,81,72,67,67,64,61,58,56,53,50,49,53,57,67,81,88,84,74,69,59,38,18,4,0,4,17,26,33,41,54,59,57,44,38,49,58,60,67,66,50,32,22,34,54,58,53,52,54,37,18,11,9,12,20,21,16,7,9,24,27,17,-1,-26,-40,-28,-9,5,10,19,22,7,-12,-22,-19,-10,-8,-13,-18,-17,-12,-5,-5,-18,-24,-19,-17,-22,-25,-27,-35,-26,-10,-5,-8,-18,-11,8,23,21,9,2,-7,-6,8};
    memcpy(msg1.eeg_data,data1,msg1.eeg_data_len*2);
	printf("%d\n",sizeof(msg1));
    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *) &msg1;
    paramsQOS1.isRetained = 0;
    paramsQOS1.payloadLen = sizeof(msg1);*/
    
    aws_msg_t msg;
	msg.counter = 3;
	msg.user_id = 2;
	msg.channel_code = 1;
	msg.eeg_data_len = 952;
	int16_t data[] = {77,74,69,70,76,75,76,75,67,66,76,93,98,82,55,37,25,23,22,17,8,3,0,-7,-9,-4,-9,-13,-6,8,8,9,27,57,72,68,61,54,48,45,52,57,49,33,37,39,32,22,17,10,5,-2,-7,-6,4,11,20,21,26,27,19,4,-2,5,-3,-19,-30,-33,-27,-33,-40,-46,-45,-34,-24,-29,-40,-38,-21,-13,-19,-23,-23,-10,6,16,17,9,3,3,2,19,36,38,44,52,48,50,60,70,69,54,45,43,49,53,50,57,59,53,42,32,32,49,65,66,59,51,37,32,40,45,43,42,38,29,42,71,97,104,97,87,88,101,112,113,104,86,55,42,51,65,81,75,51,28,33,35,17,-11,-30,-26,-11,5,23,27,28,33,29,18,19,23,13,-1,-7,-5,5,19,29,26,19,21,32,36,39,49,53,51,50,54,48,34,24,25,38,52,61,70,67,69,85,91,85,69,55,65,83,76,57,53,56,64,73,84,83,80,73,59,45,42,49,66,82,77,65,64,87,115,125,118,96,80,74,72,70,73,77,74,68,69,80,76,64,58,71,98,116,114,108,104,92,90,97,92,88,82,67,48,40,43,54,64,59,53,28,8,8,20,21,9,-4,-2,7,9,9,17,24,26,27,33,34,33,24,23,24,23,19,21,26,34,39,50,64,69,73,85,91,82,66,60,69,70,72,91,107,100,80,56,40,44,52,52,35,8,6,26,44,56,55,56,71,90,96,92,99,113,122,118,103,98,99,93,88,84,81,82,83,84,84,71,65,56,45,45,67,84,74,51,40,52,66,59,45,39,38,35,37,53,81,88,60,34,23,22,28,37,35,33,27,21,22,28,36,28,16,8,7,7,6,16,32,42,40,33,21,6,-5,-1,7,-3,-12,-21,-29,-25,-14,-7,4,4,-8,0,24,35,34,34,48,64,65,50,33,24,25,27,26,33,44,59,64,56,58,80,96,107,120,124,118,101,90,108,125,137,153,161,155,140,129,117,118,124,124,117,113,114,114,116,129,139,146,138,133,135,132,-21,-4,12,17,-2,-19,-23,-12,8,27,35,28,13,9,17,26,28,16,-7,-24,-34,-39,-38,-35,-18,10,27,51,81,96,104,112,98,76,70,84,91,91,89,81,73,74,88,106,107,85,69,67,75,80,58,42,35,42,56,50,41,43,43,36,33,28,21,18,21,24,24,12,-3,5,23,28,34,53,71,77,91,121,140,134,116,97,88,84,76,66,43,20,6,1,-4,-3,2,7,10,12,17,18,19,18,17,27,41,39,25,8,9,20,22,13,2,-1,-1,-9,-10,-9,-17,-13,-10,-7,6,24,32,32,27,18,8,5,2,2,-1,-10,-20,-23,-20,-8,2,6,3,3,4,-1,-12,-23,-7,17,22,6,-5,4,16,20,22,32,40,34,23,21,25,34,27,20,6,0,9,27,38,48,50,39,42,72,96,87,71,53,38,37,37,33,18,-1,-6,-6,-4,11,28,34,26,20,20,23,13,2,-3,-4,-5,-11,-26,-40,-45,-51,-57,-54,-40,-28,-26,-33,-45,-50,-46,-52,-53,-43,-42,-57,-68,-62,-58,-61,-65,-44,-25,-6,20,41,52,51,44,49,51,48,41,29,9,-13,-17,5,21,12,4,12,25,24,23,24,27,35,36,28,23,22,26,40,49,39,27,34,39,38,24,8,3,13,24,18,7,5,10,36,50,53,54,61,68,73,81,92,96,88,83,74,77,90,89,80,75,75,74,70,58,51,52,54,53,54,54,44,38,42,55,66,73,71,59,49,50,57,67,66,61,60,73,92,101,87,54,29,37,58,73,82,81,66,42,24,13,10,7,12,20,11,-2,-19,-38,-49,-50,-36,-20,-18,-20,-21,-23,-29,-35,-40,-35,-17,-3,-10,-20,-20,-11,-5,-5,0,0,-6,-3,5,-3,-7,1,11,20,27,28,5,-20,-37,-50,-46,-36,-26,-29,-36,-45,-50,-39,-23,-12,-18,-21,-23,-24,-13,-6,-18,-39,-42,-35,-22,-10,-6,-2,-1,-13,-25,-33,-40,-43,-49,-62,-69,-55,-41,-33,-29,-34,-36,-33,-35,-41,-52,-55,-50,-46,-43,-37,-19,-1,7,10,5,-2,-10,-27,-33,-14,-1,-7,-26,-38,-24,6,33,33,11,9,33,39,23,18,27,39,41,40,42,24,20,25,40,42,48,57,64,64,54,42,33,11,0,-4,-7,-1,17,27,29,26,26,19,9,8,5,0,-5,-8,-4,5,19,27,38,43,44,39,40,50,54,49,43,53,67,86,93,87,72,59,59,70,84,81,72,69,61,57,56,56,65,70,75,82};
    memcpy(msg.eeg_data,data,msg.eeg_data_len*2);
	printf("%d\n",sizeof(msg));
	
    paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) &msg;
    paramsQOS0.isRetained = 0;
	paramsQOS0.payloadLen = sizeof(msg);
	
    aws_msg_t msg1;
	msg1.counter = 4;
	msg1.user_id = 2;
	msg1.channel_code = 1;
	msg1.eeg_data_len = 1015;
    int16_t data1[] = {75,69,53,35,24,27,41,53,51,32,18,4,-4,-1,10,22,17,9,8,10,21,27,28,25,20,9,-4,-10,-14,-14,-17,-18,-25,-34,-23,-7,-8,-12,-9,5,19,19,17,22,39,58,67,57,39,9,-11,-14,0,11,16,19,20,20,22,24,21,8,-7,5,39,60,56,38,20,7,13,22,26,20,3,-14,-22,-7,6,11,10,22,43,68,83,80,75,87,93,103,109,102,93,98,107,118,123,119,104,86,80,98,128,138,135,131,118,107,108,106,96,83,81,80,73,73,81,83,82,84,81,82,85,84,83,83,70,44,32,42,54,39,17,4,7,19,26,32,41,37,11,-2,-4,2,11,9,1,-2,2,0,-6,-1,23,53,69,58,43,52,68,67,54,33,9,-5,-4,11,23,26,27,29,35,41,43,51,64,65,56,61,76,90,99,104,106,97,80,77,80,64,49,43,28,19,20,19,5,-8,-7,-5,-3,7,21,33,48,53,43,40,55,77,80,68,64,87,116,129,131,128,132,150,170,170,155,164,185,187,184,185,182,171,163,150,139,139,140,144,141,150,172,185,188,188,200,219,241,253,260,258,252,256,262,259,247,235,244,260,258,251,249,253,314,410,492,544,578,600,618,632,644,642,615,567,515,470,439,423,433,454,454,424,380,348,329,307,280,253,234,217,188,168,167,172,168,149,119,80,56,53,50,32,21,23,11,-5,-26,-43,-45,-35,-35,-52,-75,-93,-99,-89,-65,-51,-61,-81,-91,-103,-115,-130,-133,-131,-140,-158,-162,-158,-157,-154,-151,-148,-149,-149,-161,-178,-185,-180,-168,-162,-163,-165,-163,-161,-164,-162,-149,-136,-131,-148,-178,-201,-199,-183,-171,-167,-172,-174,-174,-172,-172,-173,-179,-182,-174,-167,-165,-165,-166,-165,-165,-170,-181,-174,-150,-120,-109,-117,-121,-114,-97,-94,-101,-97,-87,-69,-56,-60,-70,-82,-94,-104,-107,-97,-82,-73,-81,-82,-68,-49,-36,-28,-25,-25,-23,-22,-23,-23,-20,-18,-17,-19,-23,-29,-28,-13,-8,-17,-27,-38,-50,-56,-54,-40,-29,-36,-41,-34,-13,-2,0,-5,-4,-1,1,1,-6,-12,-18,-25,-43,-56,-56,-53,-46,-37,-24,-22,-21,-10,-4,2,4,-5,-14,-19,-13,-10,-22,-37,-44,-52,-53,-49,-50,-56,-55,-53,-37,-18,-3,-3,-11,-11,-14,-11,-1,21,40,43,40,42,52,55,53,49,41,36,23,12,5,6,12,18,16,10,7,7,4,-11,-26,-21,-6,0,-1,0,5,10,16,16,10,7,4,-1,-1,2,0,-10,-27,-36,-28,-10,2,0,-9,-19,-22,-25,-23,-18,-6,4,1,-12,-11,-4,4,13,3,-14,-11,12,45,60,67,71,70,65,57,57,58,41,8,-18,-20,-9,1,16,33,45,65,86,91,86,84,81,75,67,52,39,34,36,33,26,21,18,13,12,3,-5,4,20,26,33,48,68,87,92,85,73,65,56,50,43,45,58,59,53,54,45,26,18,24,29,23,25,39,43,24,-4,-12,-1,12,21,21,18,17,24,32,28,27,29,35,25,8,-3,-21,-33,-21,-3,9,19,22,27,35,40,51,53,41,36,38,29,20,13,10,3,-4,-10,-13,-13,-18,-17,-9,-4,-3,-4,0,0,-5,-13,-20,-14,5,12,2,-3,5,32,55,66,64,50,33,21,18,26,44,55,48,27,10,3,11,37,56,45,33,34,48,59,60,50,49,69,71,56,42,43,64,67,51,33,48,50,41,48,50,38,16,4,17,21,10,6,10,16,8,4,8,10,-1,-14,-18,-11,-8,2,22,42,44,27,0,-13,-9,-5,-11,-23,-39,-40,-23,1,24,40,50,56,59,70,88,92,76,75,97,112,99,75,76,98,104,98,87,82,73,61,72,84,83,66,66,58,59,76,86,65,35,36,51,53,42,39,34,29,33,32,28,39,56,66,52,43,48,52,51,49,49,42,40,43,43,36,42,49,43,40,28,7,-8,-6,3,16,21,16,7,7,33,64,69,54,38,21,3,-11,-22,-26,-22,-18,-11,-3,5,5,1,-8,3,11,7,-1,-9,-12,-9,-5,-5,4,21,35,37,28,9,-11,-23,-20,-22,-25,-20,-4,16,27,24,16,21,44,70,75,68,59,60,69,84,101,97,72,53,48,57,64,61,55,44,38,37,29,9,-5,1,18,27,19,7,12,21,23,19,11,20,33,23,26,43,64,86,89,67,41,43,56,54,53,58,67,67,67,72,68,54,42,34,22,5,-11,-14,-7,-3,-2,-5,-21,-41,-45,-42,-34,-20,-14,-24,-37,-38,-26,-17,-17,-4,6,5,-5,-12,-10,-2,-4,-26,-43,-35,-9,16,27,29,36,56,80,91,92,76,74,67,52,22,-11,-27,-23,-7,10,25,21,3,-4,3,3,0,-2,1,4,12,25,27,39,45,54,67,76,85,90,91,80,59,56,66,59,42,32,32,29,20,7,10,21,33,49,76,96,92,73,52,40,49,66,67,64,83,99,93,89,87,89,106};
    memcpy(msg1.eeg_data,data1,msg1.eeg_data_len*2);
	printf("%d\n",sizeof(msg1));
    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *) &msg1;
    paramsQOS1.isRetained = 0;
    paramsQOS1.payloadLen = sizeof(msg1);

    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }

        ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(1000 / portTICK_RATE_MS);
        rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &paramsQOS0);		
        vTaskDelay(1000 / portTICK_RATE_MS);
        rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &paramsQOS1);
        if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
            ESP_LOGW(TAG, "QOS1 publish ack not received.");
            rc = SUCCESS;
        }
    }

    ESP_LOGE(TAG, "An error occurred in the main loop.");
    abort();
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}


void app_main()
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    initialise_wifi();
    xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", 30000, NULL, 5, NULL, 1);
}
