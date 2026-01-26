// =================================================================
//  Smart City Traffic System - Generation 1 (Low Level)
//  Project: Self-Adaptive Systems
// =================================================================
//  This code implements a context-aware traffic control system
//  with four predefined operation modes. It adjusts its behavior
//  based on environmental and traffic conditions but does not learn.

// =================================================================

// -- Pin Definitions --
// --- Microcontroller ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- Traffic Light 1 ---
#define LR1 5   // Red
#define LY1 4   // Yellow
#define LG1 6   // Green

// --- Traffic Light 2 ---
#define LR2 7   // Red
#define LY2 15  // Yellow
#define LG2 16  // Green

// --- Pedestrian Buttons ---
#define P1 2    // Button for Traffic Light 1
#define P2 1    // Button for Traffic Light 2 (Note: P1/P2 pins swapped from initial description to match diagram)

// --- Vehicle Sensors (Infrared) ---
#define CNY1 42 // Sensor 1, Direction 1
#define CNY2 41 // Sensor 2, Direction 1
#define CNY3 40 // Sensor 3, Direction 1
#define CNY4 39 // Sensor 4, Direction 2
#define CNY5 38 // Sensor 5, Direction 2
#define CNY6 37 // Sensor 6, Direction 2

// --- Environmental Sensors ---
#define LDR1 13 // Light Sensor 1
#define LDR2 12 // Light Sensor 2
#define CO2_PIN 14 // CO2 Sensor

// -- System Configuration --
#define NIGHT_MODE_THRESHOLD 1000 // LDR value to trigger night mode (adjust based on sensor readings)
#define HEAVY_TRAFFIC_DIFF 3      // Vehicle count difference to trigger heavy traffic mode
#define PEDESTRIAN_CROSS_TIME 15000 // 15 seconds for pedestrians to cross

// -- LCD Display --
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C address 0x27, 20 column and 4 rows

// -- Global State Variables --
enum State { NORMAL, NIGHT, HEAVY_TRAFFIC, PEDESTRIAN };
State currentState = NORMAL;

int vehicleCount1 = 0;
int vehicleCount2 = 0;
int ldr1Value = 0;
int ldr2Value = 0;
int co2Value = 0;

// =================================================================
//  SETUP: Runs once at the beginning
// =================================================================
void setup() {
  Serial.begin(115200);

  // --- Initialize LCD ---
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Initializing");
  delay(1000);
  lcd.clear();

  // --- Initialize Traffic Lights (Outputs) ---
  pinMode(LR1, OUTPUT);
  pinMode(LY1, OUTPUT);
  pinMode(LG1, OUTPUT);
  pinMode(LR2, OUTPUT);
  pinMode(LY2, OUTPUT);
  pinMode(LG2, OUTPUT);

  // --- Initialize Sensors (Inputs) ---
  pinMode(P1, INPUT_PULLUP);
  pinMode(P2, INPUT_PULLUP);
  pinMode(CNY1, INPUT);
  pinMode(CNY2, INPUT);
  pinMode(CNY3, INPUT);
  pinMode(CNY4, INPUT);
  pinMode(CNY5, INPUT);
  pinMode(CNY6, INPUT);
  // Analog pins (LDR, CO2) don't need pinMode

  // --- Attach Interrupts for Pedestrian Buttons ---
  attachInterrupt(digitalPinToInterrupt(P1), handlePedestrianRequest, FALLING);
  attachInterrupt(digitalPinToInterrupt(P2), handlePedestrianRequest, FALLING);
  
  // Start with all lights red
  allLightsRed();
}

// =================================================================
//  MAIN LOOP: Runs continuously
// =================================================================
void loop() {
  // 1. Read all sensors to get current context
  readAllSensors();

  // 2. Determine the current state based on context
  // (Pedestrian state is handled by interrupts)
  if (currentState != PEDESTRIAN) {
    determineState();
  }

  // 3. Execute the logic for the current state
  switch (currentState) {
    case NORMAL:
      runNormalMode();
      break;
    case NIGHT:
      runNightMode();
      break;
    case HEAVY_TRAFFIC:
      runHeavyTrafficMode();
      break;
    case PEDESTRIAN:
      // This state is triggered by an interrupt
      // The interrupt handler will set the state back to NORMAL when done
      break;
  }
  
  // 4. Update the display with current info
  updateDisplay();
}

// =================================================================
//  State Determination and Sensor Reading
// =================================================================

void readAllSensors() {
  // Read LDR values
  ldr1Value = analogRead(LDR1);
  ldr2Value = analogRead(LDR2);

  // Read CO2 sensor
  co2Value = analogRead(CO2_PIN);

  // Read vehicle sensors (simple digital read for now)
  // A more robust implementation would count vehicles over time
  vehicleCount1 = !digitalRead(CNY1) + !digitalRead(CNY2) + !digitalRead(CNY3);
  vehicleCount2 = !digitalRead(CNY4) + !digitalRead(CNY5) + !digitalRead(CNY6);
  
  Serial.print("V1: "); Serial.print(vehicleCount1);
  Serial.print(" | V2: "); Serial.print(vehicleCount2);
  Serial.print(" | LDR1: "); Serial.print(ldr1Value);
  Serial.print(" | LDR2: "); Serial.println(ldr2Value);
}

void determineState() {
  // Check for Night Mode first
  if (ldr1Value < NIGHT_MODE_THRESHOLD && ldr2Value < NIGHT_MODE_THRESHOLD) {
    currentState = NIGHT;
  } 
  // Check for Heavy Traffic
  else if (abs(vehicleCount1 - vehicleCount2) >= HEAVY_TRAFFIC_DIFF) {
    currentState = HEAVY_TRAFFIC;
  } 
  // Default to Normal Mode
  else {
    currentState = NORMAL;
  }
}

void updateDisplay() {
  lcd.clear();
  
  // Line 1: Current Mode
  lcd.setCursor(0, 0);
  switch(currentState) {
    case NORMAL: lcd.print("Mode: NORMAL"); break;
    case NIGHT: lcd.print("Mode: NOCTURNO"); break;
    case HEAVY_TRAFFIC: lcd.print("Mode: T. PESADO"); break;
    case PEDESTRIAN: lcd.print("Mode: PEATONAL"); break;
  }

  // Line 2 & 3: Vehicle and Environment Data
  lcd.setCursor(0, 1);
  lcd.print("Vehiculos: D1=");
  lcd.print(vehicleCount1);
  lcd.print(" D2=");
  lcd.print(vehicleCount2);
  
  lcd.setCursor(0, 2);
  lcd.print("Luz: ");
  lcd.print((ldr1Value + ldr2Value) / 2);
  lcd.setCursor(0, 3);
  lcd.print("CO2: ");
  lcd.print(co2Value);
}


// =================================================================
//  Interrupt Handlers
// =================================================================

void handlePedestrianRequest() {
  // Set the state to PEDESTRIAN to interrupt the main loop's flow
  if (currentState != PEDESTRIAN) {
    currentState = PEDESTRIAN;
    runPedestrianMode(); // Execute directly for immediate response
  }
}

// =================================================================
//  Operation Modes
// =================================================================

void runNormalMode() {
  // Standard cycle: 10s Green, 3s Yellow
  runTrafficCycle(10000, 3000);
}

void runNightMode() {
  // Faster cycle: 6s Green, 2s Yellow
  runTrafficCycle(6000, 2000);
}

void runHeavyTrafficMode() {
  // Prioritize the direction with more vehicles
  if (vehicleCount1 > vehicleCount2) {
    // Longer green for direction 1
    runSingleCycle(1, 15000, 3000); // 15s green for TL1
    runSingleCycle(2, 5000, 3000);  // 5s green for TL2
  } else {
    // Longer green for direction 2
    runSingleCycle(1, 5000, 3000);  // 5s green for TL1
    runSingleCycle(2, 15000, 3000); // 15s green for TL2
  }
}

void runPedestrianMode() {
  // 1. Turn all traffic lights red
  allLightsRed();
  updateDisplay(); // Show "PEDESTRIAN" mode on LCD

  // 2. Wait for pedestrian crossing time
  delay(PEDESTRIAN_CROSS_TIME);

  // 3. Flash yellow lights to signal return to normal
  flashAllYellow(3); // Flash 3 times

  // 4. Return to NORMAL state
  currentState = NORMAL;
}

// =================================================================
//  Traffic Light Control Helpers
// =================================================================

// Runs a full cycle for both traffic lights
void runTrafficCycle(int greenTime, int yellowTime) {
  runSingleCycle(1, greenTime, yellowTime);
  runSingleCycle(2, greenTime, yellowTime);
}

// Runs one phase of a traffic light cycle (Green -> Yellow -> Red)
void runSingleCycle(int direction, int greenTime, int yellowTime) {
  if (direction == 1) {
    // TL1 Green, TL2 Red
    digitalWrite(LG1, HIGH);
    digitalWrite(LY1, LOW);
    digitalWrite(LR1, LOW);
    digitalWrite(LR2, HIGH);
    digitalWrite(LY2, LOW);
    digitalWrite(LG2, LOW);
    if (interruptSafeDelay(greenTime)) return; // Check for interrupt

    // TL1 Yellow, TL2 Red
    digitalWrite(LG1, LOW);
    digitalWrite(LY1, HIGH);
    if (interruptSafeDelay(yellowTime)) return; // Check for interrupt

  } else { // direction == 2
    // TL2 Green, TL1 Red
    digitalWrite(LR1, HIGH);
    digitalWrite(LY1, LOW);
    digitalWrite(LG1, LOW);
    digitalWrite(LG2, HIGH);
    digitalWrite(LY2, LOW);
    digitalWrite(LR2, LOW);
    if (interruptSafeDelay(greenTime)) return; // Check for interrupt

    // TL2 Yellow, TL1 Red
    digitalWrite(LG2, LOW);
    digitalWrite(LY2, HIGH);
    if (interruptSafeDelay(yellowTime)) return; // Check for interrupt
  }
  
  allLightsRed();
  delay(1000); // All-red delay for safety
}

void allLightsRed() {
  digitalWrite(LG1, LOW);
  digitalWrite(LY1, LOW);
  digitalWrite(LR1, HIGH);
  digitalWrite(LG2, LOW);
  digitalWrite(LY2, LOW);
  digitalWrite(LR2, HIGH);
}

void flashAllYellow(int count) {
  allLightsOff();
  for (int i = 0; i < count; i++) {
    digitalWrite(LY1, HIGH);
    digitalWrite(LY2, HIGH);
    delay(500);
    digitalWrite(LY1, LOW);
    digitalWrite(LY2, LOW);
    delay(500);
  }
}

void allLightsOff() {
    digitalWrite(LR1, LOW);
    digitalWrite(LY1, LOW);
    digitalWrite(LG1, LOW);
    digitalWrite(LR2, LOW);
    digitalWrite(LY2, LOW);
    digitalWrite(LG2, LOW);
}

// Custom delay function that checks for pedestrian interrupt
// Returns true if an interrupt occurred, false otherwise
bool interruptSafeDelay(int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (currentState == PEDESTRIAN) {
      return true; // Abort delay if pedestrian button was pressed
    }
    delay(10); // Small delay to prevent busy-waiting
  }
  return false;
}
