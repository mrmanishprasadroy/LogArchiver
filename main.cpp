/* ========================================================================== */
/*                                                                            */
/*   KacoLog.c                                                                */
/*   (c) 2012 Paul Lunter                                                     */
/*                                                                            */
/*   Description                                                              */
/*   Logging of received messages (string) sent via RS232 interface from      */
/*   Kaco Powador inverter.                                                   */
/*   The received data is stored in files in the same format like received    */
/*   and in the database table RCV_DATA.                                      */
/*   Every day a new log file is created in a year and month based directory. */
/*   Every hour the received mean valus are inserted into table RCV_DATA_MEAN */
/*                                                                            */
/* ========================================================================== */

#include <stdlib.h>

#include "cLogData.h"
#include "cSerial.h"
#include "cPwrMeter.h"
#include "cMyDb.h"

using namespace std;

void ShowError(char *sZbuf);
void ConvData(const char *szRcv, double &fPwrUsed, double &fPwrSold);

int main()
{
    char szBuf[MAX_DATA_LEN];
    char szPwrBuf[MAX_RCV_LEN];
    int iSize;

    string strPort = "/dev/ttyAMA0";	   // port for KACO invereter telegram
    // string strPort = "/dev/ttyUSB0";	   // PL+ TEST port for KACO invereter telegram
    string strPortPwr = "/dev/ttyUSB0";  // port for power meter telegram

    cout << "Start Kaco Logging" << endl;

    cSerial Serial(strPort, B9600, 0);  // device, baud rate, parity
    if (!Serial.InitDevice())
    {
        cout << "Error on init serial device: EXIT" << endl;
        exit(1);
    }

    cPwrMeter PwrMeter(strPortPwr, B9600, 0);  // device, baud rate, parity
    if (!PwrMeter.InitDevice())
    {
        cout << "Error on init power meter serial device: EXIT" << endl;
        exit(2);
    }
    cLogData LogData;
    if (!LogData.CreateFilePath())   // check if directory for logging exists and create if not
    {
        cout << "Error on accessing log directory: EXIT" << endl;
        exit(3);
    }

    cMyDb MyDb("localhost","kaco", "kaco", "kaco");
    if (!MyDb.Connect())
    {
        cout << "Error on connecting database: EXIT" << endl;
        exit(4);
    }

    cout << "Start receive loop" << endl;
    for (;;)    // endless loop receiving and writing data
    {
        memset(&szBuf[0], 0, KACO_DATA_LEN+1);
        // Serial.SendTestData();  // PL+ TEST
        iSize = Serial.GetData(szBuf);
        if (iSize >= KACO_DATA_LEN || iSize == 0)  // data block correct or no data
        {
            int iKacoSize = iSize;
            double fPwrUsed = 0.0;
            double fPwrSold = 0.0;

            if (iKacoSize > 0)
                cout << "Kaco data received" << endl;
            memset(&szPwrBuf[0], 0, MAX_RCV_LEN+1);
            // PwrMeter.SendTestData();   // PL+ TEST
            iSize = PwrMeter.GetData(szPwrBuf);
            if (iSize >= ITRON_DATA_LEN)     // data block correct
            {
                cout << "Power meter data received" << endl;
                ConvData(szPwrBuf, fPwrUsed, fPwrSold);
            }
            else
            {
                cout << "Wrong no.: " << iSize << " of Itron data received" << endl;
            }

            if (iKacoSize > 0)
                LogData.WriteLog(szBuf, iKacoSize);   // log data block in file
            else
                szBuf[0] = '\0';				  // no data received
			if (iSize > 0 || iKacoSize > 0)
				MyDb.InsData(szBuf, fPwrUsed, fPwrSold);
            MyDb.CheckOneHourChange(); // check hour change for inserting hour based data
        }
        else
        {
            cout << "Wrong no.: " << iSize << " of Kaco data received" << endl;
        }
        sleep(0);
    }

    return 0;
}

void ShowError(const char *szBuf)
{
    cout << szBuf << strerror (errno) << endl;
}

void ConvData(const char *szRcv, double &fPwrUsed, double &fPwrSold)
{
    char szBuf[16];
    strncpy(szBuf, &szRcv[70], 11);
    szBuf[11] = '\0';
    sscanf(&szBuf[0], "%lf", &fPwrUsed);
    strncpy(szBuf, &szRcv[98], 11);
    szBuf[11] = '\0';
    sscanf(&szBuf[0], "%lf", &fPwrSold);
    cout << "Pwr used: " << setiosflags(ios::fixed) << setprecision(4) << fPwrUsed << " Pwr sold: " << fPwrSold << endl;
}
