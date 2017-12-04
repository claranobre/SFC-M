/*  Project Base:
    *  Pulse Sensor Multi Sensor

     by
     Joel Murphy and Yury Gitman   http://www.pulsesensor.com *   Updated Winter 2017

     World Famous Electronics llc.  Public Domain. 2017


  ----------------------  Notes ----------------------  ----------------------
  This code:
  1) Blinks an LED to two user's Live Heartbeat   PIN 13 and PIN 12
  2) Fades an LED to two user's Live HeartBeat    PIN 5 and PIN 9
  3) Determines BPMs for both users
  4) Prints All of the Above to Arduino Serial Plotter or our Processing Visualizer

  Plug the Pulse Sensor RED wires into UNO pins 7 and 8 for 5V power! 
  Plug the Pulse Sensor BLACK wires into the GND pins
  Plug the Pulse Sensor PURPLE wires into Analog 0 and Analog 1 pins

  Read Me:
  https://github.com/WorldFamousElectronics/PulseSensorAmped_2_Sensors
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
int yellow[numPulseSensors];
int green[numPulseSensors];
int led_on;

// Volatile Variables, used in the interrupt service routine!
volatile int BPM[numPulseSensors];                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal[numPulseSensors];                // holds the incoming raw data
volatile int IBI[numPulseSensors];             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse[numPulseSensors];     // "True" when User's live heartbeat is detected. "False" [numPulseSensors]when not a "live beat".
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

float valsFromSensors[numPulseSensors];

float input[3]; ////last 3 readings from sensors
int input_index;
bool input_init; //whether input[] has only valid values

// SET THE SERIAL OUTPUT TYPE TO YOUR NEEDS
// PROCESSING_VISUALIZER works with Pulse Sensor Processing Visualizer
//      https://github.com/WorldFamousElectronics/PulseSensor_Amped_Processing_Visualizer
// SERIAL_PLOTTER outputs sensor data for viewing with the Arduino Serial Plotter
//      run the Arduino Serial Plotter at 115200 baud: Tools/Serial Plotter or Command+L

//static int outputType = SERIAL_PLOTTER;
static int outputType = PROCESSING_VISUALIZER;

void setup() {

  setStuph();                       // initialize variables and pins

  Serial.begin(250000);          // we agree to talk fast!
  //Serial.begin(115200);           // we agree to talk fast!
   
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
}

//  Where the Magic Happens
void loop(){
  
    /* Systolic range: [90, 250]
     Diastolic range: [60, 140]
    */
      
  valsFromSensors[0] = analogRead(A0);
  valsFromSensors[1] = analogRead(A1);
  bool doAnalysis = true;
  
  serialOutput() ;

  for (int i = 0; i < numPulseSensors; i++) {
    if (QS[i] == true) {
      fadeRate[i] = 0;         // Makes the LED Fade Effect Happen
      // Set 'fadeRate' Variable to 255 to fade LED with pulse
      serialOutputWhenBeatHappens(i);   // A Beat Happened, Output that to serial.
      
      QS[i] = false;
    }
    if (doAnalysis && valsFromSensors[i] < diastolic_limits[0]) {
      doAnalysis = false;
    }
  }

  if (doAnalysis) {
    analyseData();
  }
  else {
    reset();
  }

  ledFadeToBeat();                      // Makes the LED Fade Effect Happen
  delay(10);                             //  take a break
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
  
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  
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
        //fadePin[i] = 5;     // fade output for pulse 0
        break;
      case  1:
        pulsePin[i] = 1;    // pulse pin Analog 1
        blinkPin[i] = 12;   // blink output for pulse 1
        //fadePin[i] = 9;     // fade output for pulse 1
        break;
      // add more if you need to here
      default: //this is never called
        //monitorar();
        break;
    }
    
    pinMode(blinkPin[i], OUTPUT);        // pin that will blink to your heartbeat!
    digitalWrite(blinkPin[i], LOW);
    pinMode(fadePin[i], OUTPUT);         // pin that will fade to your heartbeat!
    analogWrite(fadePin[i], 255);
  }
}

void monitorar() {
  for (int i = 0; i < numPulseSensors; i++) {
    if (isPressureLow()) {
      turnLedOn(blinkPin[i]);
    }
    else if (isPressureNormal()) {
      turnLedOn(green[i]);
    }
    else if (isPressureVeryHigh()) {
      turnLedOn(blinkPin[i]);
    }
    else {
    //pressure is high
      turnLedOn(yellow[i]);
    }

  Serial.print(systolic[10]);
  Serial.print("/");
  Serial.println(diastolic[10]);
  
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
  //digitalWrite(yellow, LOW);
  //digitalWrite(green, LOW);
}

void analyseData() {
  float mean = 0.0;
  for (int i = 0; i < numPulseSensors; i++) {
   mean += valsFromSensors[i] / numPulseSensors;
  }
  
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

void addToSystolic(float val) {
  if (systlc_init) {
    //detects if local maxima represents beat
    if (isSystolic(val)) {
      updateSystolic(val);
      updateStdDev();
    }
  }
  else {
    updateSystolic(val);
    if (systlc_index == 0) { //array filled with valid values
      systlc_init = true;
      updateStdDev();
    }
  }
}

bool isSystolic(float val) {
  //detects if local maxima represents beat
  return val > diastolic[10] + std_dev;
}

void updateSystolic(float val) {
  //computes mean
  systolic[10] -= systolic[systlc_index] / 10;
  systolic[10] += val / 10;

  //adds to list
  systolic[systlc_index] = val;

  //updates index
  systlc_index = (systlc_index + 1) % 10;
}

void addToDiastolic(float val) {
  if (dstlc_init) {
    //detects if local minima represents beat
    if (isDiastolic(val)) {
      updateDiastolic(val);
      updateStdDev();
    }
  }
  else {
    updateDiastolic(val);
    if (dstlc_index == 0) { //array filled with valid values
      dstlc_init = true;
      updateStdDev();
    }
  }
}

bool isDiastolic(float val) {
  //detects if local minima represents beat
  return val < systolic[10] - std_dev;
}

void updateDiastolic(float val) {
    //computes mean
    diastolic[10] -= diastolic[dstlc_index] / 10;
    diastolic[10] += val / 10;
    
    //adds to list
    diastolic[dstlc_index] = val;
    
    //updates index
    dstlc_index = (dstlc_index + 1) % 10;
}

void updateStdDev() {
  if (systlc_init && dstlc_init) {
    std_dev = systolic[10] - diastolic[10];
    std_dev /= 1.4142; // approx. sqrt(2)
  }
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
