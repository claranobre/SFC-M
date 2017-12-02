/*  Project Base:
 *   Pulse Sensor Amped 1.5    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

----------------------  Notes ----------------------  ----------------------
This code:
1) Blinks an LED to User's Live Heartbeat   PIN 13
2) Fades an LED to User's Live HeartBeat    PIN 5
3) Determines BPM
4) Prints All of the Above to Serial

Read Me:
https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
 ----------------------       ----------------------  ----------------------
*/

/*
input cases:
Range: 5/2.5
Working: 4/3
Low Pressure: 0.75/1.3
Normal Press: 0.88/1.95 (11.8/8)
High Pressu1: 0.75/2 (11.8/8.55)
High Pressu2: 1/2 (12.4/8)
Very High Pr: 1/2.5 (15/10)
*/

#define PROCESSING_VISUALIZER 1
#define SERIAL_PLOTTER  2

//  Variables
const int numPulseSensors = 2;
const int threshold = 530;
int pulsePin[numPulseSensors];                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin[numPulseSensors];                // pin to blink led at each beat
int fadePin[numPulseSensors];                  // pin to do fancy classy fading blink at each beat
int fadeRate[numPulseSensors];                 // used to fade LED on with PWM on fadePin

// LED's
// red = blinkPin
int yellow = 12;
int green = 11;
int led_on;

// Volatile Variables, used in the interrupt service routine!
volatile int BPM[numPulseSensors];                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal[numPulseSensors];                // holds the incoming raw data
volatile int IBI[numPulseSensors];             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse[numPulseSensors];     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS[numPulseSensors];        // becomes true when Arduoino finds a beat.

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse

volatile int rate[numPulseSensors][10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime[numPulseSensors];           // used to find IBI
volatile int P[numPulseSensors];                      // used to find peak in pulse wave, seeded
volatile int T[numPulseSensors];                     // used to find trough in pulse wave, seeded
volatile int thresh[numPulseSensors];                // used to find instant moment of heart beat, seeded
volatile int amp[numPulseSensors];                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat[numPulseSensors];        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat[numPulseSensors];      // used to seed rate array so we startup with reasonable BPM

// Monitoramento
int systolic_limits[] = {280, 360, 485, 562};
int diastolic_limits[] = {160, 240, 325, 365};

float diastolic[11], systolic[11], std_dev;
int dstlc_index, systlc_index;
bool dstlc_init, systlc_init;

float input[3]; ////last 3 readings from sensors
int input_index;
bool input_init; //whether input[] has only valid values

// SET THE SERIAL OUTPUT TYPE TO YOUR NEEDS
// PROCESSING_VISUALIZER works with Pulse Sensor Processing Visualizer
//      https://github.com/WorldFamousElectronics/PulseSensor_Amped_Processing_Visualizer
// SERIAL_PLOTTER outputs sensor data for viewing with the Arduino Serial Plotter
//      run the Arduino Serial Plotter at 115200 baud: Tools/Serial Plotter or Command+L
static int outputType = SERIAL_PLOTTER;

void setup() {

  setStuph();                       // initialize variables and pins

  Serial.begin(250000);             // we agree to talk fast!

  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
}

//  Where the Magic Happens
void loop(){
    /* Systolic range: [90, 250]
     Diastolic range: [60, 140]
    */
      
  int val = analogRead(A0);
  int val2 = analogRead(A1);
  
    serialOutput() ;

  for (int i = 0; i < numPulseSensors; i++) {
    if (QS[i] == true) {
      fadeRate[i] = 0;         // Makes the LED Fade Effect Happen
      // Set 'fadeRate' Variable to 255 to fade LED with pulse
      serialOutputWhenBeatHappens(i);   // A Beat Happened, Output that to serial.
      QS[i] = false;
    }
  }

  ledFadeToBeat();                      // Makes the LED Fade Effect Happen
  delay(20);                             //  take a break
  
  if (val >= diastolic_limits[0] && val2 >= diastolic_limits[0]) {
    //receiving signal. Activate Analyzer
    analyseData(val,val2);
  }
  else {
    reset();
  }
}

// FADE BOTH LEDS
void ledFadeToBeat() {
  for (int j = 0; j < numPulseSensors; j++) {
    fadeRate[j] += 15;
    fadeRate[j] = constrain(fadeRate[j], 0, 255); //  keep RGB LED fade value from going into negative numbers!
    analogWrite(fadePin[j], fadeRate[j]);         //  fade LED
  }
}

// INITIALIZE VARIABLES AND INPUT/OUTPUT PINS
void setStuph() {

  //pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  //pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  
  /* use power from Arduino pins to power PulseSensors with +5V Power!
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);*/

//    Possible Power for 3rd PulseSensor on Pin 7, un-comment
//      pinMode(7,OUTPUT);
//      digitalWrite(7, HIGH);


/*
 * Initialize variables for each pulse sensor so we can find the beats
 */

  for (int i = 0; i < numPulseSensors; i++) {
    lastBeatTime[i] = 0;
    P[i] = T[i] = 512;
    amp[i] = 0;
    thresh[i] = threshold;
    amp[i] = 0;               // used to hold amplitude of pulse waveform, seeded
    firstBeat[i] = true;      // used to seed rate array so we startup with reasonable BPM
    secondBeat[i] = false;
    IBI[i] = 600;             // int that holds the time interval between beats! Must be seeded!
    Pulse[i] = false;         // "True" when User's live heartbeat is detected. "False" when not a "live beat".
    QS[i] = false;
    switch (i) {
      case  0:
        pulsePin[i] = 0;    // pulse pin Analog 0
        blinkPin[i] = 13;   // blink output for pulse 0
        fadePin[i] = 5;     // fade output for pulse 0
        break;
      case  1:
        pulsePin[i] = 1;    // pulse pin Analog 1
        blinkPin[i] = 12;   // blink output for pulse 1
        fadePin[i] = 9;     // fade output for pulse 1
        break;
      // add more if you need to here
      default:
        break;
    }
    pinMode(blinkPin[i], OUTPUT);        // pin that will blink to your heartbeat!
    digitalWrite(blinkPin[i], LOW);
    pinMode(fadePin[i], OUTPUT);         // pin that will fade to your heartbeat!
    analogWrite(fadePin[i], 255);
  }
}

void reset() {
  for (int i = 0; i < 11; i++) {
    //stores the last 10 pulse beats/pressure
    diastolic[i] = 0.0;
    systolic[i] = 1023.0;
    //last value (11th) is the mean
  }
  dstlc_index = 0;
  systlc_index = 0;
  dstlc_init = systlc_init = false;
  std_dev = 0; //standard deviation
  
  for (int i = 0; i < 3; i++) {
    input[i] = -1.0; //last 3 readings from sensors
  }
  input_index = 0;
  input_init = false;
  
  led_on = 0;
  digitalWrite(blinkPin, LOW);
  //digitalWrite(yellow, LOW);
  //digitalWrite(green, LOW);
}

void analyseData(int BPM0, int BPM1) {
  float mean = BPM0 / 2.0;
  //mean += BPM1 / 2.0;
  
  input[input_index] = mean;
  input_index = (input_index + 1) % 3;
  
  if (!input_init && input_index == 0) {
    input_init = true;
  }
  
  if (input_init) {
    if (isLocalMax()) {
      addToSystolic(input[(input_index + 1) % 3]);
    }
    else if (isLocalMin()) {
      addToDiastolic(input[(input_index + 1) % 3]);
    }
  }
  
  //Serial.println(mean);
  if (dstlc_init && systlc_init) {
    monitorar();
  }
}

bool isLocalMax() {
  // function may have one of the following forms:
  // 1 - /\ or ,-
  //int previous = input_index;
  int candidate = (input_index + 1) % 3;
  //int post = (candidate + 1) % 3;
  return (input[input_index] < input[candidate] &&
          input[candidate] >= input[(candidate + 1) % 3]);
}

bool isLocalMin() {
  // function may have one of the following forms:
  // 1 - \/ or \_
  //int previous = input_index;
  int candidate = (input_index + 1) % 3;
  //int post = (candidate + 1) % 3;
  return (input[input_index] > input[candidate] &&
          input[candidate] <= input[(candidate + 1) % 3]);
}

void addToSystolic(float BPM) {
  if (systlc_init) {
    //detects if local maxima represents beat
    if (isSystolic(BPM)) {
      updateSystolic(BPM);
      updateStdDev();
    }
  }
  else {
    updateSystolic(BPM);
    if (systlc_index == 0) { //array filled with valid values
      systlc_init = true;
      updateStdDev();
    }
  }
}

bool isSystolic(float BPM) {
  //detects if local maxima represents beat
  return BPM > diastolic[10] + std_dev;
}

void updateSystolic(float BPM) {
  //computes mean
  systolic[10] -= systolic[systlc_index] / 10;
  systolic[10] += BPM / 10;

  //adds to list
  systolic[systlc_index] = BPM;

  //updates index
  systlc_index = (systlc_index + 1) % 10;
}

void addToDiastolic(float BPM) {
  if (dstlc_init) {
    //detects if local minima represents beat
    if (isDiastolic(BPM)) {
      updateDiastolic(BPM);
      updateStdDev();
    }
  }
  else {
    updateDiastolic(BPM);
    if (dstlc_index == 0) { //array filled with valid values
      dstlc_init = true;
      updateStdDev();
    }
  }
}

bool isDiastolic(float BPM) {
  //detects if local minima represents beat
  return BPM < systolic[10] - std_dev;
}

void updateDiastolic(float BPM) {
    //computes mean
    diastolic[10] -= diastolic[dstlc_index] / 10;
    diastolic[10] += BPM / 10;
    
    //adds to list
    diastolic[dstlc_index] = BPM;
    
    //updates index
    dstlc_index = (dstlc_index + 1) % 10;
}

void updateStdDev() {
  if (systlc_init && dstlc_init) {
    std_dev = systolic[10] - diastolic[10];
    std_dev /= 1.4142; // approx. sqrt(2)
  }
}

void monitorar() {
  if (isPressureLow()) {
    turnLedOn(blinkPin);
  }
  else if (isPressureNormal()) {
    turnLedOn(green);
  }
  else if (isPressureVeryHigh()) {
    turnLedOn(blinkPin);
  }
  else {
    //pressure is high
    turnLedOn(yellow);
  }
  Serial.print(systolic[10]);
  Serial.print(" / ");
  Serial.println(diastolic[10]);
}

bool isPressureLow() {
  return systolic[10] < systolic_limits[1] ||
    diastolic[10] < diastolic_limits[1];
}

bool isPressureNormal() {
  return systolic[10] >= systolic_limits[1] &&
    systolic[10] < systolic_limits[2] &&
    diastolic[10] >= diastolic_limits[1] &&
    diastolic[10] < diastolic_limits[2];
}

bool isPressureVeryHigh() {
  return systolic[10] >= systolic_limits[3] ||
    diastolic[10] >= diastolic_limits[3];
}

void turnLedOn(int new_led) {
  if (led_on != new_led) {
    if (led_on != 0) {
      digitalWrite(led_on, LOW);
    }
    led_on = new_led;
    digitalWrite(led_on, HIGH);
  }
}
