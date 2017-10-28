#include "Arduino.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include "config/ConnectionParams.h"
#include "aws/ESP8266DateTimeProvider.h"

const char *AP_SSID = SSID_NAME;
const char *AP_PASS = SSID_PASS;

char *region = (char *) MY_AWS_REGION;
char *endpoint = (char *) MY_AWS_IOT_ENDPOINT;
char *mqttHost = (char *) MY_AWS_IOT_MQTT_HOST;
int mqttPort = MY_AWS_IOT_MQTT_PORT;
char *iamKeyId = (char *) MY_AWS_IAM_KEY_ID;
char *iamSecretKey = (char *) MY_AWS_IAM_SECRET_KEY;

ESP8266DateTimeProvider dtp;
AwsIotSigv4 sigv4(&dtp, region, endpoint, mqttHost, mqttPort, iamKeyId, iamSecretKey);
ConnectionParams cp(sigv4);
WebSocketClientAdapter adapter(cp);
MqttClient client(adapter, cp);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        yield();
    }

    pinMode(16, OUTPUT);
    digitalWrite(16, LOW);

    WiFi.begin(AP_SSID, AP_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    int res = client.connect();
    Serial.printf("mqtt connect=%d\n", res);

    if (res == 0) {
        client.subscribe("t1", 1, [](const char *topic, const char *msg) {
            Serial.printf("Got msg '%s' on topic %s\n", msg, topic);

            digitalWrite(16, HIGH);
        });

        client.subscribe("t2", 0, [](const char *topic, const char *msg) {
            Serial.printf("Got msg '%s' on topic %s\n", msg, topic);

            digitalWrite(16, LOW);
        });
    }
}

void loop() {
    if (client.isConnected()) {
        client.yield();
    } else {
        Serial.println("Not connected...");
        delay(2000);
    }
}
