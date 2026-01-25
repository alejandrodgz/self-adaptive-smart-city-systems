/// Maqueta con ESP32
#define LDR1 13 // LDR Light sensor from traffic light 1 connected in pin A0
#define LDR2 12 // LDR Light sensor from traffic light 2 connected in pin A1
#define CO2 14  // CO2 sensor connected in pin A3
#define P1 1    // Traffic light 1 button connected in pin 1
#define P2 2    // Traffic light 2 button connected in pin 2
#define CNY1 42 // Infrared sensor 1 in traffic light 1 connected in pin 42
#define CNY2 41 // Infrared sensor 2 in traffic light 1 connected in pin 41
#define CNY3 40 // Infrared sensor 3 in traffic light 1 connected in pin 40
#define CNY4 39 // Infrared sensor 4 in traffic light 2 connected in pin 39
#define CNY5 38 // Infrared sensor 5 in traffic light 2 connected in pin 38
#define CNY6 37 // Infrared sensor 6 in traffic light 2 connected in pin 37
#define LR1 5   // Red traffic light 1 connected in pin 5
#define LY1 4   // Yellow traffic light 1 connected in pin 4
#define LG1 6   // Green traffic light 1 connected in pin 6
#define LR2 7   // Red traffic light 2 connected in pin 7
#define LY2 15  // Yellow traffic light 2 connected in pin 15
#define LG2 16  // Green traffic light 2 connected in pin 16
 
void setup() {
  // put your setup code here, to run once:
  pinMode(LR1, OUTPUT);
  pinMode(LY1, OUTPUT);
  pinMode(LG1, OUTPUT);
  pinMode(LR2, OUTPUT);
  digitalWrite(LR1, 0);
  digitalWrite(LY1, 0);
  digitalWrite(LG1, 0);
  digitalWrite(LR2, 0);
}
 
void loop() {
  // Blink all 4 LEDs in sequence
  digitalWrite(LR1, 1);
  delay(500);
  digitalWrite(LR1, 0);
  
  digitalWrite(LY1, 1);
  delay(500);
  digitalWrite(LY1, 0);
  
  digitalWrite(LG1, 1);
  delay(500);
  digitalWrite(LG1, 0);
  
  digitalWrite(LR2, 1);
  delay(500);
  digitalWrite(LR2, 0);
}
