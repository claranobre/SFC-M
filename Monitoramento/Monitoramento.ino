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

int red = 13;
int yellow = 12;
int green = 11;
int led_on;

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
//      run the Serial Plotter at 115200 baud: Tools/Serial Plotter or Command+L
//static int outputType = SERIAL_PLOTTER;

static int OUTPUT_TYPE = PROCESSING_VISUALIZER;

void setup()
{
  Serial.begin(115200);
  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  reset();
}

void loop()
{
  /* Systolic range: [90, 250]
  	 Diastolic range: [60, 140]
  */
  
  int val = analogRead(A0);
  int val2 = analogRead(A1);
  
  if (val >= diastolic_limits[0] && val2 >= diastolic_limits[0]) {
    //receiving signal. Activate Analyzer
    analyseData(val, val2);
  }
  else {
    reset();
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
  digitalWrite(red, LOW);
  digitalWrite(yellow, LOW);
  digitalWrite(green, LOW);
}

void analyseData(int val, int val2) {
  float mean = val / 2.0;
  mean += val2 / 2.0;
  
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

void monitorar() {
  if (isPressureLow()) {
    turnLedOn(red);
  }
  else if (isPressureNormal()) {
    turnLedOn(green);
  }
  else if (isPressureVeryHigh()) {
    turnLedOn(red);
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
