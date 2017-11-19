'use strict';

const AWS = require('aws-sdk');
const LineBreaker = require('linebreak');
const pixelWidth = require("string-pixel-width");

AWS.config.region = process.env.IOT_AWS_REGION;
const iotData = new AWS.IotData({endpoint: process.env.IOT_ENDPOINT_HOST});

module.exports.petBotCalendarEvent = (event, context, callback) => {
    let message = splitMessage(event.body);

    let params = {
        topic: '$aws/things/pet-test/shadow/update',
        payload: JSON.stringify({
            "state": {
                "desired": {
                    "message": message,
                }
            }
        }),
        qos: 0
    };

    // Broadcast to IoT
    iotData.publish(params, function (err, data) {
        if (err) {
            callback(null, {
                "isBase64Encoded": false,
                "statusCode": 500,
                "headers": {},
                "body": "not ok"
            });
        } else {
            callback(null, {
                "isBase64Encoded": false,
                "statusCode": 200,
                "headers": {},
                "body": "ok",
            });
        }
    });
};

var splitMessage = function (message) {
    // This string was shown to fit well within the screen constraints
    const maxwidth = Math.ceil(pixelWidth('teaha3sd5hst35', {size: 10}));

    var lorem = message;
    var breaker = new LineBreaker(lorem);
    var last = 0;
    var bk;
    var final = [];

    while (lorem != "") {
        var selected = "";
        while (bk = breaker.nextBreak()) {
            let word = lorem.slice(last, bk.position);
            if (pixelWidth(word, {size: 10}) <= maxwidth) {
                selected = word;
            }
        }

        lorem = lorem.replace(selected, "");
        breaker = new LineBreaker(lorem);

        final.push(selected);
    }

    return final;
};
