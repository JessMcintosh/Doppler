/**************************************************************************
// PICO_SIGNATURE
 *
 * Description:
 *   This is a console mode program that demonstrates how to use the
 *   PicoScope 3000 Series A API.
 *
 * Examples:
 *    Collect a block of samples immediately
 *    Collect a block of samples when a trigger event occurs
 *    Collect a stream of data immediately
 *    Collect a stream of data when a trigger event occurs
 *    Set Signal Generator, using standard or custom signals
 *    Change timebase & voltage scales
 *    Display data in mV or ADC counts
 *    Handle power source changes (PicoScope 34XXA/B & 32XX & 34XX D/D MSO devices only)
 *
 * Digital Examples (MSO variants only): 
 *    Collect a block of digital samples immediately
 *    Collect a block of digital samples when a trigger event occurs
 *    Collect a block of analogue & digital samples when analogue AND digital trigger events occurs
 *    Collect a block of analogue & digital samples when analogue OR digital trigger events occurs
 *    Collect a stream of digital data immediately
 *    Collect a stream of digital data and show aggregated values
 *
 *	To build this application:
 *
 *		If Microsoft Visual Studio (including Express) is being used:
 *
 *			Select the solution configuration (Debug/Release) and platform (Win32/x64)
 *			Ensure that the 32-/64-bit ps3000a.lib can be located
 *			Ensure that the ps3000aApi.h and picoStatus.h files can be located
 *		
 *		Otherwise:
 *
 *			 Set up a project for a 32-/64-bit console mode application
 *			 Add this file to the project
 *			 Add 32-/64-bit ps3000a.lib to the project
 *			 Add ps3000aApi.h and picoStatus.h to the project
 *
 *		Build the project
 *
 **************************************************************************/

#include <stdio.h>

/* Headers for Windows */
#ifdef _WIN32
#include "windows.h"
#include <conio.h>
#include "ps3000aApi.h"
#else
#include <sys/types.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <libps3000a-1.1/ps3000aApi.h>
#ifndef PICO_STATUS
#include <libps3000a-1.1/PicoStatus.h>
#endif

#define Sleep(a) usleep(1000*a)
#define scanf_s scanf
#define fscanf_s fscanf
#define memcpy_s(a,b,c,d) memcpy(a,c,d)

typedef enum enBOOL{FALSE,TRUE} BOOL;

#define PORT 45454
#define SRV_IP "127.0.0.1"

/* A function to detect a keyboard press on Linux */
int32_t _getch()
{
	struct termios oldt, newt;
	int32_t ch;
	int32_t bytesWaiting;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	setbuf(stdin, NULL);
	do {
		ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
		if (bytesWaiting)
			getchar();
	} while (bytesWaiting);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

int32_t _kbhit()
{
	struct termios oldt, newt;
	int32_t bytesWaiting;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	setbuf(stdin, NULL);
	ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return bytesWaiting;
}

int32_t fopen_s(FILE ** a, const char * b, const char * c)
{
	FILE * fp = fopen(b,c);
	*a = fp;
	return (fp>0)?0:-1;
}

/* A function to get a single character on Linux */
#define max(a,b) ((a) > (b) ? a : b)
#define min(a,b) ((a) < (b) ? a : b)
#endif

#define PREF4 __stdcall

int32_t cycles = 0;

#define BUFFER_SIZE 	1024

#define QUAD_SCOPE		4
#define DUAL_SCOPE		2

// AWG Parameters

#define AWG_DAC_FREQUENCY			20e6		
#define AWG_DAC_FREQUENCY_PS3207B	100e6
#define	AWG_PHASE_ACCUMULATOR		4294967296.0

typedef enum
{
	ANALOGUE,
	DIGITAL,
	AGGREGATED,
	MIXED
}MODE;


typedef struct
{
	int16_t DCcoupled;
	int16_t range;
	int16_t enabled;
}CHANNEL_SETTINGS;

typedef enum
{
	SIGGEN_NONE = 0,
	SIGGEN_FUNCTGEN = 1,
	SIGGEN_AWG = 2
} SIGGEN_TYPE;

typedef struct tTriggerDirections
{
	PS3000A_THRESHOLD_DIRECTION channelA;
	PS3000A_THRESHOLD_DIRECTION channelB;
	PS3000A_THRESHOLD_DIRECTION channelC;
	PS3000A_THRESHOLD_DIRECTION channelD;
	PS3000A_THRESHOLD_DIRECTION ext;
	PS3000A_THRESHOLD_DIRECTION aux;
}TRIGGER_DIRECTIONS;

typedef struct tPwq
{
	PS3000A_PWQ_CONDITIONS_V2 * conditions;
	int16_t nConditions;
	PS3000A_THRESHOLD_DIRECTION direction;
	uint32_t lower;
	uint32_t upper;
	PS3000A_PULSE_WIDTH_TYPE type;
}PWQ;

typedef struct
{
	int16_t					handle;
	int8_t					model[8];
	PS3000A_RANGE			firstRange ;
	PS3000A_RANGE			lastRange;
	int16_t					channelCount;
	int16_t					maxValue;
	int16_t					sigGen;
	int16_t					ETS;
	int32_t					AWGFileSize;
	CHANNEL_SETTINGS		channelSettings [PS3000A_MAX_CHANNELS];
	int16_t					digitalPorts;
}UNIT;

uint32_t	timebase = 8;
int16_t     oversample = 1;
BOOL		scaleVoltages = TRUE;

uint16_t inputRanges [PS3000A_MAX_RANGES] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};

BOOL     	g_ready = FALSE;
int32_t 	g_times [PS3000A_MAX_CHANNELS] = {0, 0, 0, 0};
int16_t     g_timeUnit;
int32_t     g_sampleCount;
uint32_t	g_startIndex;
int16_t		g_autoStopped;
int16_t		g_trig = 0;
uint32_t	g_trigAt = 0;

char BlockFile[20]		= "block.txt";
char DigiBlockFile[20]	= "digiBlock.txt";
char StreamFile[20]		= "stream.txt";

typedef struct tBufferInfo
{
	UNIT * unit;
	MODE mode;
	int16_t **driverBuffers;
	int16_t **appBuffers;
	int16_t **driverDigBuffers;
	int16_t **appDigBuffers;

} BUFFER_INFO;


/****************************************************************************
* Streaming callback
* Used by PS3000A data streaming collection calls, on receipt of data.
* Used to set global flags etc. checked by user routines
****************************************************************************/
void PREF4 callBackStreaming(	int16_t handle,
	int32_t		noOfSamples,
	uint32_t	startIndex,
	int16_t		overflow,
	uint32_t	triggerAt,
	int16_t		triggered,
	int16_t		autoStop,
	void		*pParameter)
{
	int32_t channel;
	BUFFER_INFO * bufferInfo = NULL;

	if (pParameter != NULL)
	{
		bufferInfo = (BUFFER_INFO *) pParameter;
	}

	// used for streaming
	g_sampleCount	= noOfSamples;
	g_startIndex	= startIndex;
	g_autoStopped	= autoStop;

	// flag to say done reading data
	g_ready = TRUE;

	// flags to show if & where a trigger has occurred
	g_trig = triggered;
	g_trigAt = triggerAt;

	if (bufferInfo != NULL && noOfSamples)
	{
		if (bufferInfo->mode == ANALOGUE)
		{
			for (channel = 0; channel < bufferInfo->unit->channelCount; channel++)
			{
				if (bufferInfo->unit->channelSettings[channel].enabled)
				{
					if (bufferInfo->appBuffers && bufferInfo->driverBuffers)
					{
						if (bufferInfo->appBuffers[channel * 2]  && bufferInfo->driverBuffers[channel * 2])
						{
							memcpy_s (&bufferInfo->appBuffers[channel * 2][startIndex], noOfSamples * sizeof(int16_t),
								&bufferInfo->driverBuffers[channel * 2][startIndex], noOfSamples * sizeof(int16_t));
						}
						if (bufferInfo->appBuffers[channel * 2 + 1] && bufferInfo->driverBuffers[channel * 2 + 1])
						{
							memcpy_s (&bufferInfo->appBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(int16_t),
								&bufferInfo->driverBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(int16_t));
						}
					}
				}
			}
		}
		else if (bufferInfo->mode == AGGREGATED)
		{
			for (channel = 0; channel < bufferInfo->unit->digitalPorts; channel++)
			{
				if (bufferInfo->appDigBuffers && bufferInfo->driverDigBuffers)
				{
					if (bufferInfo->appDigBuffers[channel * 2] && bufferInfo->driverDigBuffers[channel * 2])
					{
						memcpy_s (&bufferInfo->appDigBuffers[channel * 2][startIndex], noOfSamples * sizeof(int16_t),
							&bufferInfo->driverDigBuffers[channel * 2][startIndex], noOfSamples * sizeof(int16_t));
					}
					if (bufferInfo->appDigBuffers[channel * 2 + 1] && bufferInfo->driverDigBuffers[channel * 2 + 1])
					{
						memcpy_s (&bufferInfo->appDigBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(int16_t),
							&bufferInfo->driverDigBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(int16_t));
					}
				}
			}
		}
		else if (bufferInfo->mode == DIGITAL)
		{
			for (channel = 0; channel < bufferInfo->unit->digitalPorts; channel++)
			{
				if (bufferInfo->appDigBuffers && bufferInfo->driverDigBuffers)
				{
					if (bufferInfo->appDigBuffers[channel] && bufferInfo->driverDigBuffers[channel])
					{
						memcpy_s (&bufferInfo->appDigBuffers[channel][startIndex], noOfSamples * sizeof(int16_t),
							&bufferInfo->driverDigBuffers[channel][startIndex], noOfSamples * sizeof(int16_t));
					}
				}
			}
		}
	}
}

/****************************************************************************
* Block Callback
* used by PS3000A data block collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void PREF4 callBackBlock( int16_t handle, PICO_STATUS status, void * pParameter)
{
	if (status != PICO_CANCELLED)
	{
		g_ready = TRUE;
	}
}

/****************************************************************************
* setDefaults - restore default settings
****************************************************************************/
void setDefaults(UNIT * unit)
{
	int32_t i;
	PICO_STATUS status;

	status = ps3000aSetEts(unit->handle, PS3000A_ETS_OFF, 0, 0, NULL);	// Turn off ETS
	printf(status?"SetDefaults:ps3000aSetEts------ 0x%08lx \n":"", status);

	for (i = 0; i < unit->channelCount; i++) // reset channels to most recent settings
	{
		status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL)(PS3000A_CHANNEL_A + i),
			unit->channelSettings[PS3000A_CHANNEL_A + i].enabled,
			(PS3000A_COUPLING)unit->channelSettings[PS3000A_CHANNEL_A + i].DCcoupled,
			(PS3000A_RANGE)unit->channelSettings[PS3000A_CHANNEL_A + i].range, 0);

		printf(status?"SetDefaults:ps3000aSetChannel------ 0x%08lx \n":"", status);
	}
}

/****************************************************************************
* setDigitals - enable or disable Digital Channels
****************************************************************************/
PICO_STATUS setDigitals(UNIT *unit, int16_t state)
{
	PICO_STATUS status;

	int16_t logicLevel;
	float logicVoltage = 1.5;
	int16_t maxLogicVoltage = 5;

	int16_t timebase = 1;
	int16_t port;


	// Set logic threshold
	logicLevel =  (int16_t) ((logicVoltage / maxLogicVoltage) * PS3000A_MAX_LOGIC_LEVEL);

	// Enable Digital ports
	for (port = PS3000A_DIGITAL_PORT0; port <= PS3000A_DIGITAL_PORT1; port++)
	{
		status = ps3000aSetDigitalPort(unit->handle, (PS3000A_DIGITAL_PORT)port, state, logicLevel);
		printf(status?"SetDigitals:PS3000ASetDigitalPort(Port 0x%X) ------ 0x%08lx \n":"", port, status);
	}

	return status;
}

/****************************************************************************
* disableAnalogue - Disable Analogue Channels
****************************************************************************/
PICO_STATUS disableAnalogue(UNIT *unit)
{
	PICO_STATUS status;
	int16_t ch;

	// Turn off analogue channels, keeping settings
	for (ch = 0; ch < unit->channelCount; ch++)
	{
		status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL) ch, 0, (PS3000A_COUPLING) unit->channelSettings[ch].DCcoupled, 
			(PS3000A_RANGE) unit->channelSettings[ch].range, 0);
		
		if(status != PICO_OK)
		{
			printf("disableAnalogue:ps3000aSetChannel(channel %d) ------ 0x%08lx \n", ch, status);
		}
	}
	return status;
}

/****************************************************************************
* restoreAnalogueSettings - Restores Analogue Channel settings
****************************************************************************/
PICO_STATUS restoreAnalogueSettings(UNIT *unit)
{
	PICO_STATUS status;
	int16_t ch;

	// Turn on analogue channels using previous settings
	for (ch = 0; ch < unit->channelCount; ch++)
	{
		status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL) ch, unit->channelSettings[ch].enabled, (PS3000A_COUPLING) unit->channelSettings[ch].DCcoupled, 
			(PS3000A_RANGE) unit->channelSettings[ch].range, 0);

		if(status != PICO_OK)
		{
			printf("restoreAnalogueSettings:ps3000aSetChannel(channel %d) ------ 0x%08lx \n", ch, status);
		}
	}
	return status;
}


/****************************************************************************
* adc_to_mv
*
* Convert an 16-bit ADC count into millivolts
****************************************************************************/
int32_t adc_to_mv(int32_t raw, int32_t ch, UNIT * unit)
{
	return (raw * inputRanges[ch]) / unit->maxValue;
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
int16_t mv_to_adc(int16_t mv, int16_t ch, UNIT * unit)
{
	return (mv * unit->maxValue) / inputRanges[ch];
}

/****************************************************************************************
* changePowerSource - function to handle switches between +5V supply, and USB only power
* Only applies to ps34xxA/B units 
******************************************************************************************/
PICO_STATUS changePowerSource(int16_t handle, PICO_STATUS status)
{
	char ch;

	switch (status)
	{
		case PICO_POWER_SUPPLY_NOT_CONNECTED:			// User must acknowledge they want to power via USB

			do
			{
				printf("\n5V power supply not connected");
				printf("\nDo you want to run using USB only Y/N?\n");
				ch = toupper(_getch());
				if(ch == 'Y')
				{
					printf("\nPowering the unit via USB\n");
					status = ps3000aChangePowerSource(handle, PICO_POWER_SUPPLY_NOT_CONNECTED);		// Tell the driver that's ok

					if (status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
					{
						status = changePowerSource(handle, status);
					}
				}
			}
			while(ch != 'Y' && ch != 'N');

			printf(ch == 'N'?"Please use the +5V power supply to power this unit\n":"");
			break;

		case PICO_POWER_SUPPLY_CONNECTED:

			printf("\nUsing +5V power supply voltage\n");
			status = ps3000aChangePowerSource(handle, PICO_POWER_SUPPLY_CONNECTED);					// Tell the driver we are powered from +5V supply
			break;

		case PICO_POWER_SUPPLY_UNDERVOLTAGE:

			do
			{
				printf("\nUSB not supplying required voltage");
				printf("\nPlease plug in the +5V power supply\n");
				printf("\nHit any key to continue, or Esc to exit...\n");
				ch = _getch();

				if (ch == 0x1B)	// ESC key
				{
					exit(0);
				}
				else
				{
					status = ps3000aChangePowerSource(handle, PICO_POWER_SUPPLY_CONNECTED);		// Tell the driver that's ok
				}
			}
			while (status == PICO_POWER_SUPPLY_REQUEST_INVALID);
			break;
	}
	return status;
}

/****************************************************************************
* clearDataBuffers
*
* stops GetData writing values to memory that has been released
****************************************************************************/
PICO_STATUS clearDataBuffers(UNIT * unit)
{
	int32_t i;
	PICO_STATUS status;

	for (i = 0; i < unit->channelCount; i++) 
	{
		if((status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL) i, NULL, NULL, 0, 0, PS3000A_RATIO_MODE_NONE)) != PICO_OK)
		{
			printf("ClearDataBuffers:ps3000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
		}
	}


	for (i= 0; i < unit->digitalPorts; i++) 
	{
		if((status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL) (i + PS3000A_DIGITAL_PORT0), NULL, 0, 0, PS3000A_RATIO_MODE_NONE))!= PICO_OK)
		{
			printf("ClearDataBuffers:ps3000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n", i + PS3000A_DIGITAL_PORT0, status);
		}
	}

	return status;
}

/****************************************************************************
* blockDataHandler
* - Used by all block data routines
* - acquires data (user sets trigger mode before calling), displays 10 items
*   and saves all to block.txt
* Input :
* - unit : the unit to use.
* - text : the text to display before the display of data slice
* - offset : the offset into the data buffer to start the display's slice.
****************************************************************************/
void blockDataHandler(UNIT * unit, char * text, int32_t offset, MODE mode)
{
	int16_t retry;
	int16_t bit;

	uint16_t bitValue;
	uint16_t digiValue;
	
	int16_t * buffers[PS3000A_MAX_CHANNEL_BUFFERS];
	int16_t * digiBuffer[PS3000A_MAX_DIGITAL_PORTS];

	int32_t i, j;
	int32_t timeInterval;
	int32_t sampleCount = BUFFER_SIZE;
	int32_t maxSamples;
	int32_t timeIndisposed;

	FILE * fp = NULL;
	FILE * digiFp = NULL;
	
	PICO_STATUS status;

	if (mode == ANALOGUE || mode == MIXED)		// Analogue or (MSO Only) MIXED 
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			if(unit->channelSettings[i].enabled)
			{
				buffers[i * 2] = (int16_t*) calloc(sampleCount, sizeof(int16_t));
				buffers[i * 2 + 1] = (int16_t*) calloc(sampleCount, sizeof(int16_t));

				status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL)i, buffers[i * 2], buffers[i * 2 + 1], sampleCount, 0, PS3000A_RATIO_MODE_NONE);

				printf(status?"BlockDataHandler:ps3000aSetDataBuffers(channel %d) ------ 0x%08lx \n":"", i, status);
			}
		}
	}

	if (mode == DIGITAL || mode == MIXED)		// (MSO Only) Digital or MIXED
	{
		for (i= 0; i < unit->digitalPorts; i++) 
		{
			digiBuffer[i] = (int16_t*) calloc(sampleCount, sizeof(int16_t));

			status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL) (i + PS3000A_DIGITAL_PORT0), digiBuffer[i], sampleCount, 0, PS3000A_RATIO_MODE_NONE);

			printf(status?"BlockDataHandler:ps3000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n":"", i + PS3000A_DIGITAL_PORT0, status);
		}
	}

	/* Find the maximum number of samples and the time interval (in nanoseconds) */
	while (ps3000aGetTimebase(unit->handle, timebase, sampleCount, &timeInterval, oversample, &maxSamples, 0))
	{
		timebase++;
	}

	printf("\nTimebase: %lu  Sample interval: %ld ns \n", timebase, timeInterval);

	/* Start the device collecting, then wait for completion*/
	g_ready = FALSE;


	do
	{
		retry = 0;

		if((status = ps3000aRunBlock(unit->handle, 0, sampleCount, timebase, oversample, &timeIndisposed, 0, callBackBlock, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)       // 34xxA/B devices...+5V PSU connected or removed
			{
				status = changePowerSource(unit->handle, status);
				retry = 1;
			}
			else
			{
				printf("BlockDataHandler:ps3000aRunBlock ------ 0x%08lx \n", status);
				return;
			}
		}
	}
	while(retry);

	printf("Waiting for trigger...Press a key to abort\n");

	while (!g_ready && !_kbhit())
	{
		Sleep(0);
	}

	if(g_ready) 
	{
		status = ps3000aGetValues(unit->handle, 0, (uint32_t*) &sampleCount, 1, PS3000A_RATIO_MODE_NONE, 0, NULL);

		if(status != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
			{
				if (status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
				{
					changePowerSource(unit->handle, status);
				}
				else
				{
					printf("\nPower Source Changed. Data collection aborted.\n");
				}
			}
			else
			{
				printf("BlockDataHandler:ps3000aGetValues ------ 0x%08lx \n", status);
			}
		}
		else
		{
			/* Print out the first 10 readings, converting the readings to mV if required */
			printf("%s\n",text);

			if (mode == ANALOGUE || mode == MIXED)		// If we are capturing analogue or MIXED (analogue + digital) data
			{
				printf("Channels are in %s\n\n", ( scaleVoltages ) ? ("mV") : ("ADC Counts"));

				for (j = 0; j < unit->channelCount; j++) 
				{
					if (unit->channelSettings[j].enabled) 
					{
						printf("Channel %c:    ", 'A' + j);
					}
				}

				printf("\n");
			}

			if (mode == DIGITAL || mode == MIXED)	// If we are capturing digital or MIXED data 
			{
				printf("Digital\n");
			}

			printf("\n");

			for (i = offset; i < offset+10; i++) 
			{
				if (mode == ANALOGUE || mode == MIXED)	// If we are capturing analogue or MIXED data
				{
					for (j = 0; j < unit->channelCount; j++) 
					{
						if (unit->channelSettings[j].enabled) 
						{
							printf("  %d     ", scaleVoltages ? 
								adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit)	// If scaleVoltages, print mV value
								: buffers[j * 2][i]);																	// else print ADC Count
						}
					}
				}
				if (mode == DIGITAL || mode == MIXED)	// If we're doing digital or MIXED
				{
					digiValue = 0x00ff & digiBuffer[1][i];	// Mask Port 1 values to get lower 8 bits
					digiValue <<= 8;						// Shift by 8 bits to place in upper 8 bits of 16-bit word
					digiValue |= digiBuffer[0][i];			// Mask Port 0 values to get lower 8 bits and apply bitwise inclusive OR to combine with Port 1 values 
					printf("0x%04X", digiValue);

				}
				printf("\n");
			}

			if (mode == ANALOGUE || mode == MIXED)		// If we're doing analogue or MIXED
			{
				sampleCount = min(sampleCount, BUFFER_SIZE);

				fopen_s(&fp, BlockFile, "w");

				if (fp != NULL)
				{
					fprintf(fp, "Block Data log\n\n");
					fprintf(fp,"Results shown for each of the %d Channels are......\n",unit->channelCount);
					fprintf(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

					fprintf(fp, "Time  ");

					for (i = 0; i < unit->channelCount; i++) 
					{
						if (unit->channelSettings[i].enabled) 
						{
							fprintf(fp," Ch   Max ADC   Max mV   Min ADC   Min mV   ");
						}
					}

					fprintf(fp, "\n");

					for (i = 0; i < sampleCount; i++) 
					{
						fprintf(fp, "%d ", g_times[0] + (int32_t)(i * timeInterval));

						for (j = 0; j < unit->channelCount; j++) 
						{
							if (unit->channelSettings[j].enabled) 
							{
								fprintf(	fp,
									"Ch%C  %d = %+dmV, %d = %+dmV   ",
									'A' + j,
									buffers[j * 2][i],
									adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit),
									buffers[j * 2 + 1][i],
									adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit));
							}
						}
						fprintf(fp, "\n");
					}
				}
				else
				{
					printf(	"Cannot open the file %s for writing.\n"
						"Please ensure that you have permission to access.\n", BlockFile);
				}
			}

			// Output digital values to separate file
			if(mode == DIGITAL || mode == MIXED)
			{
				fopen_s(&digiFp, DigiBlockFile, "w");

				if (digiFp != NULL)
				{
					fprintf(digiFp, "Block Digital Data log\n");
					fprintf(digiFp, "Digital Channels will be in the order D15...D0\n");

					fprintf(digiFp, "\n");

					for (i = 0; i < sampleCount; i++) 
					{
						digiValue = 0x00ff & digiBuffer[1][i];	// Mask Port 1 values to get lower 8 bits
						digiValue <<= 8;						// Shift by 8 bits to place in upper 8 bits of 16-bit word
						digiValue |= digiBuffer[0][i];			// Mask Port 0 values to get lower 8 bits and apply bitwise inclusive OR to combine with Port 1 values

						// Output data in binary form
						for (bit = 0; bit < 16; bit++)
						{
							// Shift value (32768 - binary 1000 0000 0000 0000), AND with value to get 1 or 0 for channel
							// Order will be D15 to D8, then D7 to D0

							bitValue = (0x8000 >> bit) & digiValue? 1 : 0;
							fprintf(digiFp, "%d, ", bitValue);
						}

						fprintf(digiFp, "\n");

					}
				}
			}

		} 
	}
	else 
	{
		printf("\nData collection aborted.\n");
		_getch();
	}

	if ((status = ps3000aStop(unit->handle)) != PICO_OK)
	{
		printf("BlockDataHandler:ps3000aStop ------ 0x%08lx \n", status);
	}

	if (fp != NULL)
	{
		fclose(fp);
	}

	if (digiFp != NULL)
	{
		fclose(digiFp);
	}

	if (mode == ANALOGUE || mode == MIXED)		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			if(unit->channelSettings[i].enabled)
			{
				free(buffers[i * 2]);
				free(buffers[i * 2 + 1]);

			}
		}
	}

	if (mode == DIGITAL || mode == MIXED)		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->digitalPorts; i++) 
		{
			free(digiBuffer[i]);
		}
	}
	
	clearDataBuffers(unit);
}

/****************************************************************************
* Stream Data Handler
* - Used by the two stream data examples - untriggered and triggered
* Inputs:
* - unit - the unit to sample on
* - preTrigger - the number of samples in the pre-trigger phase 
*					(0 if no trigger has been set)
***************************************************************************/
void streamDataHandler(UNIT * unit, uint32_t preTrigger, MODE mode)
{
	int16_t autostop;
	int16_t retry = 0;
	int16_t powerChange = 0;

	uint16_t portValue, portValueOR, portValueAND;

	int32_t i, j;
	int32_t bit;
	int32_t index = 0;
	int32_t totalSamples;

	/*** THIS IS WHERE THE SAMPLING PARAMETERS ARE ***/

	uint32_t postTrigger;
	uint32_t sampleCount = 100000; /* Make sure buffer large enough */
	uint32_t sampleInterval;
	uint32_t downsampleRatio;
	uint32_t triggeredAt = 0;

	int16_t * buffers[PS3000A_MAX_CHANNEL_BUFFERS];
	int16_t * appBuffers[PS3000A_MAX_CHANNEL_BUFFERS];
	int16_t * digiBuffers[PS3000A_MAX_DIGITAL_PORTS];
	int16_t * appDigiBuffers[PS3000A_MAX_DIGITAL_PORTS];
	
	int channels = 2;
	int *voltageBuffers = (int*) calloc(sampleCount * channels, sizeof(int));
	//int *A_buf = (int*) calloc(sampleCount, sizeof(int));
	//int *B_buf = (int*) calloc(sampleCount, sizeof(int));
	//int *C_buf = (int*) calloc(sampleCount, sizeof(int));
	//int *D_buf = (int*) calloc(sampleCount, sizeof(int));
	
	PICO_STATUS status;

	PS3000A_TIME_UNITS timeUnits;
	PS3000A_RATIO_MODE ratioMode;

	BUFFER_INFO bufferInfo;
	FILE * fp = NULL;

	// UDP socket setup
	
	struct sockaddr_in si_other;
	int s, slen=sizeof(si_other);
	//char buf[4];
	char buf[50];
	
	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		exit(1);

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	if (inet_aton(SRV_IP, &si_other.sin_addr)==0) {
	  fprintf(stderr, "inet_aton() failed\n");
	  exit(1);
	}



	if (mode == ANALOGUE)		// Analogue - collect raw data
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			if(unit->channelSettings[i].enabled)
			{
				buffers[i * 2] = (int16_t*) calloc(sampleCount, sizeof(int16_t));
				buffers[i * 2 + 1] = (int16_t*) calloc(sampleCount, sizeof(int16_t));

				status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL)i, buffers[i * 2], buffers[i * 2 + 1], sampleCount, 0, PS3000A_RATIO_MODE_NONE);

				appBuffers[i * 2] = (int16_t*) calloc(sampleCount, sizeof(int16_t));
				appBuffers[i * 2 + 1] = (int16_t*) calloc(sampleCount, sizeof(int16_t));

				printf(status ? "StreamDataHandler:ps3000aSetDataBuffers(channel %ld) ------ 0x%08lx \n":"", i, status);
			}
		}

		downsampleRatio = 1;
		timeUnits = PS3000A_US;
		//sampleInterval = 10;
		//sampleInterval = 10;
		//sampleInterval = 10;
		
		//sampleInterval = 8;
		sampleInterval = 4;
		//sampleInterval = 2;
		//sampleInterval = 10;
		ratioMode = PS3000A_RATIO_MODE_NONE;
		postTrigger = 1000000;
		autostop = TRUE;
		// AUTOSTOP FALSE HERE
		autostop = FALSE;
	}

	bufferInfo.unit = unit;
	bufferInfo.mode = mode;	
	bufferInfo.driverBuffers = buffers;
	bufferInfo.appBuffers = appBuffers;
	bufferInfo.driverDigBuffers = digiBuffers;
	bufferInfo.appDigBuffers = appDigiBuffers;

	if (autostop)
	{
		printf("\nStreaming Data for %lu samples", postTrigger / downsampleRatio);

		if (preTrigger)	// we pass 0 for preTrigger if we're not setting up a trigger
		{
			printf(" after the trigger occurs\nNote: %lu Pre Trigger samples before Trigger arms\n\n", preTrigger / downsampleRatio);
		}
		else
		{
			printf("\n\n");
		}
	}
	else
	{
		printf("\nStreaming Data continually.\n\n");
	}

	g_autoStopped = FALSE;

	do
	{
		retry = 0;

		status = ps3000aRunStreaming(unit->handle, &sampleInterval, timeUnits, preTrigger, postTrigger, autostop, downsampleRatio,
			ratioMode, sampleCount);

		if(status != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
			{
				status = changePowerSource(unit->handle, status);
				retry = 1;
			}
			else
			{
				printf("StreamDataHandler:ps3000aRunStreaming ------ 0x%08lx \n", status);
				return;
			}
		}
	}
	while(retry);

	printf("Streaming data...Press a key to stop\n");

	//if (mode == ANALOGUE)
	//{
	//	fopen_s(&fp, StreamFile, "w");

	//	if (fp != NULL)
	//	{
	//		printf(fp,"For each of the %d Channels, results shown are....\n",unit->channelCount);
	//		printf(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

	//		for (i = 0; i < unit->channelCount; i++) 
	//		{
	//			if (unit->channelSettings[i].enabled) 
	//			{
	//				printf(fp,"Ch  Max ADC  Max mV  Min ADC  Min mV   ");
	//			}
	//		}
	//		fprintf(fp, "\n");
	//	}
	//}

	totalSamples = 0;

	while (!_kbhit() && !g_autoStopped)
	{
		// Register callback function with driver and check if data has been received
		g_ready = FALSE;

		status = ps3000aGetStreamingLatestValues(unit->handle, callBackStreaming, &bufferInfo);

		if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE) // 34xxA/B devices...+5V PSU connected or removed
		{
			if (status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
			{
				changePowerSource(unit->handle, status);
			}
			printf("\n\nPower Source Change");
			powerChange = 1;
		}

		index ++;

		if (g_ready && g_sampleCount > 0) /* can be ready and have no data, if autoStop has fired */
		{
			if (g_trig)
			{
				triggeredAt = totalSamples + g_trigAt;		// calculate where the trigger occurred in the total samples collected
			}

			totalSamples += g_sampleCount;

			printf("Collected %li samples, index = %lu, Total: %d samples \n", g_sampleCount, g_startIndex, totalSamples);

			if (g_trig)
			{
				printf("Trig. at index %lu", triggeredAt);	// show where trigger occurred
			}
			
			for (i = g_startIndex; i < (int32_t)(g_startIndex + g_sampleCount); i++) {
				voltageBuffers[i*channels] = adc_to_mv(appBuffers[0][i], unit->channelSettings[PS3000A_CHANNEL_A + 0].range, unit);
				voltageBuffers[i*channels+1] = adc_to_mv(appBuffers[2][i], unit->channelSettings[PS3000A_CHANNEL_A + 1].range, unit);
				//voltageBuffers[i*channels+2] = adc_to_mv(appBuffers[4][i], unit->channelSettings[PS3000A_CHANNEL_A + 2].range, unit);
			}
			//int packetsize = 1000, j = g_startIndex;	
			int packetsize = 100, j = g_startIndex;	
			for (; j <	(int32_t)(g_startIndex + g_sampleCount); j+=packetsize){
				int diff = 100;
				//printf("diff: %d\n", diff);
				//printf("startindex: %d, j: %d, g_sampleCount: %d, end: %d\n", g_startIndex, j, g_sampleCount, (int32_t)(g_sampleCount + g_startIndex));
				
				if((g_startIndex+g_sampleCount) < j+packetsize){
				   	diff = 100 - (j+packetsize - (g_startIndex+g_sampleCount));
					//printf("diff: %d\n", diff);
				}
				int p;
				for(p = 0; p < packetsize; p++){
					//printf("%d\n", voltageBuffers[j*channels + p]);
				}
				if (sendto(s, &(voltageBuffers[j*channels]), diff*channels*sizeof(int), 0, &si_other, slen)==-1){
					printf("Failed to send data, j = %d, g_startIndex = %d, diff = %d,  sampleCount = %d", j, g_startIndex, diff, g_sampleCount);
					close(s);
					exit(1);
				}
				
			}
			/*
			for (i = g_startIndex; i < (int32_t)(g_startIndex + g_sampleCount); i++) {

					// THIS IS WHERE THE DATA IS EXTRACTED FROM THE BUFFERS
					
					int AVmax = adc_to_mv(appBuffers[0 * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + 0].range, unit);
					int AVmin = adc_to_mv(appBuffers[0 * 2 + 1][i], unit->channelSettings[PS3000A_CHANNEL_A + 0].range, unit);
					int BVmax = adc_to_mv(appBuffers[1 * 2][i] * 10, unit->channelSettings[PS3000A_CHANNEL_A + 1].range, unit);
					int BVmin = adc_to_mv(appBuffers[1 * 2 + 1][i], unit->channelSettings[PS3000A_CHANNEL_A + 1].range, unit);

					int mixedValue = AVmax * BVmax / 1000;
					//printf("ARaw: %+dmV, BRaw: %+dmV\n", appBuffers[0][i], appBuffers[2][i]);
					//printf("AMax: %+dmV, AMin: %+dmV\n", AVmax, AVmin);
					//printf("A: %+dmV, B: %+dmV\n", AVmax, BVmax);
					//printf("Mixed: %d\n", mixedValue);
					//char channel = (char)('A' + 0);

					//printf("Ch%C: ", channel);
					//printf("Max: %+dmV, Min: %+dmV\n", Vmax, Vmin);

					//sprintf(buf, "Max: %+dmV, Min: %+dmV\n", Vmax, Vmin);
					//sprintf(buf, "Mixed: %d\n", mixedValue);
					//sprintf(buf, "%d, %d\n", mixedValue, i);

					//memcpy(buf, &channel, 1);
					//memcpy(buf+4, &Vmax, sizeof(int));
					//memcpy(buf+8, &Vmin, sizeof(int));
					//memcpy(buf, &mixedValue, sizeof(int));
					memcpy(buf, &AVmax, sizeof(int));
					memcpy(buf+4, &BVmax, sizeof(int));
					//memcpy(buf, &i, sizeof(int));
					//memcpy(buf, &BVmax, sizeof(int));
					
					if (sendto(s, buf, 2*sizeof(int), 0, &si_other, slen)==-1){
					//if (sendto(s, buf, 10, 0, &si_other, slen)==-1){
						printf("Failed to send data");
						close(s);
						exit(1);
					}
					
					//fprintf( fp, "%d\n", BVmax);
					//fprintf( fp, "%d\n", mixedValue);

					//fprintf(	fp,
					//	"Ch%C  %d = %+dmV, %d = %+dmV   ",
					//	(char)('A' + j),
					//	appBuffers[j * 2][i],
					//	adc_to_mv(appBuffers[j * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit),
					//	appBuffers[j * 2 + 1][i],
					//	adc_to_mv(appBuffers[j * 2 + 1][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit));
			}
			*/
		}
	}

	ps3000aStop(unit->handle);

	if (!g_autoStopped && !powerChange)  
	{
		printf("\nData collection aborted.\n");
		_getch();
	}

	if(fp != NULL) 
	{
		fclose(fp);	
	}

	if (mode == ANALOGUE)		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			if(unit->channelSettings[i].enabled)
			{
				free(buffers[i * 2]);
				free(buffers[i * 2 + 1]);

				free(appBuffers[i * 2]);
				free(appBuffers[i * 2 + 1]);
			}
		}
	}

	if (mode == DIGITAL) 		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->digitalPorts; i++) 
		{
			free(digiBuffers[i]);
			free(appDigiBuffers[i]);
		}

	}

	if (mode == AGGREGATED) 		// Only if we allocated these buffers
	{
		for (i = 0; i < unit->digitalPorts * 2; i++) 
		{
			free(digiBuffers[i]);
			free(appDigiBuffers[i]);
		}
	}

	close(s);
	clearDataBuffers(unit);
}



/****************************************************************************
* setTrigger
*
* - Used to call aall the functions required to set up triggering
*
***************************************************************************/
PICO_STATUS setTrigger(	UNIT * unit,

						struct tPS3000ATriggerChannelProperties * channelProperties,
							int16_t nChannelProperties,
							PS3000A_TRIGGER_CONDITIONS_V2 * triggerConditionsV2,
							int16_t nTriggerConditions,
							TRIGGER_DIRECTIONS * directions,

						struct tPwq * pwq,
							uint32_t delay,
							int16_t auxOutputEnabled,
							int32_t autoTriggerMs,
							PS3000A_DIGITAL_CHANNEL_DIRECTIONS * digitalDirections,
							int16_t nDigitalDirections)
{
	PICO_STATUS status;

	if ((status = ps3000aSetTriggerChannelProperties(unit->handle,
		channelProperties,
		nChannelProperties,
		auxOutputEnabled,
		autoTriggerMs)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerChannelProperties ------ Ox%08lx \n", status);
		return status;
	}

	if ((status = ps3000aSetTriggerChannelConditionsV2(unit->handle, triggerConditionsV2, nTriggerConditions)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerChannelConditions ------ 0x%08lx \n", status);
		return status;
	}

	if ((status = ps3000aSetTriggerChannelDirections(unit->handle,
		directions->channelA,
		directions->channelB,
		directions->channelC,
		directions->channelD,
		directions->ext,
		directions->aux)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerChannelDirections ------ 0x%08lx \n", status);
		return status;
	}

	if ((status = ps3000aSetTriggerDelay(unit->handle, delay)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerDelay ------ 0x%08lx \n", status);
		return status;
	}

	if((status = ps3000aSetPulseWidthQualifierV2(unit->handle, 
		pwq->conditions,
		pwq->nConditions, 
		pwq->direction,
		pwq->lower, 
		pwq->upper, 
		pwq->type)) != PICO_OK)
	{
		printf("SetTrigger:ps3000aSetPulseWidthQualifier ------ 0x%08lx \n", status);
		return status;
	}

	if (unit->digitalPorts)					
	{
		if((status = ps3000aSetTriggerDigitalPortProperties(unit->handle, digitalDirections, nDigitalDirections)) != PICO_OK) 
		{
			printf("SetTrigger:ps3000aSetTriggerDigitalPortProperties ------ 0x%08lx \n", status);
			return status;
		}
	}
	return status;
}

/****************************************************************************
* collectBlockImmediate
*  this function demonstrates how to collect a single block of data
*  from the unit (start collecting immediately)
****************************************************************************/
void collectBlockImmediate(UNIT * unit)
{
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&directions, 0, sizeof(struct tTriggerDirections));
	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect block immediate...\n");
	printf("Press a key to start\n");
	_getch();

	setDefaults(unit);

	/* Trigger disabled	*/
	setTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	blockDataHandler(unit, "\nFirst 10 readings:\n", 0, ANALOGUE);
}

/****************************************************************************
* collectBlockEts
*  this function demonstrates how to collect a block of
*  data using equivalent time sampling (ETS).
****************************************************************************/
void collectBlockEts(UNIT * unit)
{
	PICO_STATUS status;
	int32_t ets_sampletime;
	int16_t	triggerVoltage = mv_to_adc(1000,	unit->channelSettings[PS3000A_CHANNEL_A].range, unit);
	uint32_t delay = 0;
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_CHANNEL_A,
		PS3000A_LEVEL };

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE };


	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));
	directions.channelA = PS3000A_RISING;

	printf("Collect ETS block...\n");

	printf("Collects when value rises past %d", scaleVoltages? 
		adc_to_mv(sourceDetails.thresholdUpper,	unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count

	printf(scaleVoltages? "mV\n" : "ADC Counts\n");
	printf("Press a key to start...\n");
	_getch();

	setDefaults(unit);

	//Trigger enabled
	//Rising edge
	//Threshold = 1000mV
	status = setTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, delay, 0, 0, 0, 0);

	status = ps3000aSetEts(unit->handle, PS3000A_ETS_FAST, 20, 4, &ets_sampletime);
	printf("ETS Sample Time is: %ld\n", ets_sampletime);

	blockDataHandler(unit, "Ten readings after trigger:\n", BUFFER_SIZE / 10 - 5, ANALOGUE); // 10% of data is pre-trigger

	status = ps3000aSetEts(unit->handle, PS3000A_ETS_OFF, 0, 0, &ets_sampletime);
}

/****************************************************************************
* collectBlockTriggered
*  this function demonstrates how to collect a single block of data from the
*  unit, when a trigger event occurs.
****************************************************************************/
void collectBlockTriggered(UNIT * unit)
{

	int16_t triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS3000A_CHANNEL_A].range, unit);

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_CHANNEL_A,
		PS3000A_LEVEL};

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE};

	struct tPwq pulseWidth;

	struct tTriggerDirections directions = {	PS3000A_RISING,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE };

	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect block triggered...\n");
	printf("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count

	printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	printf("Press a key to start...\n");
	_getch();

	setDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	setTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	blockDataHandler(unit, "Ten readings after trigger:\n", 0, ANALOGUE);
}


/****************************************************************************
* Initialise unit' structure with Variant specific defaults
****************************************************************************/
void get_info(UNIT * unit)
{
	char description [11][25]= { "Driver Version",
		"USB Version",
		"Hardware Version",
		"Variant Info",
		"Serial",
		"Cal Date",
		"Kernel",
		"Digital H/W",
		"Analogue H/W",
		"Firmware 1",
		"Firmware 2"};

	int16_t i, r = 0;
	int8_t line [80];
	
	// Variables used for arbitrary waveform parameters
	int16_t			minArbitraryWaveformValue = 0;
	int16_t			maxArbitraryWaveformValue = 0;
	uint32_t		minArbitraryWaveformSize = 0;
	uint32_t		maxArbitraryWaveformSize = 0;

	PICO_STATUS status = PICO_OK;

	//Initialise default unit properties and change when required
	unit->sigGen		= SIGGEN_FUNCTGEN;
	unit->firstRange	= PS3000A_50MV;
	unit->lastRange		= PS3000A_20V;
	unit->channelCount	= DUAL_SCOPE;
	unit->ETS			= FALSE;
	unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
	unit->digitalPorts	= 0;

	if (unit->handle) 
	{
		for (i = 0; i < 11; i++) 
		{
			status = ps3000aGetUnitInfo(unit->handle, line, sizeof (line), &r, i);

			if (i == 3) 
			{
				memcpy(unit->model, line, strlen((char*)(line))+1);

				//If 4 (analogue) channel variant 
				if (line[1] == '4')
				{
					unit->channelCount = QUAD_SCOPE;
				}

				//If ETS mode is enabled
				if (strlen(line) == 8 || line[3] != '4')
				{
					unit->ETS	= TRUE;
				}

				
				if (line[4] != 'A')
				{
					// Set Signal generator type if the device is not an 'A' model
					unit->sigGen = SIGGEN_AWG;

					// 3000D and 3000D MSO models have a lower range of +/- 20mV
					if (line[4] == 'D')
					{
						unit->firstRange = PS3000A_20MV;
					}
				}

				// Check if MSO device
				if (strlen(line) >= 7)
				{
					line[4] = toupper(line[4]);
					line[5] = toupper(line[5]);
					line[6] = toupper(line[6]);

					if(strcmp(line + 4, "MSO") == 0 || strcmp(line + 5, "MSO") == 0 )
					{
						unit->digitalPorts = 2;
						unit->sigGen = SIGGEN_AWG;
					}
				}

				//If device has Arbitrary Waveform Generator, find the maximum AWG buffer size
				if (unit->sigGen == SIGGEN_AWG)
				{
					ps3000aSigGenArbitraryMinMaxValues(unit->handle, &minArbitraryWaveformValue, &maxArbitraryWaveformValue, &minArbitraryWaveformSize, &maxArbitraryWaveformSize);
					unit->AWGFileSize = maxArbitraryWaveformSize;
				}
			}
			printf("%s: %s\n", description[i], line);
		}		
	}
}

/****************************************************************************
* setVoltages
* Select input voltage ranges for channels
****************************************************************************/
void setVoltages(UNIT * unit)
{
	int32_t i, ch;
	int32_t count = 0;

	/* See what ranges are available... */
	for (i = unit->firstRange; i <= unit->lastRange; i++) 
	{
		printf("%d -> %d mV\n", i, inputRanges[i]);
	}

	do
	{
		/* Ask the user to select a range */
		printf("Specify voltage range (%d..%d)\n", unit->firstRange, unit->lastRange);
		printf("99 - switches channel off\n");

		for (ch = 0; ch < unit->channelCount; ch++) 
		{
			printf("\n");
			do 
			{
				printf("Channel %c: ", 'A' + ch);
				fflush(stdin);
				scanf_s("%hd", &unit->channelSettings[ch].range);
			} while (unit->channelSettings[ch].range != 99 && (unit->channelSettings[ch].range < unit->firstRange || unit->channelSettings[ch].range > unit->lastRange));

			if (unit->channelSettings[ch].range != 99) 
			{
				printf(" - %d mV\n", inputRanges[unit->channelSettings[ch].range]);
				unit->channelSettings[ch].enabled = TRUE;
				count++;
			} 
			else 
			{
				printf("Channel Switched off\n");
				unit->channelSettings[ch].enabled = FALSE;
				unit->channelSettings[ch].range = PS3000A_MAX_RANGES-1;
			}
		}
		printf(count == 0? "\n** At least 1 channel must be enabled **\n\n":"");
	}
	while(count == 0);	// must have at least one channel enabled

	setDefaults(unit);	// Put these changes into effect
}

/****************************************************************************
*
* Select timebase, set oversample to on and time units as nano seconds
*
****************************************************************************/
void setTimebase(UNIT unit)
{
	int32_t timeInterval = 0;
	int32_t maxSamples = 0;

	PICO_STATUS status = PICO_INVALID_TIMEBASE;

	printf("Specify desired timebase: ");
	fflush(stdin);
	scanf_s("%lud", &timebase);

	do
	{
		status = ps3000aGetTimebase(unit.handle, timebase, BUFFER_SIZE, &timeInterval, 1, &maxSamples, 0);
		
		if(status != PICO_OK)
		{
			timebase++;  // Increase timebase if the one specified can't be used. 
		}
	}
	while(status != PICO_OK);

	printf("Timebase used %lu = %ldns Sample Interval\n", timebase, timeInterval);
	oversample = TRUE;
}

/****************************************************************************
* Sets the signal generator
* - allows user to set frequency and waveform
* - allows for custom waveform (values -32768..32767) 
* - of up to 8192 samples long (PS3x04B & PS3x05B), 16384 samples long (PS3x06B)
*   or 32768 samples long (PS3207B and PS3X0X D MSO devices)
******************************************************************************/
void setSignalGenerator(UNIT unit)
{
	char ch;
	char fileName [128];

	int16_t waveform;
	int16_t choice;
	int16_t *arbitraryWaveform;

	int32_t waveformSize = 0;
	int32_t offset = 0;

	uint32_t pkpk = 4000000; // +/- 2V
	uint32_t delta = 0;

	double frequency = 1.0;

	FILE * fp = NULL;
	PICO_STATUS status;


	while (_kbhit())			// use up keypress
	{
		_getch();
	}

	do
	{
		printf("\nSignal Generator\n================\n");
		printf("0 - SINE         1 - SQUARE\n");
		printf("2 - TRIANGLE     3 - DC VOLTAGE\n");

		if(unit.sigGen == SIGGEN_AWG)
		{
			printf("4 - RAMP UP      5 - RAMP DOWN\n");
			printf("6 - SINC         7 - GAUSSIAN\n");
			printf("8 - HALF SINE    A - AWG WAVEFORM\n");
		}

		printf("F - SigGen Off\n\n");

		//ch = _getch();
		ch = '0';

		if (ch >= '0' && ch <='9')
		{
			choice = ch -'0';
		}
		else
		{
			ch = toupper(ch);
		}
	}
	while(unit.sigGen == SIGGEN_FUNCTGEN && ch != 'F' && (ch < '0' || ch > '3') || 
			unit.sigGen == SIGGEN_AWG && ch != 'A' && ch != 'F' && (ch < '0' || ch > '8')  );

	if(ch == 'F')				// If we're going to turn off siggen
	{
		printf("Signal generator Off\n");
		waveform = 8;		// DC Voltage
		pkpk = 0;			// 0V
		waveformSize = 0;
	}
	else
	{
		if (ch == 'A' && unit.sigGen == SIGGEN_AWG)		// Set the AWG
		{
			arbitraryWaveform = (int16_t*) malloc( unit.AWGFileSize * sizeof(int16_t));
			memset(arbitraryWaveform, 0, unit.AWGFileSize * sizeof(int16_t));

			waveformSize = 0;

			printf("Select a waveform file to load: ");
			scanf_s("%s", fileName, 128);

			if (fopen_s(&fp, fileName, "r") == 0) 
			{ // Having opened file, read in data - one number per line (max 8192 lines for PS3x04B & PS3x05B devices, 16384 for PS3x06B, 32768 for PS3207B), with values in (-32768..+32767)
				while (EOF != fscanf_s(fp, "%hi", (arbitraryWaveform + waveformSize))&& waveformSize++ < unit.AWGFileSize - 1);
				fclose(fp);
				printf("File successfully loaded\n");
			} 
			else 
			{
				printf("Invalid filename\n");
				return;
			}
		}
		else			// Set one of the built in waveforms
		{
			switch (choice)
			{
				case 0:
					waveform = PS3000A_SINE;
					break;

				case 1:
					waveform = PS3000A_SQUARE;
					break;

				case 2:
					waveform = PS3000A_TRIANGLE;
					break;

				case 3:
					waveform = PS3000A_DC_VOLTAGE;
					do 
					{
						printf("\nEnter offset in uV: (0 to 2000000)\n"); // Ask user to enter DC offset level;
						scanf_s("%lu", &offset);
					} while (offset < 0 || offset > 2000000);
					break;

				case 4:
					waveform = PS3000A_RAMP_UP;
					break;

				case 5:
					waveform = PS3000A_RAMP_DOWN;
					break;

				case 6:
					waveform = PS3000A_SINC;
					break;

				case 7:
					waveform = PS3000A_GAUSSIAN;
					break;

				case 8:
					waveform = PS3000A_HALF_SINE;
					break;

				default:
					waveform = PS3000A_SINE;
					break;
			}
		}

		if(waveform < 8 || (ch == 'A' && unit.sigGen == SIGGEN_AWG))				// Find out frequency if required
		{
			do 
			{
				//printf("\nEnter frequency in Hz: (1 to 1000000)\n"); // Ask user to enter signal frequency;
				//scanf_s("%lf", &frequency);
				printf("frequency set to 40KHz\n");
				frequency = 40000;
			} while (frequency <= 0 || frequency > 1000000);
		}

		if (waveformSize > 0)		
		{
			ps3000aSigGenFrequencyToPhase(unit.handle, frequency, PS3000A_SINGLE, waveformSize, &delta);

			status = ps3000aSetSigGenArbitrary(	unit.handle, 
				0,				// offset voltage
				pkpk,			// PkToPk in microvolts. Max = 4uV  +2v to -2V
				delta,			// start delta
				delta,			// stop delta
				0, 
				0, 
				arbitraryWaveform, 
				waveformSize, 
				(PS3000A_SWEEP_TYPE)0,
				(PS3000A_EXTRA_OPERATIONS)0, 
				PS3000A_SINGLE, 
				0, 
				0, 
				PS3000A_SIGGEN_RISING,
				PS3000A_SIGGEN_NONE, 
				0);

			printf(status? "\nps3000aSetSigGenArbitrary: Status Error 0x%x \n":"", (uint32_t) status);		// If status != 0, show the error
		} 
		else 
		{
			status = ps3000aSetSigGenBuiltInV2(unit.handle, 
				offset, 
				pkpk, 
				waveform, 
				(double) frequency, 
				(double) frequency, 
				0, 
				0, 
				(PS3000A_SWEEP_TYPE)0, 
				(PS3000A_EXTRA_OPERATIONS)0, 
				0, 
				0, 
				(PS3000A_SIGGEN_TRIG_TYPE)0, 
				(PS3000A_SIGGEN_TRIG_SOURCE)0, 
				0);

			printf(status?"\nps3000aSetSigGenBuiltIn: Status Error 0x%x \n":"", (uint32_t)status);		// If status != 0, show the error
		}
	}
}


/****************************************************************************
* collectStreamingImmediate
*  this function demonstrates how to collect a stream of data
*  from the unit (start collecting immediately)
***************************************************************************/
void collectStreamingImmediate(UNIT * unit)
{
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));

	setDefaults(unit);

	printf("Collect streaming...\n");
	printf("Data is written to disk file (stream.txt)\n");
	printf("Press a key to start\n");
	_getch();

	/* Trigger disabled	*/
	setTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	streamDataHandler(unit, 0, ANALOGUE);
}

/****************************************************************************
* collectStreamingTriggered
*  this function demonstrates how to collect a stream of data
*  from the unit (start collecting on trigger)
***************************************************************************/
void collectStreamingTriggered(UNIT * unit)
{
	int16_t triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS3000A_CHANNEL_A].range, unit); // ChannelInfo stores ADC counts
	struct tPwq pulseWidth;

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_CHANNEL_A,
		PS3000A_LEVEL };

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE };

	struct tTriggerDirections directions = {	PS3000A_RISING,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE };

	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect streaming triggered...\n");
	printf("Data is written to disk file (stream.txt)\n");
	printf("Press a key to start\n");
	_getch();
	
	setDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	setTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	streamDataHandler(unit, 10000, ANALOGUE);
}


/****************************************************************************
* displaySettings 
* Displays information about the user configurable settings in this example
* Parameters 
* - unit        pointer to the UNIT structure
*
* Returns       none
***************************************************************************/
void displaySettings(UNIT *unit)
{
	int32_t ch;
	int32_t voltage;

	printf("\n\nReadings will be scaled in (%s)\n\n", (scaleVoltages)? ("mV") : ("ADC counts"));

	for (ch = 0; ch < unit->channelCount; ch++)
	{
		if (!(unit->channelSettings[ch].enabled))
		{
			printf("Channel %c Voltage Range = Off\n", 'A' + ch);
		}
		else
		{
			voltage = inputRanges[unit->channelSettings[ch].range];
			printf("Channel %c Voltage Range = ", 'A' + ch);

			if (voltage < 1000)
			{
				printf("%dmV\n", voltage);
			}
			else
			{
				printf("%dV\n", voltage / 1000);
			}
		}
	}

	printf("\n");

	if(unit->digitalPorts > 0)
	{
		printf("Digital ports switched off.");
	}

	printf("\n");
}

/****************************************************************************
* openDevice 
* Parameters 
* - unit        pointer to the UNIT structure, where the handle will be stored
*
* Returns
* - PICO_STATUS to indicate success, or if an error occurred
***************************************************************************/
PICO_STATUS openDevice(UNIT *unit)
{
	int16_t value = 0;
	int32_t i;
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;
	PICO_STATUS status = ps3000aOpenUnit(&(unit->handle), NULL);

	if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT )
	{
		status = changePowerSource(unit->handle, PICO_POWER_SUPPLY_NOT_CONNECTED);
	}

	printf("Handle: %d\n", unit->handle);

	if (status != PICO_OK) 
	{
		printf("Unable to open device\n");
		printf("Error code : 0x%08lx\n", status);
		while(!_kbhit());
		exit(99); // exit program
	}

	printf("Device opened successfully, cycle %d\n\n", ++cycles);

	// setup devices
	get_info(unit);
	timebase = 1;

	ps3000aMaximumValue(unit->handle, &value);
	unit->maxValue = value;

	for ( i = 0; i < unit->channelCount; i++) 
	{
		unit->channelSettings[i].enabled = TRUE;
		unit->channelSettings[i].DCcoupled = TRUE;
		unit->channelSettings[i].range = PS3000A_5V;
	}

	memset(&directions, 0, sizeof(struct tTriggerDirections));
	memset(&pulseWidth, 0, sizeof(struct tPwq));

	setDefaults(unit);

	/* Trigger disabled	*/
	setTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	return status;
}


/****************************************************************************
* closeDevice 
****************************************************************************/
void closeDevice(UNIT *unit)
{
	ps3000aCloseUnit(unit->handle);
}

void calcDoppler(UNIT *unit)
{

	// Turn off two channels
	//setSignalGenerator(*unit);
	printf("Switching channel D off\n");
	unit->channelSettings[3].enabled = FALSE;
	unit->channelSettings[4].range = PS3000A_MAX_RANGES-1;

	// Set A to 10v, Set B to 50mV
	unit->channelSettings[0].range = 9; // +- 10V
	// For water
	unit->channelSettings[1].range = 1; // 20mV
	// For air
	//unit->channelSettings[1].range = 4; // 200mV
	unit->channelSettings[2].range = 4; // 200mV

	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&pulseWidth, 0, sizeof(struct tPwq));
	memset(&directions, 0, sizeof(struct tTriggerDirections));

	setDefaults(unit);

	printf("Collect streaming...\n");
	//printf("Press a key to start\n");
	//_getch();

	/* Trigger disabled	*/
	setTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	streamDataHandler(unit, 0, ANALOGUE);

}

/****************************************************************************
* New main, to capture data and compute doppler
***************************************************************************/
int32_t main(void)
{
	char ch;
	PICO_STATUS status;
	UNIT unit;

	printf("PS3000A Doppler Computer\n");
	printf("\nOpening the device...\n");

	status = openDevice(&unit);

	ch = '.';

	while (ch != 'X')
	{
		displaySettings(&unit);

		printf("\n\n");
		//printf("Please a key to begin:\n\n");
		//ch = toupper(_getch());
		//printf("\n\n");

		calcDoppler(&unit);
	}
	
	closeDevice(&unit);

	return 1;
}
