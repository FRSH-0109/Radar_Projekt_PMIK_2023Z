/**
 * @file radar_driver.h
 * @author Kami Kośnik, Kacper Radzikowski
 * @date 20 Nov 2023
 * @brief File used to collapse ale used by radar stuff,
 * sensors, servo's etc in one place.
 *
 * @see https://github.com/FRSH-0109/Radar_Project_PMIK_2023Z
 */


#include "radar.h"

extern drawHelperStruct drawHelper;

void radarInit(radarStruct* radar, servoDriverStruct* servo, TIM_HandleTypeDef* servo_tim, distanceSensorStruct* sensor, TIM_HandleTypeDef* sensor_tim)
{
	radar->servo = servo;
	radar->position = 0.0f;
	radar->servoTim = servo_tim;
	radar->direction = 1;
	radarSetPositionMin(radar, 0.0f);
	radarSetPositionMax(radar, 180.0f);
	radarSetPositionUpdateStep(radar, 0.5f);
	radarSetPositionUpdatePeriod(radar, 11);

	radar->sensor = sensor;
	radar->sensorTim = sensor_tim;
	radarSetMeasureFreq(radar, SENSOR_MEASURE_PERIOD_DEFAULT_MS);
}


////////////// SERVO //////////////////////////////////
void radarServoUpdate(radarStruct* radar)
{
	float posNew = 0;
	posNew = (radar->direction * radar->positionUpdateStep) + radar->position;

	if(posNew < radar->positionMin)
	{
		posNew = radar->positionMin;
		radar->direction *= -1;
	}
	else if(posNew > radar->positionMax)
	{
		posNew = radar->positionMax;
		radar->direction *= -1;
	}

	radarSetPosition(radar, posNew);
}

bool radarSetPositionMin(radarStruct* radar, float pos)
{
	if(pos >= SERVO_MIN_DEGREES && pos <= SERVO_MAX_DEGREES && pos < radar->positionMax)
	{
		radar->positionMin = pos;
		return true;
	}

	return false;
}

float radarGetPositionMin(radarStruct* radar)
{
	return radar->positionMin;
}

bool radarSetPositionMax(radarStruct* radar, float pos)
{
	if(pos >= SERVO_MIN_DEGREES && pos <= SERVO_MAX_DEGREES && pos > radar->positionMin)
	{
		radar->positionMax = pos;
		return true;
	}

	return false;
}

float radarGetPositionMax(radarStruct* radar)
{
	return radar->positionMax;
}

void radarServoStart(radarStruct* radar)
{
	servoDriverStartTimer(radar->servo);
	HAL_TIM_Base_Start_IT(radar->servoTim);
}

void radarServoStop(radarStruct* radar)
{
	HAL_TIM_Base_Stop_IT(radar->servoTim);
}

bool radarSetPositionUpdatePeriod(radarStruct* radar, uint16_t periodMs)
{
	if(periodMs > 10 && periodMs < UINT16_MAX)
	{
		radar->positionUpdatePeriodMs = periodMs;
		__HAL_TIM_SET_AUTORELOAD(radar->servoTim, periodMs);
		return true;
	}

	return false;
}

bool radarSetPositionUpdateStep(radarStruct* radar, float step)
{
	if(step > 0.001 && step < 90)
	{
		radar->positionUpdateStep = step;
		return true;
	}

	return false;
}

bool radarSetPositionMinMax(radarStruct* radar, float pos, bool isMin)
{
	if(pos >= 0.0 && pos <= 180.0)
	{
		if(isMin)
		{
			radar->positionMin = pos;
		}
		else
		{
			radar->positionMax = pos;
		}
		return true;
	}

	return false;
}

bool radarSetPosition(radarStruct* radar, float pos)
{
	if(servoDriverSetDegrees(radar->servo, pos))
	{
		radar->position = pos;
		return true;
	}
	return false;
}

float radarGetPosition(radarStruct* radar)
{
	return radar->position;
}
////////////// SERVO //////////////////////////////////



////////////// SENSOR //////////////////////////////////
void radarMeasureStart(radarStruct* radar)
{
	HAL_TIM_Base_Start_IT(radar->sensorTim);
}

void radarMeasureStop(radarStruct* radar)
{
	HAL_TIM_Base_Stop_IT(radar->sensorTim);
}

bool radarSetMeasureFreq(radarStruct* radar, uint16_t periodMs)
{
	if(periodMs > 10 && periodMs < UINT16_MAX)
	{
		radar->measurePeriodMs = periodMs;
		__HAL_TIM_SET_COUNTER(radar->sensorTim, 0);
		__HAL_TIM_SET_AUTORELOAD(radar->sensorTim, periodMs);
		return true;
	}

	return false;
}

inline void radarTriggerMeasure(radarStruct* radar)
{
	distanceSensorSendTrig(radar->sensor);
}

double radarGetMeasure(radarStruct* radar)
{
	return distanceSensorGetDistance(radar->sensor);
}


////////////// SENSOR //////////////////////////////////

////////////// DISPLAY //////////////////////////////////
void radarDrawMeasure(radarStruct* radar)
{

}
////////////// DISPLAY //////////////////////////////////


//PARSING//
bool radarParseSetPosition(radarStruct* radar, uint8_t* data)
{
	float newPos = 0;
	char* dataNew = strnstr((char*) data, COMMAND_SET_RADAR_POSITION, strlen((char*)data));

	if(dataNew == NULL)
	{
		return false;
	}

	dataNew += strlen((char*) COMMAND_SET_RADAR_POSITION);

	if (sscanf(dataNew, "%f", &newPos) != 1)
	{
		return false;
	}

	return radarSetPosition(radar, newPos);
}

bool radarParseSetStep(radarStruct* radar, uint8_t* data)
{
	float newStep = 0;
	char* dataNew = strnstr((char*) data, COMMAND_SET_RADAR_STEP, strlen((char*)data));

	if(dataNew == NULL)
	{
		return false;
	}

	dataNew += strlen((char*) COMMAND_SET_RADAR_STEP);

	if (sscanf(dataNew, "%f", &newStep) != 1)
	{
		return false;
	}

	return radarSetPositionUpdateStep(radar, newStep);
}

bool radarParseSetPositionMinMax(radarStruct* radar, uint8_t* data, bool isMin)
{
	float newPos = 0;
	char* dataNew = NULL;
	if(isMin)
	{
		dataNew = strnstr((char*) data, COMMAND_SET_RADAR_POS_MIN, strlen((char*)data));
	}
	else
	{
		dataNew = strnstr((char*) data, COMMAND_SET_RADAR_POS_MAX, strlen((char*)data));
	}

	if(dataNew == NULL)
	{
		return false;
	}

	if(isMin)
	{
		dataNew += strlen((char*) COMMAND_SET_RADAR_POS_MIN);
	}
	else
	{
		dataNew += strlen((char*) COMMAND_SET_RADAR_POS_MAX);
	}

	if (sscanf(dataNew, "%f", &newPos) != 1)
	{
		return false;
	}

	return radarSetPositionMinMax(radar, newPos, isMin);
}

bool radarParseSetMeasurePeriod(radarStruct* radar, uint8_t* data)
{
	int newPeriod = 0;
	char* dataNew = strnstr((char*) data, COMMAND_SET_RADAR_SENSOR_PERIOD, strlen((char*)data));

	if(dataNew == NULL)
	{
		return false;
	}

	dataNew += strlen((char*) COMMAND_SET_RADAR_SENSOR_PERIOD);

	if (sscanf(dataNew, "%d", &newPeriod) != 1)
	{
		return false;
	}

	return radarSetMeasureFreq(radar, newPeriod);
}

bool radarParseSetDrawScale(radarStruct* radar, uint8_t* data)
{
	float newScale = 0;
	char* dataNew = strnstr((char*) data, COMMAND_SET_RADAR_DRAW_SCALE, strlen((char*)data));

	if(dataNew == NULL)
	{
		return false;
	}

	dataNew += strlen((char*) COMMAND_SET_RADAR_DRAW_SCALE);

	if (sscanf(dataNew, "%f", &newScale) != 1)
	{
		return false;
	}

	drawClear();
	return drawSetupUpdate(newScale, drawHelper.measureScalesNumber);
}

bool radarParseSetDrawRules(radarStruct* radar, uint8_t* data)
{
	float newRulesNum = 0;
	char* dataNew = strnstr((char*) data, COMMAND_SET_RADAR_DRAW_RULES, strlen((char*)data));

	if(dataNew == NULL)
	{
		return false;
	}

	dataNew += strlen((char*) COMMAND_SET_RADAR_DRAW_RULES);

	if (sscanf(dataNew, "%f", &newRulesNum) != 1)
	{
		return false;
	}

	drawClear();
	return drawSetupUpdate(drawHelper.measureScalesNumber, newRulesNum);
}
