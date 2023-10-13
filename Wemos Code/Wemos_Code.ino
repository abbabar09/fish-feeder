
#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>
Servo servo;

#define FIREBASE_HOST "fishfeeder-8e3e2-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "J3KtuvYvQaRMIRF0y71HxpDXHsV3TJqkvKywblhkx"
#define WIFI_SSID "LYKA ANN"
#define WIFI_PASSWORD "04241903"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);

FirebaseData fbdo;

FirebaseData timer, feed, pH;
String stimer;
String Str[] = { "00:00", "00:00", "00:00" };
int i, feednow = 0;

//pH Sensor Codes
float calibration = 31.88; //change to calibrate
const int analogInPin = A0;
unsigned long int avgValue;
float b;
int buf[10], temp;

void setup() {
    Serial.begin(9600);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to a");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("connected: ");
    Serial.println(WiFi.localIP());

    timeClient.begin();
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
    servo.attach(D3); // Pin attached to D3

}

void loop() {
    Firebase.getInt(feed, "feednow");
    feednow = feed.to<int>();
    Serial.println(feednow);
    if (feednow == 1) // Direct Feeding
    {
        servo.writeMicroseconds(1000); // rotate clockwise
        delay(700); // allow to rotate for n micro seconds, you can change this to your need
        servo.writeMicroseconds(1500); // stop rotation
        feednow = 0;
        Firebase.setInt(feed, "/feednow", feednow);
        Serial.println("Fed");
    }
    else { // Scheduling feed
        for (i = 0; i < 3; i++) {
            String path = "timers/timer" + String(i);
            Firebase.getString(timer, path);
            stimer = timer.to<String>();
            Str[i] = stimer.substring(9, 14);
        }
        timeClient.update();
        String currentTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
        if (Str[0] == currentTime || Str[1] == currentTime || Str[2] == currentTime)
        {
            servo.writeMicroseconds(1000); // rotate clockwise
            delay(700); // allow to rotate for n micro seconds, you can change this to your need
            servo.writeMicroseconds(1500); // stop rotation
            Serial.println("Success");
            delay(6000);
        }
    }
    Str[0] = "00:00";
    Str[1] = "00:00";
    Str[2] = "00:00";


    {

        for (int i = 0; i < 10; i++)
        {
            buf[i] = analogRead(analogInPin);
            delay(30);
        }
        for (int i = 0; i < 9; i++)
        {
            for (int j = i + 1; j < 10; j++)
            {
                if (buf[i] > buf[j])
                {
                    temp = buf[i];
                    buf[i] = buf[j];
                    buf[j] = temp;
                }
            }
        }
        avgValue = 0;
        for (int i = 2; i < 8; i++)
            avgValue += buf[i];
        float pHVol = (float)avgValue * 5.0 / 1024 / 6;
        float phValue = -5.70 * pHVol + calibration;
        Serial.println(phValue);

        Firebase.RTDB.setFloat(&fbdo, "sensor", phValue);
        float total = (phValue);
        if (total < 4) {
            Firebase.RTDB.setString(&fbdo, "result", " Very Acidic");
        }
        else if (total >= 4 && total < 5) {
            Firebase.RTDB.setString(&fbdo, "result", "Acidic");
        }
        else if (total >= 5 && total < 7) {
            Firebase.RTDB.setString(&fbdo, "result", "Acidic-ish");
        }
        else if (total >= 7 && total < 8) {
            Firebase.RTDB.setString(&fbdo, "result", "Neutral");
        }
        else if (total >= 8 && total < 10) {
            Firebase.RTDB.setString(&fbdo, "result", "Alkaline-ish");
        }
        else if (total >= 10 && total < 11) {
            Firebase.RTDB.setString(&fbdo, "result", "Alkaline");
        }
        else if (total >= 11) {
            Firebase.RTDB.setString(&fbdo, "result", "Very Alkaline");
        }
    }
}
