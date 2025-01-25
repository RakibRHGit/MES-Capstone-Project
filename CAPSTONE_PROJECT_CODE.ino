#include <object_detection_v2_inferencing.h>

//#include <object_detection_inferencing.h>

/**
 * Run Edge Impulse FOMO model.
 * It works on both PSRAM and non-PSRAM boards.
 * 
 * The difference from the PSRAM version
 * is that this sketch only runs on 96x96 frames,
 * while PSRAM version runs on higher resolutions too.
 * 
 * The PSRAM version can be found in my
 * "ESP32S3 Camera Mastery" course
 * at https://dub.sh/ufsDj93
 *
 * BE SURE TO SET "TOOLS > CORE DEBUG LEVEL = INFO"
 * to turn on debug messages
 */

#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/edgeimpulse/fomo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_sleep.h>

// ESP32-CAM doesn't have dedicated i2c pins, so we define our own. Let's choose 15 and 14
#define I2C_SDA 15
#define I2C_SCL 14
TwoWire I2Cbus = TwoWire(0);

// Display defines
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cbus, OLED_RESET);

// Ultrasonic sensor pins
#define TRIG_PIN 2
#define ECHO_PIN 13

// Other variables
int tcount = 0;
float previousDistance = 0.0;

using eloq::camera;
using eloq::ei::fomo;





void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("__EDGE IMPULSE FOMO (NO-PSRAM)__");

    // Camera settings
    camera.pinout.aithinker();
    camera.brownout.disable();
    camera.resolution.yolo();
    camera.pixformat.rgb565();

    // Initialize camera
    while (!camera.begin().isOk())
        Serial.println(camera.exception.toString());

    Serial.println("Camera OK");
    Serial.println("Put object in front of camera");

    // Initialize I2C for display
    I2Cbus.begin(I2C_SDA, I2C_SCL, 100000);
    Serial.println("Initialize display");

    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.printf("SSD1306 OLED display failed to initialize.\nCheck that display SDA is connected to pin %d and SCL connected to pin %d\n", I2C_SDA, I2C_SCL);
        while (true);
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();

    // Initialize ultrasonic sensor
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    
}

float getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = (duration * 0.0343) / 2;
    return distance;
}

void enterDeepSleep() {
    Serial.println("Entering deep sleep for 10 seconds...");

    display.clearDisplay();
     display.setCursor(0, 0);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.print("Entering  Deep sleep ...");
        display.display();

        delay(500);
      

    esp_sleep_enable_timer_wakeup(10 * 1000000); //initializing sleep time
    esp_deep_sleep_start(); //entering deep sleep
}

void loop() {
    // Capture picture
    display.clearDisplay();


    float currentDistance = getDistance();

    // Check the difference between current and previous distance
    if (abs(currentDistance - previousDistance) < 3) {
        tcount++;
    } else {
        tcount = tcount - 5;

    }
    
    Serial.print("Previous = ");
    Serial.print(previousDistance);
    Serial.print(" cm and Current = ");
    Serial.print(currentDistance);
    Serial.print(" cm ");

    // Update previous distance
    previousDistance = currentDistance;
    

    // Check if tcount reaches 10
    if (tcount == 10) {
        enterDeepSleep();
    }





    if (!camera.capture().isOk()) {
        Serial.println(camera.exception.toString());
        return;
    }

    // Run FOMO
    if (!fomo.run().isOk()) {
        Serial.println(fomo.exception.toString());
        return;
    }

    // How many objects were found?
    Serial.printf("Found %d object(s) in %dms\n", fomo.count(), fomo.benchmark.millis());

    // If no object is detected, display message and return
    if (!fomo.foundAnyObject()) {
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.print("No object found");
        display.display();
        return;
    }

    // If you expect to find a single object, use fomo.first
    Serial.printf("Found %s at (x = %d, y = %d) (size %d x %d). Proba is %.2f\n", fomo.first.label, fomo.first.x, fomo.first.y, fomo.first.width, fomo.first.height, fomo.first.proba);

    display.setCursor(0, 0);
    display.setTextSize(1.5);
    display.setTextColor(SSD1306_WHITE);
    display.print(fomo.first.label);
    display.print("(");
    display.print( fomo.first.proba);
    display.print(")");
    display.display();

    // If you expect to find many objects, use fomo.forEach
    if (fomo.count() > 1) {
        fomo.forEach([](int i, bbox_t bbox) {
            Serial.printf("#%d) Found %s at (x = %d, y = %d) (size %d x %d). Proba is %.2f\n", i + 1, bbox.label, bbox.x, bbox.y, bbox.width, bbox.height, bbox.proba);
            display.setCursor(0, 16 * (i + 1));
            display.setTextSize(1.5);
            display.setTextColor(SSD1306_WHITE);
            display.print(fomo.first.label);
            display.print("(");
            display.print( fomo.first.proba);
            display.print(")");
            display.display();

        });
    }

    // Get current distance from ultrasonic sensor
    

    // Add a 1-second delay
    delay(1000);
}
