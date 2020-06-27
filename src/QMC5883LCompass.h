#ifndef QMC5883L_Compass
#define QMC5883L_Compass

#include "Arduino.h"
#include "Wire.h"


class QMC5883LCompass{
	
  public:
    QMC5883LCompass();
	void init();
    void setADDR(byte b);
    void setMode(byte mode, byte odr, byte rng, byte osr);
	void setSmoothing(byte steps, bool adv);
	void setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max);
    void setReset();
    void read();
	int getX();
	int getY();
	int getZ();
	int getAzimuth();
	byte getBearing(int azimuth);
	void getDirection(char* myArray, int azimuth);
	
  private:
    void _writeReg(byte reg,byte val);
	int _get(int index);
	bool _smoothUse = false;
	byte _smoothSteps = 5;
	bool _smoothAdvanced = false;
    byte _ADDR = 0x0D;
	int _vRaw[3] = {0,0,0};
	int _vHistory[10][3];
	int _vScan = 0;
	long _vTotals[3] = {0,0,0};
	int _vSmooth[3] = {0,0,0};
	void _smoothing();
	bool _calibrationUse = false;
	int _vCalibration[3][2];
	int _vCalibrated[3];
	void _applyCalibration();
	const char _bearings[16][3] =  {
		{' ', ' ', 'N'},
		{'N', 'N', 'E'},
		{' ', 'N', 'E'},
		{'E', 'N', 'E'},
		{' ', ' ', 'E'},
		{'E', 'S', 'E'},
		{' ', 'S', 'E'},
		{'S', 'S', 'E'},
		{' ', ' ', 'S'},
		{'S', 'S', 'W'},
		{' ', 'S', 'W'},
		{'W', 'S', 'W'},
		{' ', ' ', 'W'},
		{'W', 'N', 'W'},
		{' ', 'N', 'W'},
		{'N', 'N', 'W'},
	};
	
	
	
};

#endif
