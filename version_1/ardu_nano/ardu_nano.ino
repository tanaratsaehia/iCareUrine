#define buzzerPin 12
#define buttonPin 8

bool sleepMode = false;
bool presentMode = false;
bool buzzerAlert = false;
const float waitTimeToSleep = 3.5; // minute
bool wifiState = false;

unsigned long sleepModeMillis;
unsigned long displayMillis;
unsigned long timeOutSerialMillis;
unsigned long presentModeMillis;
unsigned long waitNotiBattLoss;
unsigned long buzAlertMillis;

void setup() {
  // Initialize Serial Communication
  Serial.begin(115200);
  
  // Initialize GPIO Pins
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  offBuzzer();  // Turn off the buzzer initially
  
  // Initialize LCD Display
  lcd_init();
  onBuzzer(0.05);  // Short beep to indicate startup
  
  // Display Welcome Message
  displayWelcomeMessage();

  // Check for Present Mode
  checkForPresentMode();

  // Initialize Load Cell
  initializeLoadCell();

  // Wait for User to Set Zero Point
  waitForZeroPoint();

  // Set Initial Sleep Mode Timer
  sleepModeMillis = millis();
}

void loop() {
  if (millis() - displayMillis >= 1000 & !sleepMode & wifiState){
    displayMillis = millis();
    clear_display();
    display_batt_and_weight();
  }
  sleepModeEvent();
  if (get_batt_percent() > 20){
    waitNotiBattLoss = millis();
  }else if (get_batt_percent() < 20 & millis() - waitNotiBattLoss >= 2000){
    Serial.println("c_batt_loss");
  }
  read_command("one");

  if (buzzerAlert & millis() - buzAlertMillis <= 50000){
    onBuzzer(0.4);
  }else{
    buzzerAlert = false;
  }
  if (buzzerAlert & buttonPressed()){
    buzzerAlert = false;
  }
}

void sleepModeEvent(){
  if (buttonPressed()){
    sleepModeMillis = millis();
  }
  if (millis() - sleepModeMillis >= (waitTimeToSleep*60)*1000 & !sleepMode & !presentMode){
    sleepMode = true;
    lcd_sleep();
    load_cell_sleep();
  }else if (millis() - sleepModeMillis < (waitTimeToSleep*60)*1000 & sleepMode){
    sleepMode = false;
    lcd_wake();
    load_cell_wake();
  }
}

void onBuzzer(float second){
  digitalWrite(buzzerPin, HIGH);
  delay(second*1000);
  digitalWrite(buzzerPin, LOW);
  delay(second*1000);
}

void offBuzzer(){
  digitalWrite(buzzerPin, LOW);
}

bool buttonPressed(){
  return digitalRead(buttonPin);
}

void displayWelcomeMessage() {
  clear_display();
  display_custom("Welcome to", 3, 0);
  display_custom("I Care Urine", 2, 1);
}

void checkForPresentMode() {
  unsigned long presentModeStartTime = millis();
  
  // Wait for Button Press to Enter Present Mode
  while (buttonPressed()) {
    if (millis() - presentModeStartTime >= 5000) {
      enterPresentMode();
      break;
    }
  }
  
  // Indicate Ready State
  Serial.println("hi");
  read_command("inf"); // Check for Wi-Fi Configuration
  // onBuzzer(0.1);
  // onBuzzer(0.1);
  read_command("inf"); // Check for Existing Files
  // onBuzzer(0.1);
  // onBuzzer(0.1);
}

void enterPresentMode() {
  onBuzzer(0.05);
  onBuzzer(0.05);
  presentMode = true;

  // Update Display
  clear_display();
  display_custom("Present Mode", 2, 0);
  display_custom("Ready Now!", 3, 1);

  waitForCommand("present_ok", "c_present_mode");

  // Wait for Button Release
  while (buttonPressed()) {}
}

void initializeLoadCell() {
  load_cell_init();
  delay(500);
} 

void waitForZeroPoint() {
  clear_display();
  display_custom("Press button for", 0, 0);
  display_custom("Set Zero & Start", 0, 1);
  
  // Wait for Button Press to Set Zero
  while (!buttonPressed()) {}
  
  onBuzzer(0.05);
  load_cell_set_zero();
}

void waitForCommand(const String& expectedCommand, const String& statusMessage) {
  while (true) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      if (cmd.startsWith("c_")) {
        cmd = cmd.substring(2);
        cmd.trim();
        if (cmd == expectedCommand) {
          // onBuzzer(0.30);
          break;
        }
      }
    }
    Serial.println(statusMessage);
    delay(1500);
  }
}