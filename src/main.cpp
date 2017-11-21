#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <epd.h>
#include <aJSON.h>
#include "config.h"
#include "time.h"
#include "pin_mappings.h"
#include "bitmap_helpers.h"
#include "config/ConnectionParams.h"
#include "aws/ESP8266DateTimeProvider.h"

#define SHADOW_STATUS_REPORTED "reported"
#define SHADOW_STATUS_DESIRED "desired"

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
uint32_t sleepTime = 30; // in minutes

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

    pinMode(VARIANT_PIN_B, INPUT_PULLUP);
    pinMode(VARIANT_PIN_C, INPUT_PULLUP);
}

void setup_screen() {
    epd_set_memory(MEM_TF);
    epd_set_color(BLACK, WHITE);
    epd_screen_rotation(EPD_INVERSION);
    epd_set_en_font(GBK32);
}

void enter_sleep_mode(uint32_t minimum_sleep_time) {
    epd_enter_stopmode();
    if (sleepTime > 0) {
        ESP.deepSleep(sleepTime * SECONDS_PER_MINUTE * MICROSECONDS_PER_SECOND);
    } else {
        ESP.deepSleep(minimum_sleep_time);
    }
}

void update_thing_shadow(char *status, char *key, char *value) {
    aJsonObject *rootObj, *stateObj, *reportedObj;
    rootObj = aJson.createObject();
    aJson.addItemToObject(rootObj, "state", stateObj = aJson.createObject());
    aJson.addItemToObject(stateObj, status, reportedObj = aJson.createObject());
    aJson.addStringToObject(reportedObj, key, value);

    char *json = aJson.print(rootObj);
    client.publish("$aws/things/pet-test/shadow/update", json, 0, false);
    free(json);
}

void update_thing_shadow(char *status, char *key, int value) {
    aJsonObject *rootObj, *stateObj, *reportedObj;
    rootObj = aJson.createObject();
    aJson.addItemToObject(rootObj, "state", stateObj = aJson.createObject());
    aJson.addItemToObject(stateObj, status, reportedObj = aJson.createObject());
    aJson.addNumberToObject(reportedObj, key, value);

    char *json = aJson.print(rootObj);
    client.publish("$aws/things/pet-test/shadow/update", json, 0, false);
    free(json);
}

void update_thing_shadow(char *status, char *key, aJsonObject *value) {
    aJsonObject *rootObj, *stateObj, *reportedObj;
    rootObj = aJson.createObject();
    aJson.addItemToObject(rootObj, "state", stateObj = aJson.createObject());
    aJson.addItemToObject(stateObj, status, reportedObj = aJson.createObject());
    aJson.addItemToObject(reportedObj, key, value);

    char *json = aJson.print(rootObj);
    client.publish("$aws/things/pet-test/shadow/update", json, 0, false);
    free(json);
}

void setup_wifi_mqtt() {
    WiFi.begin(AP_SSID, AP_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    int res = client.connect();

    if (res == 0) {
        update_thing_shadow((char *) SHADOW_STATUS_DESIRED, (char *) "type", getTypeName());
        update_thing_shadow((char *) SHADOW_STATUS_DESIRED, (char *) "variant", getVariantNumber());

        delay(100);

        client.subscribe("$aws/things/pet-test/shadow/get/accepted", 0, [](const char *topic, const char *msg) {
            // parse JSON, if there is a delta, update the screen
            aJsonObject *jsonObject = aJson.parse((char *) msg);
            aJsonObject *stateObject = aJson.getObjectItem(jsonObject, "state");
            aJsonObject *deltaObject = aJson.getObjectItem(stateObject, "delta");
            aJsonObject *desiredObject = aJson.getObjectItem(stateObject, "desired");
            aJsonObject *messageArray = aJson.getObjectItem(desiredObject, "message");
            aJsonObject *sleepTimeObject = aJson.getObjectItem(desiredObject, "sleepTime");
            sleepTime = sleepTimeObject->valueint;

            // Is there a delta? If so, we need to update the display
            // Otherwise, we are fine to sleep
            if (deltaObject != NULL && deltaObject->type != NULL && deltaObject->type == aJson_Object) {
                epd_clear();

                if (messageArray->type == aJson_Array && ((int) aJson.getArraySize(messageArray)) > 0) {
                    renderBitmap(STATUS_HAS_MESSAGE);

                    int totalHeight = 600;
                    int usedHeight = 50 * (int) aJson.getArraySize(messageArray);
                    int paddingTop = 100 + ((totalHeight - usedHeight) / 2);

                    int size = (int) aJson.getArraySize(messageArray);

                    for (int i = 0; i < size; i++) {
                        aJsonObject *m = aJson.getArrayItem(messageArray, (unsigned char) i);
                        epd_disp_string(m->valuestring, 330, paddingTop + (50 * i));
                    }

                    epd_udpate();
                } else {
                    renderBitmap(STATUS_NO_MESSAGE);
                }

                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "message", messageArray);
                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "type", getTypeName());
                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "variant", getVariantNumber());
                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "sleepTime", sleepTime);

                aJson.deleteItem(jsonObject);

                // The display takes about 20 seconds to refresh
                enter_sleep_mode(20 * MICROSECONDS_PER_SECOND);
            }
        });
    }
}

void setup() {
    Serial.begin(115200);

    setup_pins();
    setup_screen();
    setup_wifi_mqtt();

    // Request device shadow
    client.publish("$aws/things/pet-test/shadow/get", "{}", 0, false);

    // Receive device shadow (see lambda above)
    client.yield();

    // No display update, so we restart as soon as possible
    enter_sleep_mode(MICROSECONDS_PER_SECOND);
}

void loop() {
    // pass
}
