#include <NewPing.h>
#include <WiFi.h>
#include "HT_SSD1306Wire.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

HTTPClient http;
HTTPClient http2;
HTTPClient http3;

DynamicJsonDocument doc(2048);

const char* ssid = "DukeVisitor";

// const char* ssid = "notwifi";
// const char* pass = "12345678";

//Defining where the components are attached
#define TRIG_IN 2
#define TRIG_OUT 36
#define ECHO_IN 3
#define ECHO_OUT 34
#define BUTTON 41

SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

#define iterations 5 //Number of readings in the calibration stage
#define MAX_DISTANCE 150 // Maximum distance (in cm) for the sensors to try to read.
#define DEFAULT_DISTANCE 45 // Default distance (in cm) is only used if calibration fails.
#define MIN_DISTANCE 15 // Minimum distance (in cm) for calibrated threshold.

float calibrate_in = 0, calibrate_out = 0; // The calibration in the setup() function will set these to appropriate values.
float distance_in, distance_out; // These are the distances (in cm) that each of the Ultrasonic sensors read.
int count = 0, limit = 5; //Occupancy limit should be set here: e.g. for maximum 8 people in the shop set 'limit = 8'.
bool prev_inblocked = false, prev_outblocked = false; //These booleans record whether the entry/exit was blocked on the previous reading of the sensor.

NewPing sonar[2] = {   // Sensor object array.
  NewPing(TRIG_IN, ECHO_IN, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(TRIG_OUT, ECHO_OUT, MAX_DISTANCE)
};

/*
   A quick note that the sonar.ping_cm() function returns 0 (cm) if the object is out of range / nothing is detected.
   We will include a test to remove these erroneous zero readings later.
*/

bool del = false;

int attendance_count(String s){

  String fnl = "";

  for (int i = 0; i < s.length() ;i++){
    if (s.charAt(i) != '[' && s.charAt(i) != ']'){
      fnl += s.charAt(i);
    }
  }
  fnl.trim();
  return fnl.toInt();
}

void restart(){
  // button pressing interrupt
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50) {
    del = true;
  }
  last_interrupt_time = interrupt_time;

  }

void calibrate(){

  Serial.println("Calibrating...");
  delay(1500);

  for (int a = 0; a < iterations; a++) {
    delay(50);
    calibrate_in += sonar[0].ping_cm();
    delay(50);
    calibrate_out += sonar[1].ping_cm();
    delay(200);
  }
  
  calibrate_in = 0.75 * calibrate_in / iterations; //The threshold is set at 75% of the average of these readings. This should prevent the system counting people if it is knocked.
  calibrate_out = 0.75 * calibrate_out / iterations;

  if (calibrate_in > MAX_DISTANCE || calibrate_in < MIN_DISTANCE) { //If the calibration gave a reading outside of sensible bounds, then the default is used
    calibrate_in = DEFAULT_DISTANCE;
  }
  if (calibrate_out > MAX_DISTANCE || calibrate_out < MIN_DISTANCE) {
    calibrate_out = DEFAULT_DISTANCE;
  }

}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid);

  pinMode(BUTTON, INPUT);

  http.begin("http://3.21.105.25/num");

  http2.begin("http://3.21.105.25/delete");

  http3.begin("http://3.21.105.25/update_count");

  display.init();

  attachInterrupt(digitalPinToInterrupt(BUTTON), &restart, RISING);

  calibrate();

  // calibration
  Serial.print("Entry threshold set to: ");
  Serial.println(calibrate_in);
  Serial.print("Exit threshold set to: ");
  Serial.println(calibrate_out);
  delay(1000);

}

int cur_count;

void push_count(int current_count){
    // pushing the count of the ultrasonic to the database
    doc["count"] = count;

    String json;
    serializeJson(doc, json);

    http3.addHeader("Content-Type", "application/json");
    http3.POST(json);

}

void loop() {

  if (del == true){
    // Button Pushed
    
    count = 0;
    display.clear();

    display.drawString(0, 0, "Restarting");
    display.display();
    Serial.println("Button Pressed. Wait 10 seconds.");

    http2.POST("2");
    
    String l = http2.getString();
    Serial.println(l);

    calibrate();

    Serial.print("Recalibrated. Count reset to: ");
    Serial.println(count);

    del = false;
    delay(100);

  }
  else{
    distance_in = sonar[0].ping_cm();
    delay(40); // Wait 40 milliseconds between pings. 29ms should be the shortest delay between pings.
    distance_out = sonar[1].ping_cm();
    delay(40);
    if (distance_in < calibrate_in && distance_in > 0) { // If closer than wall/calibrated object (person is present) && throw out zero readings
      if (prev_inblocked == false) {
        // Someone is entering, block all sensors
        count++; // Increase count by one
        // too high
        if (count > 50){
            count = 0;
        }

        Serial.print("Count: ");
        Serial.println(count);

        int val = http.GET();
        String res = http.getString();
        cur_count = attendance_count(res);

        Serial.print("Val: ");
        Serial.println(cur_count);

        display.clear();
        display.drawString(0, 0, "Count: " + String(count));
        display.display();

        push_count(count);

      }
      prev_inblocked = true;
    } else {
      prev_inblocked = false;
    }

    if (distance_out < calibrate_out && distance_out > 0) {
      if (prev_outblocked == false) {
        // Someone is exiting, block all sensors
        count--; // Decrease count by one
        // too low
        if (count < 0){
            count = 0;
        }
        Serial.print("Count: ");
        Serial.println(count);

        int val = http.GET();
        String res = http.getString();

        cur_count = attendance_count(res);
        Serial.print("Val: ");
        Serial.println(cur_count);

        display.clear();
        display.drawString(0, 0, "Count: " + String(count));
        display.display();

        push_count(count);

      }
      prev_outblocked = true;
    } else {
      prev_outblocked = false;
    }

  }
}