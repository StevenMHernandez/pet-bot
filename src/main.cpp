#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <epd.h>
#include <aJSON.h>
#include "config.h"
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
            aJsonObject *message = aJson.getObjectItem(desiredObject, "message");

            // Is there a delta? If so, we need to update the display
            // Otherwise, we are fine to sleep
            if (deltaObject != NULL && deltaObject->type != NULL && deltaObject->type == aJson_Object) {
                epd_clear();

                if (message->type == aJson_Array && ((int) aJson.getArraySize(message)) > 0) {
                    renderBitmap(STATUS_HAS_MESSAGE);

                    int totalHeight = 600;
                    int usedHeight = 50 * (int) aJson.getArraySize(message);
                    int paddingTop = 100 + ((totalHeight - usedHeight) / 2);

                    int size = (int) aJson.getArraySize(message);

                    for (int i = 0; i < size; i++) {
                        aJsonObject *m = aJson.getArrayItem(message, (unsigned char) i);
                        epd_disp_string(m->valuestring, 330, paddingTop + (50 * i));
                    }

                    epd_udpate();
                } else {
                    renderBitmap(STATUS_NO_MESSAGE);
                }

                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "message", message);
                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "type", getTypeName());
                update_thing_shadow((char *) SHADOW_STATUS_REPORTED, (char *) "variant", getVariantNumber());

                aJson.deleteItem(jsonObject);
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

    epd_enter_stopmode();
    ESP.deepSleep(1 * 60 * 1000000); // Sleep for 1 minute // TODO: change based on the device shadow
}

void loop() {
    // pass
}
