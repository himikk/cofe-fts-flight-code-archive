// telemetry.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <algorithm>
#include <vector>
#include <time.h>

using std::string;
using std::wstring;
using std::cin;
using std::cout;
using std::vector;

#define NUMBER_OF_BYTE 256
#define SPACEBALL_DELIMITER "SPACBALL"
#define SPACEBALL_DELIMITER_LEN 8

SYSTEMTIME timeinfo;
WCHAR holdertime[100];

int Initial(HANDLE * comport, wstring nComport, DWORD baudrate)
{
	LPCWSTR stuff= (wchar_t*)nComport.c_str();
	*comport = CreateFile(stuff, GENERIC_READ, 0, 0,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
	if(*comport == INVALID_HANDLE_VALUE)
	{
		if(GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			printf("serial port does not exist\n");
		}
		printf("Failed to open the com port\n");
		printf("Make sure the com port is not in use\n");
		return 2;
	}

	DCB dcbComParams = {0};
	if(!GetCommState(*comport, &dcbComParams))
	{
		printf("Some error has occured Happened!");
		return 1;
	}
	dcbComParams.BaudRate = baudrate;
	dcbComParams.ByteSize = 8;
	dcbComParams.StopBits = ONESTOPBIT;
	dcbComParams.Parity = NOPARITY;

	if(!SetCommState(*comport, &dcbComParams))
	{
		printf("Some Error has occured Happened!");
		return 1;
	}

	COMMTIMEOUTS timeouts = {0};

	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	if(!SetCommTimeouts(*comport, &timeouts))
	{
		printf("Error with Time happened!");
		return 1;
	}

	return 0;
}

void ComportRead(HANDLE comport, char* buff, DWORD *numbytes)
{
	*numbytes = 0;
	if(!ReadFile(comport, buff, NUMBER_OF_BYTE,numbytes, NULL))
		printf("Apparently an error occured!\n");
}

void WriteToFile(vector<BYTE> *pdata, wstring & rootname)
{
	DWORD numbytes;
	GetLocalTime(&timeinfo);
	wstring extension = L".spaceball";
	swprintf_s(holdertime, L"%02d%02d%02d%ls", timeinfo.wHour, timeinfo.wMinute, timeinfo.wSecond,extension.c_str());
	wstring filename = rootname + wstring(holdertime);
	HANDLE fileholder;
	LPCWSTR stuff= (wchar_t*)filename.c_str();
	fileholder = CreateFile(stuff, GENERIC_WRITE , FILE_SHARE_READ, NULL,CREATE_NEW, 0,NULL);

	int datasize = (int)pdata->size();
	//int datasize = (int)pdata->size()- 8; I assume subtracting 8 was to trim off delimiters. Dunno!
	vector<BYTE>::iterator it = pdata ->begin();
	char * buffer = (char *)calloc(datasize,sizeof(BYTE));
	for( int i = 0; i < datasize; ++i)
	{
		buffer[i]=*it;
		pdata->erase(it);
	}

	if(!WriteFile(fileholder, buffer,(DWORD)datasize,&numbytes,NULL))
		printf("Error Writing to file?");
	else
		printf("Wrote to file just fine.");
	free(buffer);
	CloseHandle(fileholder);
}

void readUntilDelimiter( HANDLE comport, vector<BYTE> &data, vector<BYTE> &postSepData ) {
	/* Reads data from comport and puts it into data, until it hits SPACEBALL_DELIMITER.
	   Puts the delimiter and all following data into postSepData.
	   e.g. data = abc, postSepData = <empty> upon entry
	        <reads "defghijDELIMITERklmnop" from comport>
			==> data = abcdefghij, postSepData = DELIMITERklmnop upon return.
	   THIS IS IMPERFECT. If two delimiters appear in the same comport read,
	    the second will be ignored. That is,
		    data = abc, postSepData = <empty> upon entry
			<reads "defgDELIMITERhijklmnDELIMITERop" from comport>
			==> data = abcdefg, postSepData = DELIMITERhijklmnDELIMITERop upon return.
	   But spaceball files are usually bigger than 256 bytes. Just... just noting
	    that there's an edge-case hole in this function.
	*/
	char buffer[NUMBER_OF_BYTE+1] = {0}; // The buffer we read into from the comport.
	DWORD numbytes = 0; // Number of bytes read from the comport.
	int matched = 0; // Number of characters of the delimiter we've matched.
	int i, j;
	while (1) {
		// Read from the comport into the buffer. Go through the buffer,
		//  putting bytes into the data vector (unless they match the delimiter).
		// When we find the delimiter, we put it and all following buffered data
		//  into the postSepData vector.
		ComportRead(comport, buffer, &numbytes);
		for (i=0; i<int(numbytes); i++) {
			if (buffer[i] == SPACEBALL_DELIMITER[matched]) {
				matched += 1;
				if (matched == SPACEBALL_DELIMITER_LEN) {
					// We've found our delimiter. Put it and following data into postSepData.
					for (j=0; j<8; j++)
						postSepData.push_back((BYTE)(SPACEBALL_DELIMITER[j]));
					for (j=i+1; j<int(numbytes); j++) 
						postSepData.push_back((BYTE)buffer[j]);
					//printf("Found delimiter. Data size: %d -- post-sep data size: %d\n", data.size(), postSepData.size());
					return;
				}
			}
			else {
				if (matched > 0) {
					for (j=0; j<matched; j++)
						data.push_back((BYTE)(SPACEBALL_DELIMITER[j]));
					matched = 0;
				}
				data.push_back((BYTE)buffer[i]);
			}
		}
	}
}

wstring charArrayToWstring(char cs[]) {
	string s = cs;
	wchar_t *buf = new wchar_t[ s.size() ];
	size_t num_chars = mbstowcs( buf, s.c_str(), s.size() );
	wstring ws( buf, num_chars );
	delete[] buf;
	return ws;
}

int main(int argc, char* argv[])
{
	// argv = (name of program, target folder, name of comport, baud rate)
	HANDLE comport;
	wstring foldername = charArrayToWstring(argv[1]);
	string nComport = "\\\\.\\";
	nComport += argv[2];
	DWORD baudrate = (DWORD)atoi(argv[3]);
	printf("Folder name: %s (%d chars)\n", foldername.c_str(), foldername.size());
	printf("Attempting to open: %s\n", nComport.c_str());
	wstring portname(nComport.length(),L'');
	std::copy(nComport.begin(), nComport.end(),portname.begin());
	int test;
	vector<BYTE> data;
	vector<BYTE> postSepData;

	printf("Checking/creating directory to write data.");
	if(CreateDirectory(foldername.c_str(), NULL) == (0 | ERROR_ALREADY_EXISTS))
	{
		printf("Directory creation failed with error %d.\n", GetLastError());
	}
	GetLocalTime(&timeinfo);
	swprintf_s(holdertime, L"\\%04d%02d%02d\\", timeinfo.wYear, timeinfo.wMonth, timeinfo.wDay);
	foldername += wstring(holdertime);
	if(CreateDirectory(foldername.c_str(), NULL) == (0 | ERROR_ALREADY_EXISTS))
	{
			printf("Directory creation failed with error %d.\n", GetLastError());
	}
	// All right. We have our directory. Now we just set up the comport...
	do
	{
		test = Initial(&comport, portname, baudrate);
		// (I guess that sets it up.)
		if(test == 0)
		{
			printf("\n\nsuccesfully opened: %s\n",nComport.c_str());
			bool first = true; // The first batch of data we read is likely to be a partial file.
			    // For that reason, we just toss it away. Partial files are useless to us.
			do {
				readUntilDelimiter(comport, data, postSepData);
				// Now data holds everything that came before the next delimiter, and postSepData
				//  everything that came after.
				if (first)
					first = false;
				else {
					//printf("data size at write: %d\n", data.size());
					WriteToFile(&data, foldername);
				}

				// Wipe the data we just wrote (or tossed, if we're on the first pass)
				//  and put the post-prefix data in that vector, so that the next
				//  readUntilDelimiter call will push onto the end of it.
				data.clear();
				data.insert(data.end(), postSepData.begin(), postSepData.end());
				//printf("postSepData size: %d -- data size: %d\n", postSepData.size(), data.size());
				postSepData.clear();
			}while(!_kbhit());
		}
	}while(test != 0 && !_kbhit());
	CloseHandle(comport);
//	cin.get();
	return 0;
}



