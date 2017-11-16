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
    epd_disp_string("loading...", 240, 750);
    epd_udpate();

    epd_clear();
    buildBitmapFileName(filename, status);
    epd_disp_bitmap(filename, 0, 0);

    if (status == STATUS_NO_MESSAGE) {
        epd_udpate();
    }
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
    epd_set_en_font(GBK32);
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
            // TODO: display message here
            epd_disp_string("teaha3sd5hst35", 330, 100);
            epd_disp_string("teaha3sd5hst35", 330, 150);
            epd_disp_string("teaha3sd5hst35", 330, 200);
            epd_disp_string("teaha3sd5hst35", 330, 250);
            epd_disp_string("teaha3sd5hst35", 330, 300);
            epd_disp_string("teaha3sd5hst35", 330, 350);
            epd_disp_string("teaha3sd5hst35", 330, 400);
            epd_disp_string("teaha3sd5hst35", 330, 450);
            epd_disp_string("teaha3sd5hst35", 330, 500);
            epd_disp_string("teaha3sd5hst35", 330, 550);
            epd_disp_string("teaha3sd5hst35", 330, 600);
            epd_disp_string("teaha3sd5hst35", 330, 650);
            epd_disp_string("teaha3sd5hst35", 330, 700);
            epd_udpate();
        });
    }
}

void setup() {
    Serial.begin(115200);

    setup_pins();
    setup_screen();
    setup_wifi_mqtt();
}

void loop() {
    // TODO: handle this with an interrupt
    if (toggleChanged()) {
        // This delay accounts for moving from the top position to last position without stopping in the middle
        // Otherwise, we would `renderBitmap` then in the next iteration of `loop`, we would notice another
        // `toggleChanged` and then `renderBitmap` a second time.
        delay(500);
        renderBitmap(STATUS_NO_MESSAGE);
    }

    if (client.isConnected()) {
        client.yield();
    } else {
        Serial.println("Not connected...");
        delay(2000);
    }

    epd_enter_stopmode();
    ESP.deepSleep(1 * 60 * 1000000); // Sleep for 1 minute
}
