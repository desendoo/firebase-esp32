#define FIREBASE_HOST "FIREBASE_HOST"
#define FIREBASE_AUTH "FIREBASE_AUTH"
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

#include <WiFi.h>
#include "FirebaseESP32.h"
#include <Servo_ESP32.h>

#define ldr 36
#define servo 13

Servo_ESP32 servoMotor;
FirebaseData firebaseData;

String path = "/Node-1";

int oldAdcLdr;
int newAdcLdr;
String automationStatus;         

/* ===== ===== ===== ===== ===== ===== ===== */
/* ===== ===== START CONFIG TIMER ===== ===== */
int automationDuration = 2700;
int counter = 1; // Set counter value

// Create a hardware timer
hw_timer_t * timer = NULL;
volatile byte state = LOW;

void IRAM_ATTR onTimer(){
  // Check whether the counter is reaching an endValue (means there is nothing to wait for)
  // endValue = automationDuration;
  if (counter == (automationDuration)) {
    counter = 1; // Reset counter value
  } else if(automationStatus == "on") {
    // Counting in second for 45 minutes
    counter = 1;
    automationDuration = 2700;
  } else if(automationStatus == "off") {
    counter = 1;
    automationDuration = 0;
  } else {
    counter++; // Increment counter value
  }
}
/* ===== ===== END CONFIG TIMER ===== ===== */
/* ===== ===== ===== ===== ===== ===== ===== */

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  pinMode(ldr,INPUT);
  
  servoMotor.attach(servo);
  // Min: 10; Max: 49;
  oldAdcLdr = (0.009768*analogRead(ldr))+10;
  
  initWifi();

  /* ===== ===== ===== ===== ===== ===== ===== */
  /* ===== ===== START CONFIG TIMER ===== ===== */
  // Initilise the timer.
  // Parameter 1 is the timer we want to use. Valid: 0, 1, 2, 3 (total 4 timers)
  // Parameter 2 is the prescaler. The ESP32 default clock is at 80MHz. The value "80" will
  // divide the clock by 80, giving us 1,000,000 ticks per second.
  // Parameter 3 is true means this counter will count up, instead of down (false).
  timer = timerBegin(3, 80, true);

  // Attach the timer to the interrupt service routine named "onTimer".
  // The 3rd parameter is set to "true" to indicate that we want to use the "edge" type (instead of "flat").
  timerAttachInterrupt(timer, &onTimer, true);

  // This is where we indicate the frequency of the interrupts.
  // The value "1000000" (because of the prescaler we set in timerBegin) will produce
  // one interrupt every second.
  // The 3rd parameter is true so that the counter reloads when it fires an interrupt, and so we
  // can get periodic interrupts (instead of a single interrupt).
  timerAlarmWrite(timer, 1000000, true);

  // Start the timer
  timerAlarmEnable(timer);
  /* ===== ===== END CONFIG TIMER ===== ===== */
  /* ===== ===== ===== ===== ===== ===== ===== */
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  Serial.print("Automation Duration and Counter: ");
  Serial.print(automationDuration);
  Serial.print(" | ");
  Serial.println(counter);
  
  // Set value /Node-1/servo
  if(Firebase.getInt(firebaseData, path + "/servo")){
//    Serial.println("PATH: " + firebaseData.dataPath());
//    Serial.println("TYPE: " + firebaseData.dataType());
//    Serial.println("ETag: " + firebaseData.ETag());
//    Serial.print("VALUE: ");
    Serial.print("Data Servo: ");
    Serial.println(firebaseData.intData());
    servoMotor.write(firebaseData.intData());
  }

  // Get the sensor LDR value /Node-1/ldr and send to firebase realtime database
  newAdcLdr = (0.009768*analogRead(ldr))+10;
  Serial.print("Data LDR: ");
  Serial.println(newAdcLdr);
  if(newAdcLdr != oldAdcLdr){
    Firebase.setDouble(firebaseData, path + "/ldr", newAdcLdr);
    oldAdcLdr = newAdcLdr;
  }
  
  // Automation
  if(Firebase.getString(firebaseData, path + "/automation")){
    Serial.print("Automation: ");
    Serial.println(firebaseData.stringData());
    // Check the automation status/value
    // if true, set the automationDuration to 45 minutes 
    if(firebaseData.stringData()=="on") {
      automationStatus = firebaseData.stringData();
      // The automation will be turned on if the LDR sensor
      // receive more light or bathed in sunshine
      if(newAdcLdr >= 15) {
        automationDuration = 2700;
      }
    } else if(firebaseData.stringData()=="off") {
      automationStatus = firebaseData.stringData();
      automationDuration = 0;
    }
  }

  Serial.println("= = = = =");
}

void initWifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  
  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
}
