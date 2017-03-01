/* mqttexample.h
 *
 * Copyright (C) 2006-2016 wolfSSL Inc.
 *
 * This file is part of wolfMQTT.
 *
 * wolfMQTT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#ifndef WOLFMQTT_EXAMPLE_H
#define WOLFMQTT_EXAMPLE_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Default Configurations */
#define DEFAULT_CMD_TIMEOUT_MS  30000
#define DEFAULT_CON_TIMEOUT_MS  5000
#define DEFAULT_MQTT_QOS        MQTT_QOS_0
#define DEFAULT_KEEP_ALIVE_SEC  60
#define DEFAULT_CLIENT_ID       "IndigoSCADAMQTTClient"
#define WOLFMQTT_TOPIC_NAME     "indigoSCADAMQTT/example/"
//#define DEFAULT_TOPIC_NAME    WOLFMQTT_TOPIC_NAME"testTopic"
//#define DEFAULT_TOPIC_NAME      "$SYS/#"
//#define DEFAULT_TOPIC_NAME      "#"

#define PRINT_BUFFER_SIZE       80
#define MAX_PACKET_ID           ((1 << 16) - 1)


/* MQTT Client state */
typedef enum MQTTCtxState {
    WMQ_BEGIN = 0,
    WMQ_NET_INIT,
    WMQ_INIT,
    WMQ_TCP_CONN,
    WMQ_MQTT_CONN,
    WMQ_SUB,
    WMQ_PUB,
    WMQ_WAIT_MSG,
    WMQ_UNSUB,
    WMQ_DISCONNECT,
    WMQ_NET_DISCONNECT,
    WMQ_DONE,
} MQTTCtxState;

/* MQTT Client context */
typedef struct MQTTCtx {
    MQTTCtxState stat;
    int return_code;

    /* configuration */
    const char* app_name;
    word16 port;
    const char* host;
    int use_tls;
    MqttQoS qos;
    byte clean_session;
    word16 keep_alive_sec;
    const char* client_id;
    int enable_lwt;
    const char* username;
    const char* password;
    byte *tx_buf, *rx_buf;
    const char* topic_name;
    word32 cmd_timeout_ms;
    byte dump_mode;
    const char* pub_file;
    int retain;
#if defined(ENABLE_AZUREIOTHUB_EXAMPLE) || defined(ENABLE_AWSIOT_EXAMPLE)
    union {
    #ifdef ENABLE_AZUREIOTHUB_EXAMPLE
        char sasToken[400];
    #endif
    #ifdef ENABLE_AWSIOT_EXAMPLE
        char pubMsg[400];
    #endif
    } buffer;
#endif

    /* client and net containers */
    MqttClient client;
    MqttNet net;

    /* temp mqtt containers */
    MqttConnect connect;
    MqttMessage lwt_msg;
    MqttSubscribe subscribe;
    MqttUnsubscribe unsubscribe;
    MqttTopic topics[1], *topic;
    MqttPublish publish;
	/////////for dump of topics name
	FILE *dump;//apa added at Enscada.com
	int nTestItem;//apa added at Enscada.com
	////////////////////////////////
	void *parent_class;//apa added at Enscada.com
} MQTTCtx;

void mqtt_init_ctx(MQTTCtx* mqttCtx);
int err_sys(const char* msg);
int mqtt_tls_cb(MqttClient* client);
word16 mqtt_get_packetid(void);
int mqttclient(MQTTCtx *mqttCtx); //apa added at Enscada.com

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* WOLFMQTT_EXAMPLE_H */
