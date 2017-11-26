/************************** Configuration ***********************************/

// Edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"
#include <SimpleTimer.h>

#include <Servo.h>

/************************ Example Starts Here *******************************/

#define SERVO_PIN 2
#define SENSOR_PIN 5

// create an instance of the servo class
Servo servo;

SimpleTimer timer;

int motionState = 0;
int isTreatLocked = 0;
int treatLockoutTime = 7200000;
int treatTimer;

AdafruitIO_Feed *motionFeed = io.feed("treat-feeder.motion-feed");
AdafruitIO_Feed *treatFeed = io.feed("treat-feeder.treat-feed");
AdafruitIO_Feed *manualTreatFeed = io.feed("treat-feeder.manual-dispense-feed");
AdafruitIO_Feed *lockoutFeed = io.feed("treat-feeder.lockout-feed");
AdafruitIO_Feed *consoleFeed = io.feed("treat-feeder.console-feed");

void doDispense(AdafruitIO_Data *data) {
  servo.write(10);
  delay(1000);
  servo.write(50);
  
  consoleFeed->save("Treat dispensed!");
  treatFeed->save(1);
}

void dispenseTreat() {
  if(!isTreatLocked) {
    timer.restartTimer(treatTimer);

    isTreatLocked = 1;
    doDispense(NULL);
  } else {
    consoleFeed->save("Treat not dispensed due to lockout.");
  }
}

void setTreatLockout(AdafruitIO_Data *data) {
  int lockoutMillis = atoi(data->value()) * 1000;
  treatTimer = timer.setInterval(treatLockoutTime, removeTreatLockout);
}

void removeTreatLockout() {
  if(isTreatLocked) {
    isTreatLocked = 0;
    consoleFeed->save("Removing lockout.");
  }
}

void updateTimerResetRate(AdafruitIO_Data *data) {
  treatLockoutTime = atoi(data->value()) * 1000;
  consoleFeed->save("Updated Treat Lockout Time");
  consoleFeed->save(treatLockoutTime);

  timer.deleteTimer(treatTimer);
  treatTimer = timer.setInterval(treatLockoutTime, removeTreatLockout);
}

void readMotionDetector() {
  // grab the current state of the photocell
  int currentRead = digitalRead(SENSOR_PIN);

  if (currentRead == HIGH) {
    if(motionState == LOW) {
      motionState = currentRead;
      dispenseTreat();

      motionFeed->save(1);
      consoleFeed->save("Motion detected!");
    }
  } else {
    if(motionState == HIGH) {
        motionState = currentRead;

        motionFeed->save(0);
        consoleFeed->save("Motion ended!");
    }
  }
}

void setup() {

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while (!Serial);

  // tell the servo class which pin we are using
  servo.attach(SERVO_PIN);
  servo.write(50);

  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  manualTreatFeed->onMessage(doDispense);
  lockoutFeed->onMessage(updateTimerResetRate);
}

void loop() {
  io.run();
  timer.run();

  readMotionDetector();
  delay(1000); // 1 second delay
}
