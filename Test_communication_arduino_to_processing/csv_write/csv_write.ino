/*
Sending Data to Processing via the Serial Port
This sketch provides a basic framework to send data from Arduino to Processing over a Serial Port. This is a beginning level sketch.

Hardware:
* Sensors connected to Arduino input pins
* Arduino connected to computer via USB cord

Software:
*Arduino programmer
*Processing (download the Processing software here: https://www.processing.org/download/

Additional Libraries:
*Read about the Software Serial library here: http://arduino.cc/en/Reference/softwareSerial

Created 12 November 2014
By Elaine Laguerta
http://url/of/online/tutorial.cc
*/

/*To avoid overloading the Arduino memory, and to encourage portability to smaller microprocessors, this sketch
does not timestamp or transform data. In this tutorial, timestamping data is handled on the processing side.

Whether you process data on the Arduino side is up to you. Given memory limitations of the Arduino, even a few computations and mapping of values can
max out the memory and fail. I recommend doing as little as possible on the Arduino board.*/

//#include SoftwareSerial.h

/*Declare your sensor pins as variables. I'm using Analog In pins 0 and 1. Change the names and numbers as needed
Pro tip: If you're pressed for memory, use #define to declare your sensor pins without using any memory. Just be careful that your pin name shows up NOWHERE ELSE in your sketch!
for more info, see: http://arduino.cc/en/Reference/Define
*/
int sensor1Pin = 0;
int sensor2Pin = 1;
int sensor3Pin = 2;

/*Create an array to store sensor values. I'm using floats. Floats use 4 bytes to represent numbers in exponential notation. Use int if you are representing whole numbers from -32,768 to 32,767.
For more info on the appropriate data type for your sensor values, check out the language reference on data type: http://arduino.cc/en/Reference/HomePage
Customize the array's size to be equal to your number of sensors.
*/
float sensorVal[] = {0,1,2};

/*Pro tip: if you have a larger number of sensors, you can use a for loop to initialize your sensor value array. Here's sample code (assuming you have 6 sensor values):
float sensorVals[6];
int i;
for (i=0; i&lt;6; i++)
{
sensorVals[i] = 0;
}
*/

void setup(){
Serial.begin(9600); //This line tells the Serial port to begin communicating at 9600 bauds
}

//
void loop(){
//read each sensor value. We are assuming analog values. Customize as nessary to read all of your sensor values into the array. Remember to replace "sensor1Pin" and "sensor2Pin" with your actual pin names from above!
int v = sensorVal[0];
sensorVal[0] = sensorVal[1];
sensorVal[1] = sensorVal[2];
sensorVal[2] = v;
/*If you are reading digital values, use digitalRead() instead. Here's an example:
sensorVal[0] = digitalRead(sensor1Pin);
*/

//print over the serial line to send to Processing. To work with the processisng sketch we provide, follow this easy convention: separate each sensor value with a comma, and separate each cycle of loop with a newline character.
//Remember to separate each sensor value with a comma. Print every value and comma using Serial.print(), except the last sensor value. For the last sensor value, use Serial.println()
Serial.print(sensorVal[0]);
Serial.print(",");
Serial.print(sensorVal[1]);
Serial.print(",");
Serial.println(sensorVal[2]);
delay(100);
}
