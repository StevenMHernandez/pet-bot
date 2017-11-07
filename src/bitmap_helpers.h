#include "Arduino.h"
#include "pin_mappings.h"

#ifndef PT_BITMAP_HELPERS_H
#define PT_BITMAP_HELPERS_H

#define STATUS_NO_MESSAGE 0
#define STATUS_HAS_MESSAGE 1

byte type = 0b000;
byte variant = 0b000;

byte loadCurrentType() {
    byte _type = 0;

    if (!digitalRead(TYPE_PIN_A)) {
        _type = 0b001;
    } else if (!digitalRead(TYPE_PIN_B)) {
        _type = 0b010;
    } else {
        _type = 0b100;
    }

    return _type;
}

byte loadCurrentVariant() {
    byte _variant = 0;

    if (!digitalRead(VARIANT_PIN_B)) {
        _variant = 0b010;
    } else if (!digitalRead(VARIANT_PIN_C)) {
        _variant = 0b100;
    } else {
        _variant = 0b001;
    }

    return _variant;
}

char *getTypeName() {
    type = loadCurrentType();

    if (!(type ^ 0b001)) {
        return "DOG";
    } else if (!(type ^ 0b010)) {
        return "CAT";
    } else {
        return "RAN";
    }
}

int getVariantNumber() {
    variant = loadCurrentVariant();

    if (!(variant ^ 0b001)) {
        return 0;
    } else if (!(variant ^ 0b010)) {
        return 1;
    } else {
        return 2;
    }
}

bool toggleChanged() {
    return (type ^ loadCurrentType() || variant ^ loadCurrentVariant());
}

void buildBitmapFileName(char *filename, int status) {
    sprintf(filename, "%s%d%d.BMP", getTypeName(), getVariantNumber(), status);
}

#endif //PT_BITMAP_HELPERS_H
