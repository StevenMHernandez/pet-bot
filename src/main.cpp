#include "Arduino.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include "config/ConnectionParams.h"
#include "aws/ESP8266DateTimeProvider.h"
#include <epd.h>

#define ONBOARD_LED 16

#define TYPE_PIN_A 14
#define TYPE_PIN_B 12
#define TYPE_PIN_C 17

#define VARIANT_PIN_A 16
#define VARIANT_PIN_B 5
#define VARIANT_PIN_C 4

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

void test_screen() {
    epd_clear();
    epd_disp_bitmap("DOG1.BMP", 0, 0);
    epd_disp_string("BOW", 360, 250);
    epd_disp_string("WOW", 360, 350);
    epd_disp_string("WOW", 360, 450);
    epd_udpate();

    delay(10000);

    epd_clear();
    epd_disp_bitmap("DOG0.BMP", 0, 0);
    epd_udpate();
}

void setup_screen() {
    epd_set_memory(MEM_TF);
    epd_set_color(BLACK, WHITE);
    epd_screen_rotation(EPD_INVERSION);
    epd_set_en_font(GBK64);
}

void setup_pins() {
    pinMode(TYPE_PIN_A, INPUT_PULLUP);
    pinMode(TYPE_PIN_B, INPUT_PULLUP);
    pinMode(TYPE_PIN_C, INPUT_PULLUP);

    pinMode(VARIANT_PIN_A, INPUT_PULLUP);
    pinMode(VARIANT_PIN_B, INPUT_PULLUP);
    pinMode(VARIANT_PIN_C, INPUT_PULLUP);

    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, LOW);
}

void setup_wifi_mqtt() {
    WiFi.begin(AP_SSID, AP_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    int res = client.connect();
    Serial.printf("mqtt connect=%d\n", res);

    if (res == 0) {
        client.subscribe("calendar", 0, [](const char *topic, const char *msg) {
            epd_clear();
            // epd_disp_bitmap("123.BMP", 0, 0);
            epd_disp_string("ASCII64: Hello, World!", 0, 450);
            epd_udpate();
        });
    }
}

void setup() {
    Serial.begin(115200);

    setup_pins();
    setup_screen();
    setup_wifi_mqtt();

    test_screen();
}

void loop() {
    if (client.isConnected()) {
        client.yield();
    } else {
        Serial.println("Not connected...");
        delay(2000);
    }
}
