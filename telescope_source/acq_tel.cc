// acq_tel_07_08_10.cc
//
//  For DaqBoard3000USB Series
//
//	*** If your DaqBoard3000USB Series Device
//	*** has a name other than DaqBoard3000USB - Set the 'devName'
//  *** variable below to the apropriate name
//  Version of the code from 5-19-2010. Ryan came in and added an error capture
// to allow the code to self-restart
// removed some crap, update the name for clarity. Tested working 7/8/10 prm
// corrected typo of 131 where should have been 13.
// This version has the correction to the channel number--- critical for differential operations with more than
// 8 a/d inputs!!!!! 
// was used for flight- this note added PRM August 27, 2012

#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "..\..\..\include\daqx.h"
//#include "..\..\..\include\cbw.h"

#define STARTSOURCE	DatsImmediate	
#define STOPSOURCE	DatsScanCount
#define PREWRITE	0
#define CHANCOUNT	21
#define SCANS		12800
#define RATE		5	//you should use a low rate to allow for collection of pulses

using namespace std;

//Globsl Error Handler
bool ryanError = false;

//Global variables for use by threads
char comBuff[8];
DWORD demodData[3];
DWORD bchan[3]={2,256,257};
HANDLE hPipe;
const int numThreads = 2;
DWORD BytesRead;
DWORD length;
OVERLAPPED op;

//MCC DIO variables
//WORD DataValue[4];
//int PortNum[4];
//int Direction;
//int BoardNum = 0;
//int ULStat = 0;
//char ErrMessage[80];
//float RevLevel = (float)CURRENTREVNUM;
DWORD retvalue[3];	//IOtech DIO

//Global variables for sending demodulated data
DWORD demodBuff[256];
DWORD IDat = 0; DWORD QDat = 0; DWORD UDat = 0;
int sendDat;

//Thread for writing data
DWORD WINAPI wThread(LPVOID arg ){
	int i;
	float negative = -1;
	while(strncmp(comBuff,"stop",4) != 0){
		if(sendDat >= 15){
			IDat = 0; QDat = 0; UDat = 0;
			for(i = 0; i < 128; i++){
				IDat = IDat + demodBuff[i];
				QDat = QDat + (demodBuff[i] * (pow(negative, (i/16))));
				UDat = UDat + (demodBuff[i] * (pow(negative, ((i + 8)/16))));
			}
			demodData[0] = IDat; demodData[1] = QDat; demodData[2] = UDat;
			if(WriteFileEx(hPipe,&demodData,sizeof(demodData),&op,NULL) == 0){
				printf("Write failed with error %d.\n", GetLastError());
			}
			sendDat = 0;
			Sleep(20);
		}
	}
	return 0;
}

//Thread for reading data
DWORD WINAPI rThread(LPVOID arg ){
	while(strncmp(comBuff,"exit",4) != 0){
		Sleep(100);
		if(ReadFileEx(hPipe,&comBuff,sizeof(comBuff),&op,NULL) == 0){
			printf("Read failed with error %d.\n", GetLastError());
		}
	}
	return 0;
}

//error handler reacts to timeouts
void WINAPI ErrorHandler(DaqHandleT handle, DaqError error_code){
	if (error_code == DerrTimeout){
		printf("\nTransfer Timed Out..");		
	}
	else{
		printf("\nError %d, let's ignore it.", error_code);
		ryanError = true;
		//daqDefaultErrorHandler(handle, error_code);
	}
}

void main(int argc, char* argv[])
{	
	printf("ACQ_TEL compiled 7/8/2010 by prm\n");
	printf("Version includes Ryan's error capture from May 19\n");
    printf("Has MCC DIO stuff commented out\n");
    printf("set up for 256 samples/revolution and 1000 revolutions per file\n");
    printf("Corrected channel number typo from Russell\n");
    printf("Added back two redundant encoder channels to test initialization solution\n");
	
	//used to connect to iotech device
	char* devName;
	DaqHandleT handle;

	//used to connect to serial
	HANDLE hThread[numThreads];
	
	//used for file creation
	FILE * pFile;
	time_t rawtime;
	struct tm * timeinfo;
	char	filetime[9];
	char	filename[80];
	char	foldertime[9];
	char	foldername[80];
	int result;

	//digital IO 
	DWORD config;
	DWORD config_dum;
	
	//data buffers
	WORD	buffer[SCANS*CHANCOUNT];
	int sleep = 100;
	if (argc > 1)
		sleep = atoi(argv[1]);
	printf("The current sleeping: %d",sleep);

	// Can't do this, why?: WORD	buffer_dum[SCANS*CHANCOUNT];
		
	// Scan List Setup
	//normal working DWORD    channels[CHANCOUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 16, 9, 10, 11, 12, 13, 14, 15, 0, 0, 0, 1, 2};
	DWORD    channels_dum[CHANCOUNT + 4] = {0, 1, 2, 3, 4, 5, 6, 7,
											256, 257, 258, 259, 260, 261, 262, 263,
											0, 0, 1, 1, 2, 2, 0, 1, 2};
	DaqAdcGain  gains_dum[CHANCOUNT + 4] = {DgainDbd3kX1, DgainDbd3kX1, 
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1, 
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1};
									

	DWORD       flags_dum[CHANCOUNT + 4] = {DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									/*DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,*/
									DafCtr32EnhLow, DafCtr32EnhHigh,
									DafCtr32EnhLow, DafCtr32EnhHigh,
									DafCtr32EnhLow, DafCtr32EnhHigh,
									DafP2Local8, DafP2Local8, 
									DafP2Local8};
	
	
	//used to monitor scan
	DWORD active, retCount,tail, head = 0;
	DWORD i,j;
	DWORD bufPos = 0;
	FLOAT tickTime = 20.83E-9f; // Used for Period scaling.	
	DWORD dataPos = 0;
	DWORD NumOfReads = 0;
	DWORD ReadsPerRev = 256;
	DWORD RevolutionsPerFile = 1000;
	PWORD sample=0;

	devName = "DaqBoard3031USB";//any USB 3031 iotech card
// here we'll set the scan up, then redo the defs. this is to initialize the two redundant counters without having to store their data.....
			printf( "Attempting to Connect with (for the initialization only) %s\n", devName );		
			//attempt to open device
			handle = daqOpen(devName);
			DaqDevInfoT devInfo;
			daqGetDeviceInfo(handle, &devInfo);
			printf( "Hardware type: %d\n Subtype: %d\n", devInfo.DeviceType, devInfo.DeviceSubType );
			printf("before daqadcrd \n");
			DaqAdcExpSubType chanSubType;
			DWORD chanType = daqGetChannelType(handle,bchan[0],1, &chanSubType);
			printf( "channel type/subtype: %d/%d\n ", chanType, chanSubType );
			//printf("before daqadcrd \n");
			//daqAdcRd(handle,bchan[1],sample,DgainDbd3kX1,DafBipolar);
			//printf("after daqadcrd \n");
			//set error handler
			//daqAdcExpSetBank(handle,bchan[0],DmctpDaq);
			//daqSetErrorHandler(handle, &ErrorHandler);
			// set # of scans to perform and scan mode
			daqAdcSetAcq(handle, DaamInfinitePost, 0, SCANS);	
		    //dummy Scan settings
			daqAdcSetScan(handle, channels_dum, gains_dum, flags_dum, CHANCOUNT +4);
			//set scan rate
			daqAdcSetFreq(handle, RATE);
			//set clock source
			daqAdcSetClockSource(handle, DacsExternalTTL);
			daqAdcSetClockSource(handle, DacsRisingEdge);

			daqSetOption(handle, channels_dum[CHANCOUNT-5], DcofChannel, DcotCounterEnhMapChannel, DcovCounterEnhMap_Channel_2);
			daqSetOption(handle, channels_dum[CHANCOUNT-3], DcofChannel, DcotCounterEnhMapChannel, DcovCounterEnhMap_Channel_2);

				//Setup Channel 16-20 lows
			daqSetOption(handle, channels_dum[CHANCOUNT-5], DcofChannel, DcotCounterEnhMeasurementMode,
			DcovCounterEnhMode_Encoder | DcovCounterEnhModeMask_32Bit | DcovCounterEnhEncoder_X4 | DcovCounterEnhTriggerAfterStable |
			DcovCounterEnhDebounce500ns | DcovCounterEnhRisingEdge |DcovCounterEnhEncoder_LatchOnSOS | DcovCounterEnhEncoder_ClearOnZ_On); 

			daqSetOption(handle, channels_dum[CHANCOUNT-3], DcofChannel, DcotCounterEnhMeasurementMode, 
				DcovCounterEnhMode_Encoder |DcovCounterEnhModeMask_32Bit | DcovCounterEnhEncoder_X4 | DcovCounterEnhDebounce500ns);
				
			daqSetOption(handle, channels_dum[CHANCOUNT-1], DcofChannel, DcotCounterEnhMeasurementMode, 
				DcovCounterEnhMode_Encoder |DcovCounterEnhModeMask_32Bit | DcovCounterEnhEncoder_X4 | DcovCounterEnhDebounce500ns);

			//Set buffer location, size and flag settings
			daqAdcTransferSetBuffer(handle, buffer, SCANS, DatmDriverBuf|DatmUpdateBlock|DatmCycleOn|DatmIgnoreOverruns);	
			//Set to Trigger on software trigger
			daqSetTriggerEvent(handle, STARTSOURCE, DetsRisingEdge, channels_dum[CHANCOUNT-5],
							   gains_dum[CHANCOUNT-5], flags_dum[CHANCOUNT-5],
			DaqTypeAnalogLocal, 0, 0, DaqStartEvent);

			//set port A, B, and C as inputs for digital I/O
			daqIOGet8255Conf(handle, 1 ,1, 1, 1, &config);
			//write settings to internal register
			daqIOWrite(handle, DiodtLocal8255, Diodp8255IR, 0, DioepP2, config);
			//begin dummy data acquisition setup
			printf("\nScanning...\n");
			daqAdcTransferStart(handle);
			daqAdcArm(handle);
			daqAdcDisarm(handle);
//now reset the parameters to the nominal ones and set up again inside the loop below:
	DWORD channels[CHANCOUNT] = {0, 1, 2, 3, 4, 5, 6, 7,
								 256, 257, 258, 259, 260, 261, 262, 263,
					  			 0, 0, 0, 1, 2};
	DaqAdcGain gains[CHANCOUNT] = {DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1, 
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1, DgainDbd3kX1,
									DgainDbd3kX1};
									

	DWORD       flags[CHANCOUNT] = {DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									DafBipolar|DafDifferential|DafSettle1us, DafBipolar|DafDifferential|DafSettle1us,
									//DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									//DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									//DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									//DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									/*DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,
									DafBipolar|DafSettle1us, DafBipolar|DafSettle1us,*/
									DafCtr32EnhLow, DafCtr32EnhHigh,
									DafP2Local8, DafP2Local8, 
									DafP2Local8};



	while (!_kbhit()){
		ryanError = false;
		if (*devName != NULL)
		{
			printf( "Attempting to Connect with %s\n", devName );		
			//attempt to open device
			handle = daqOpen(devName);
			//daqAdcExpSetBank(handle,bchan[0],DmctpDaq);

			if (handle > -1)//a return of -1 indicates failure
			{		
				printf("Setting up scan...\n");

				//set error handler
				daqSetErrorHandler(handle, &ErrorHandler);
				//daqSetErrorHandler(handle, 0);
				// set # of scans to perform and scan mode
				daqAdcSetAcq(handle, DaamInfinitePost, 0, SCANS);	

				//Scan settings
				daqAdcSetScan(handle, channels, gains, flags, CHANCOUNT);  
						
				//set scan rate
				daqAdcSetFreq(handle, RATE);
				
				//set clock source
				daqAdcSetClockSource(handle, DacsExternalTTL);
				//daqAdcSetClockSource(handle, DacsRisingEdge);
				
				daqSetOption(handle, channels[CHANCOUNT-5], DcofChannel, DcotCounterEnhMapChannel, DcovCounterEnhMap_Channel_2);

				//Setup Channel 16
				daqSetOption(handle, channels[CHANCOUNT-5], DcofChannel, DcotCounterEnhMeasurementMode,
					DcovCounterEnhMode_Encoder | DcovCounterEnhModeMask_32Bit | DcovCounterEnhEncoder_X4 | DcovCounterEnhTriggerAfterStable |
					DcovCounterEnhDebounce500ns | DcovCounterEnhRisingEdge |DcovCounterEnhEncoder_LatchOnSOS | DcovCounterEnhEncoder_ClearOnZ_On); 
				//	DcovCounterEnhEncoder_LatchOnSOS);	

				//Set buffer location, size and flag settings DatmUpdateBlock
				daqAdcTransferSetBuffer(handle, buffer, SCANS, DatmUserBuf|DatmUpdateBlock|DatmCycleOn | DatmIgnoreOverruns);	
				
				//Set to Trigger on software trigger
				daqSetTriggerEvent(handle, STARTSOURCE, DetsRisingEdge, channels[CHANCOUNT-5],
								   gains[CHANCOUNT-5], flags[CHANCOUNT-5],
		    						DaqTypeAnalogLocal, 0, 0, DaqStartEvent);

				//Set to Stop when the requested number of scans is completed
				//daqSetTriggerEvent(handle, STOPSOURCE, DetsRisingEdge, channels[CHANCOUNT-5], gains[CHANCOUNT-5] flags[CHANCOUNT-5],
									//DaqTypeAnalogLocal, 0, 0, DaqStopEvent);

				//set port A, B, and C as inputs for digital I/O
				daqIOGet8255Conf(handle, 1 ,1, 1, 1, &config);

				//write settings to internal register
				daqIOWrite(handle, DiodtLocal8255, Diodp8255IR, 0, DioepP2, config);

				//Opening binary file to save important data
				if(CreateDirectory("data", NULL) == (0 | ERROR_ALREADY_EXISTS)){
					printf("Directory creation failed with error %d.\n", GetLastError());
				}
				time ( &rawtime );
				timeinfo = localtime ( &rawtime );
				strftime(foldertime,9,"%Y%m%d",timeinfo);
				strcpy(foldername,"data\\");
				strcat(foldername,foldertime);
				strcat(foldername, "\\");
				if(CreateDirectory(foldername, NULL) == (0 | ERROR_ALREADY_EXISTS)){
					printf("Directory creation failed with error %d.\n", GetLastError());
				}
				strftime(filetime,9,"%H%M%S00",timeinfo);
				strcpy(filename,foldername);
				strcat(filename,filetime);
				strcat(filename,".dat");
				pFile = fopen(filename,"ab+");
				//daqAdcSetDiskFile(handle, filename, DaomCreateFile, PREWRITE);

				//Starting Pipe I/O threads
				dataPos = 0; sendDat = 0;
				//hThread[0] = CreateThread(NULL, 0, wThread, NULL, 0, NULL );
				//hThread[1] = CreateThread(NULL, 0, rThread, NULL, 0, NULL );

				//begin data acquisition
				printf("\nScanning...\n");
				daqAdcTransferStart(handle);
				daqAdcArm(handle);
				
				//begin data acquisition
				do{	
					if(ryanError)
						break;
					try{
						//Using 100 for mini computers
						Sleep(sleep);

						daqAdcTransferGetStat(handle, &active, &retCount);	
//						cout << "What is this: "<< retCount << endl;
						//transfer data (voltage readings) into computer memory and halt acquisition when done
//						daqAdcTransferBufData(handle, buffer, SCANS, DabtmOldest, &retCount);
//						printf("The amount of data is: %d\r",retCount);
								//add some lines to get measurment computing DIO value and spit to screen
								//Measurement computing data
								
								//printf("\nBefore mcc read\n");
								//cbDIn(BoardNum, PortNum[0], &DataValue[0]);
								//cbDIn(BoardNum, PortNum[1], &DataValue[1]);
								//cbDIn(BoardNum, PortNum[2], &DataValue[2]);
								//cbDIn(BoardNum, PortNum[3], &DataValue[3]);
								//printf("\nafter read\n");
								//printf("test %d. \n",DataValue[0]);
								//printf("test %d. \n",DataValue[1]);
								//printf("test %d. \n",DataValue[2]);
								//printf("test %d. \n",DataValue[3]);
						tail = head;		
						head = (retCount % SCANS);
						//tail = ( retCount < SCANS) ? 0 : (head+1) % SCANS;
						for(j = tail ; j != head ; j = (j+1) % SCANS){

							NumOfReads++;

							if( (NumOfReads/ReadsPerRev) >= RevolutionsPerFile ){
								fclose (pFile);
								result = rename(foldername,filename);
								time ( &rawtime );
								timeinfo = localtime ( &rawtime );								
								strftime(foldertime,9,"%Y%m%d",timeinfo);
								strcpy(foldername,"data\\");
								strcat(foldername,foldertime);
								strcat(foldername, "\\");
								if(CreateDirectory(foldername, NULL) == (0 | ERROR_ALREADY_EXISTS)){
									printf("Directory creation failed with error %d.\n", GetLastError());
								}
								strftime(filetime,9,"%H%M%S00",timeinfo);
								strcpy(filename,foldername);
								strcat(filename,filetime);
								strcat(filename,".dat");
								strcpy(foldername,filename);
								strcat(foldername,".tmp");
								pFile = fopen(foldername,"ab+");
								NumOfReads = 0;

								//printf("Read failed with error %d.\n", GetLastError());
							}

							fwrite((buffer + (j * CHANCOUNT)), sizeof(WORD), CHANCOUNT, pFile);

							/*demodBuff[dataPos] = buffer[(CHANCOUNT * j) + 1];
							if(dataPos == 256){dataPos = 0; sendDat++;}*/
						}
					}
					catch(...)
					{
						cout << "exception caught!" << endl;
					}
				}
				while (!_kbhit());
				//while (( active & DaafAcqActive ) && (strncmp(comBuff,"stop",4) != 0) & (!_kbhit()));
				printf("\nScan Completed\n\n");

				//Wait for exit command
				//WaitForMultipleObjects(numThreads,hThread,TRUE,INFINITE);

				//close binary file
				fclose (pFile);

				//Disarm when completed
				daqAdcDisarm(handle);

				//close device connections
				daqClose(handle);
				printf("Acquistion Terminated!\n");
				
				//Closing Pipe
				if(CloseHandle(hPipe) == 0) 
					printf("Pipe Closing Failed!\n");
				else
					printf("Pipe Closed!\n");
				
				
			}
			else
				printf("Could not connect to device\n");
		}
		else
			printf("No device name specified\n");
	}
	//user exit command
	printf("\nPress any key to exit.\n");
	getch();
}