/*
===============================================================================================================
QMC5883LCompass.h
Library for using QMC5583L series chip boards as a compass.
Learn more at [https://github.com/mprograms/QMC5883LCompass]

Supports:

- Getting values of XYZ axis.
- Calculating Azimuth.
- Getting 16 point Azimuth bearing direction (0 - 15).
- Getting 16 point Azimuth bearing Names (N, NNE, NE, ENE, E, ESE, SE, SSE, S, SSW, SW, WSW, W, WNW, NW, NNW)
- Smoothing of XYZ readings via rolling averaging and min / max removal.
- Optional chipset modes (see below)

===============================================================================================================

v1.0 - June 13, 2019
Written by MRPrograms 
Github: [https://github.com/mprograms/]

Release under the GNU General Public License v3
[https://www.gnu.org/licenses/gpl-3.0.en.html]

===============================================================================================================



FROM QST QMC5883L Datasheet [https://nettigo.pl/attachments/440]
-----------------------------------------------
 MODE CONTROL (MODE)
	Standby			0x00
	Continuous		0x01

OUTPUT DATA RATE (ODR)
	10Hz        	0x00
	50Hz        	0x04
	100Hz       	0x08
	200Hz       	0x0C

FULL SCALE (RNG)
	2G          	0x00
	8G          	0x10

OVER SAMPLE RATIO (OSR)
	512         	0x00
	256         	0x40
	128         	0x80
	64          	0xC0 
  
*/



#include "Arduino.h"
#include "QMC5883LCompass.h"
#include <Wire.h>

QMC5883LCompass::QMC5883LCompass() {

}


/**
	INIT
	Initialize Chip - This needs to be called in the sketch setup() function.

	Optional:
	A predefined TwoWire bus can be assigned using "init(&YourPredefinedBus);": 
	Example: 
		compass.init(&I2C_1);
	@since v0.1;
**/
void QMC5883LCompass::init(TwoWire *bus){
	_int_bus = bus;
	_int_bus->begin();
	_writeReg(0x0B,0x01);
	setMode(0x01,0x0C,0x10,0X00);
}


/**
	SET ADDRESS
	Set the I2C Address of the chip. This needs to be called in the sketch setup() function.
	
	@since v0.1;
**/
// Set I2C Address if different then default.
void QMC5883LCompass::setADDR(byte b){
	_ADDR = b;
}




/**
	REGISTER
	Write the register to the chip.
	
	@since v0.1;
**/
// Write register values to chip
void QMC5883LCompass::_writeReg(byte r, byte v){
	_int_bus->beginTransmission(_ADDR);
	_int_bus->write(r);
	_int_bus->write(v);
	_int_bus->endTransmission();
}


/**
	CHIP MODE
	Set the chip mode.
	
	@since v0.1;
**/
// Set chip mode
void QMC5883LCompass::setMode(byte mode, byte odr, byte rng, byte osr){
	_writeReg(0x09,mode|odr|rng|osr);
}


/**
	RESET
	Reset the chip.
	
	@since v0.1;
**/
// Reset the chip
void QMC5883LCompass::setReset(){
	_writeReg(0x0A,0x80);
}

// 1 = Basic 2 = Advanced
void QMC5883LCompass::setSmoothing(byte steps, bool adv){
	_smoothUse = true;
	_smoothSteps = ( steps > 10) ? 10 : steps;
	_smoothAdvanced = (adv == true) ? true : false;
}

/**
    SET CALIBRATION
	Set calibration values for more accurate readings
		
	@author Claus Näveke - TheNitek [https://github.com/TheNitek]
	
	@since v1.1.0
**/
void QMC5883LCompass::setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max){
	_calibrationUse = true;

	_vCalibration[0][0] = x_min;
	_vCalibration[0][1] = x_max;
	_vCalibration[1][0] = y_min;
	_vCalibration[1][1] = y_max;
	_vCalibration[2][0] = z_min;
	_vCalibration[2][1] = z_max;
}



/**
	READ
	Read the XYZ axis and save the values in an array.
	
	@since v0.1;
**/
void QMC5883LCompass::read(){
	_int_bus->beginTransmission(_ADDR);
	_int_bus->write(0x00);
	int err = _int_bus->endTransmission();
	if (!err) {
		_int_bus->requestFrom(_ADDR, (byte)6);
		_vRaw[0] = (int)(int16_t)(_int_bus->read() | _int_bus->read() << 8);
		_vRaw[1] = (int)(int16_t)(_int_bus->read() | _int_bus->read() << 8);
		_vRaw[2] = (int)(int16_t)(_int_bus->read() | _int_bus->read() << 8);

		if ( _calibrationUse ) {
			_applyCalibration();
		}
		
		if ( _smoothUse ) {
			_smoothing();
		}
		
		//byte overflow = Wire.read() & 0x02;
		//return overflow << 2;
	}
}

/**
    APPLY CALIBRATION
	This function uses the calibration data provided via @see setCalibration() to calculate more
	accurate readings
	
	@author Claus Näveke - TheNitek [https://github.com/TheNitek]
	
	Based on this awesome article:
	https://appelsiini.net/2018/calibrate-magnetometer/
	
	@since v1.1.0
	
**/
void QMC5883LCompass::_applyCalibration(){
	int x_offset = (_vCalibration[0][0] + _vCalibration[0][1])/2;
	int y_offset = (_vCalibration[1][0] + _vCalibration[1][1])/2;
	int z_offset = (_vCalibration[2][0] + _vCalibration[2][1])/2;
	int x_avg_delta = (_vCalibration[0][1] - _vCalibration[0][0])/2;
	int y_avg_delta = (_vCalibration[1][1] - _vCalibration[1][0])/2;
	int z_avg_delta = (_vCalibration[2][1] - _vCalibration[2][0])/2;

	int avg_delta = (x_avg_delta + y_avg_delta + z_avg_delta) / 3;

	float x_scale = (float)avg_delta / x_avg_delta;
	float y_scale = (float)avg_delta / y_avg_delta;
	float z_scale = (float)avg_delta / z_avg_delta;

	_vCalibrated[0] = (_vRaw[0] - x_offset) * x_scale;
	_vCalibrated[1] = (_vRaw[1] - y_offset) * y_scale;
	_vCalibrated[2] = (_vRaw[2] - z_offset) * z_scale;
}


/**
	SMOOTH OUTPUT
	This function smooths the output for the XYZ axis. Depending on the options set in
	@see setSmoothing(), we can run multiple methods of smoothing the sensor readings.
	
	First we store (n) samples of sensor readings for each axis and store them in a rolling array.
	As each new sensor reading comes in we replace it with a new reading. Then we average the total
	of all (n) readings.
	
	Advanced Smoothing
	If you turn advanced smoothing on, we will select the min and max values from our array
	of (n) samples. We then subtract both the min and max from the total and average the total of all
	(n - 2) readings.
	
	NOTE: This function does several calculations and can cause your sketch to run slower.
	
	@since v0.3;
**/
void QMC5883LCompass::_smoothing(){
	byte max = 0;
	byte min = 0;
	
	if ( _vScan > _smoothSteps - 1 ) { _vScan = 0; }
	
	for ( int i = 0; i < 3; i++ ) {
		if ( _vTotals[i] != 0 ) {
			_vTotals[i] = _vTotals[i] - _vHistory[_vScan][i];
		}
		_vHistory[_vScan][i] = ( _calibrationUse ) ? _vCalibrated[i] : _vRaw[i];
		_vTotals[i] = _vTotals[i] + _vHistory[_vScan][i];
		
		if ( _smoothAdvanced ) {
			max = 0;
			for (int j = 0; j < _smoothSteps - 1; j++) {
				max = ( _vHistory[j][i] > _vHistory[max][i] ) ? j : max;
			}
			
			min = 0;
			for (int k = 0; k < _smoothSteps - 1; k++) {
				min = ( _vHistory[k][i] < _vHistory[min][i] ) ? k : min;
			}
					
			_vSmooth[i] = ( _vTotals[i] - (_vHistory[max][i] + _vHistory[min][i]) ) / (_smoothSteps - 2);
		} else {
			_vSmooth[i] = _vTotals[i]  / _smoothSteps;
		}
	}
	
	_vScan++;
}


/**
	GET X AXIS
	Read the X axis
	
	@since v0.1;
	@return int x axis
**/
int QMC5883LCompass::getX(){
	return _get(0);
}


/**
	GET Y AXIS
	Read the Y axis
	
	@since v0.1;
	@return int y axis
**/
int QMC5883LCompass::getY(){
	return _get(1);
}


/**
	GET Z AXIS
	Read the Z axis
	
	@since v0.1;
	@return int z axis
**/
int QMC5883LCompass::getZ(){
	return _get(2);
}

/**
	GET SENSOR AXIS READING
	Get the smoothed, calibration, or raw data from a given sensor axis
	
	@since v1.1.0
	@return int sensor axis value
**/
int QMC5883LCompass::_get(int i){
	if ( _smoothUse ) 
		return _vSmooth[i];
	
	if ( _calibrationUse )
		return _vCalibrated[i];

	return _vRaw[i];
}



/**
	GET AZIMUTH
	Calculate the azimuth (in degrees);
	
	@since v0.1;
	@return int azimuth
**/
int QMC5883LCompass::getAzimuth(){
	int a = atan2( getY(), getX() ) * 180.0 / PI;
	return a < 0 ? 360 + a : a;
}


/**
	GET BEARING
	Divide the 360 degree circle into 16 equal parts and then return the a value of 0-15
	based on where the azimuth is currently pointing.
	
	@since v1.0.1 - function now requires azimuth parameter.
	@since v0.2.0 - initial creation
	
	@return byte direction of bearing
*/
byte QMC5883LCompass::getBearing(int azimuth){
	unsigned long a = azimuth / 22.5;
	unsigned long r = a - (int)a;
	byte sexdec = 0;	
	sexdec = ( r >= .5 ) ? ceil(a) : floor(a);
	return sexdec;
}


/**
	This will take the location of the azimuth as calculated in getBearing() and then
	produce an array of chars as a text representation of the direction.
	
	NOTE: This function does not return anything since it is not possible to return an array.
	Values must be passed by reference back to your sketch.
	
	Example:
	
	( if direction is in 1 / NNE)
	
	char myArray[3];
	compass.getDirection(myArray, azimuth);
	
	Serial.print(myArray[0]); // N
	Serial.print(myArray[1]); // N
	Serial.print(myArray[2]); // E
	
	
	@see getBearing();
	
	@since v1.0.1 - function now requires azimuth parameter.
	@since v0.2.0 - initial creation
*/
void QMC5883LCompass::getDirection(char* myArray, int azimuth){
	int d = getBearing(azimuth);
	myArray[0] = _bearings[d][0];
	myArray[1] = _bearings[d][1];
	myArray[2] = _bearings[d][2];
}