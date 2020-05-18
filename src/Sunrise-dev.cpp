#include <stdint.h>
#include <Arduino.h>
#include "Sunrise.h"

#define SUNRISE_ADDR            0x68

#define PIN_EN                  2
#define PIN_NRDY                3

//#define MEASUREMENT_INTERVAL    (5UL * 1000UL)              // 5s
//#define MEASUREMENT_INTERVAL    (60UL * 1000UL)             // 1m
#define MEASUREMENT_INTERVAL    ((5UL * 60UL) * 1000UL)     // 5m

#define MS_PER_H                ((60UL * 60UL) * 1000UL)    // 3600000

Sunrise sunrise;
const measurementmode_t measurementMode = measurementmode_t::MM_SINGLE;
uint16_t hourCount = 0U;
unsigned long int nextHour;
unsigned long int nextMeasurement;

volatile uint8_t isReady = false;
void nrdyISR(void)
{
    isReady = true;
}

bool awaitISR(unsigned long int timeout = 2000UL)
{
    timeout += millis();
    while(!isReady && (long)(millis() - timeout) < 0L);
    return isReady;
}

void delayUntil(unsigned long int time)
{
    while((long)(millis() - time) < 0L);
}

void switchMode(measurementmode_t mode)
{
    while(true)
    {
        measurementmode_t measurementMode;
        if(!sunrise.GetMeasurementModeEE(measurementMode))
        {
            Serial.println("*** ERROR: Could not get measurement mode");
            while(true);
        }

        if(measurementMode == mode)
        {
            break;
        }

        Serial.println("Attempting to switch measurement mode...");
        if(!sunrise.SetMeasurementModeEE(mode))
        {
            Serial.println("*** ERROR: Could not set measurement mode");
            while(true);
        }

        if(!sunrise.HardRestart())
        {
            Serial.println("*** ERROR: Failed to restart the device");
            while(true);
        }
    }
}

void setup(void)
{
    delay(2500UL);

    Serial.begin(9600);
    while(!Serial);

    Serial.println("Initializing...");
    pinMode(PIN_NRDY, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_NRDY), nrdyISR, FALLING);

    if(!sunrise.Begin(PIN_EN, true))
    {
        Serial.println("Error: Could not initialize the device");
        while(true);
    }

    sunrise.Awake();
    switchMode(measurementMode);
    sunrise.Sleep();

    Serial.println("Done");
    Serial.println();
    Serial.flush();

    //while(true);

    nextHour = millis() + MS_PER_H;
    nextMeasurement = millis();
}

void loop(void)
{
    sunrise.Awake();
    #ifdef OUTPUT_TEXT
    Serial.println("Measuring...");
    #endif

    if((long)(millis() - nextHour) >= 0L)
    {
        hourCount++;
        nextHour += MS_PER_H;
    }

    uint16_t tmpHourCount = sunrise.GetABCTime();
    #ifdef OUTPUT_TEXT
    Serial.print("Hour count:    "); Serial.print(tmpHourCount);
    #endif
    if(hourCount != tmpHourCount)
    {
        sunrise.SetABCTime(hourCount);
        #ifdef OUTPUT_TEXT
        Serial.print(" => "); Serial.print(hourCount);
        #endif
    }
    #ifdef OUTPUT_TEXT
    Serial.println();
    #endif

    isReady = false;
    if(sunrise.StartSingleMeasurement())
    {
        unsigned long int measurementStartTime = millis();
        if(awaitISR())
        {
            #ifdef OUTPUT_TEXT
            unsigned long int measurementDuration = millis() - measurementStartTime;
            #endif

            #ifdef OUTPUT_TEXT
            Serial.print("Duration:      "); Serial.println(measurementDuration);
            #endif
            if(sunrise.ReadMeasurement())
            {
                #ifdef OUTPUT_TEXT
                Serial.print("Time:          "); Serial.println(measurementStartTime);
                Serial.print("Error status:  "); Serial.println(sunrise.GetErrorStatusRaw(), BIN);
                Serial.print("CO2:           "); Serial.println(sunrise.GetCO2());
                Serial.print("Temperature:   "); Serial.println(sunrise.GetTemperature());
                Serial.print("CO2 UP:        "); Serial.println(sunrise.GetCO2_UP());
                Serial.print("CO2 F:         "); Serial.println(sunrise.GetCO2_F());
                Serial.print("CO2 U:         "); Serial.println(sunrise.GetCO2_U());
                //Serial.print("Count:         "); Serial.println(sunrise.GetMeasurementCount());
                //Serial.print("Cycle time:    "); Serial.println(sunrise.GetMeasurementCycleTime());
                #endif

                #ifdef OUTPUT_CSV
                Serial.print(measurementStartTime); Serial.print(';');
                Serial.print(sunrise.GetCO2()); Serial.print(';');
                Serial.print(sunrise.GetTemperature()); Serial.print(';');
                Serial.print(sunrise.GetErrorStatusRaw()); Serial.print(';');
                //Serial.print(sunrise.GetCO2_UP()); Serial.print(';');
                //Serial.print(sunrise.GetCO2_F()); Serial.print(';');
                //Serial.print(sunrise.GetCO2_U()); Serial.print(';');
                //Serial.print(sunrise.GetMeasurementCount()); Serial.print(';');
                //Serial.print(sunrise.GetMeasurementCycleTime()); Serial.print(';');
                #endif
            }
            else
            {
                Serial.println("*** ERROR: Reading measurement");
                //while(true);
            }
        }
        else
        {
            Serial.println("*** ERROR: ISR timeout");
            //while(true);
        }
    }
    else
    {
        Serial.println("*** ERROR: Starting single measurement");
        //while(true);
    }
    #ifdef OUTPUT_TEXT
    Serial.println();
    #endif
    #ifdef OUTPUT_CSV
    Serial.print("\n");
    #endif

    sunrise.Sleep();

    nextMeasurement += MEASUREMENT_INTERVAL;
    delayUntil(nextMeasurement);
}
