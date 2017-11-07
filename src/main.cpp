#include "Arduino.h"
#include "config.h"
#include "pin_mappings.h"
#include "bitmap_helpers.h"
#include <ESP8266WiFi.h>
#include "config/ConnectionParams.h"
#include "aws/ESP8266DateTimeProvider.h"
#include <epd.h>

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

char filename[11];

void renderBitmap(int status) {
    epd_clear();
    buildBitmapFileName(filename, status);
    epd_disp_string(filename, 0, 200);
    // epd_disp_bitmap(filename, 0, 0);
    epd_udpate();
}

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

void setup_pins() {
    pinMode(TYPE_PIN_A, INPUT_PULLUP);
    pinMode(TYPE_PIN_B, INPUT_PULLUP);
    pinMode(TYPE_PIN_C, INPUT_PULLUP);

    pinMode(VARIANT_PIN_A, INPUT_PULLUP);
    pinMode(VARIANT_PIN_B, INPUT_PULLUP);
    pinMode(VARIANT_PIN_C, INPUT_PULLUP);
}

void setup_screen() {
    epd_set_memory(MEM_TF);
    epd_set_color(BLACK, WHITE);
    epd_screen_rotation(EPD_INVERSION);
    epd_set_en_font(GBK64);
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
            renderBitmap(STATUS_HAS_MESSAGE); // TODO: check if there is a message or not
            // epd_disp_bitmap("123.BMP", 0, 0); // TODO: write the msg to the screen as well
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
    // TODO: handle this with an interrupt
    if (toggleChanged()) {
        renderBitmap(STATUS_NO_MESSAGE);
    }

    if (client.isConnected()) {
        client.yield();
    } else {
        Serial.println("Not connected...");
        delay(2000);
    }
}
