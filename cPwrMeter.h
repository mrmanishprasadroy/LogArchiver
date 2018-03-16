#ifndef CPWRMETER_H
#define CPWRMETER_H

#include <iostream>
#include <string>
#include <sys/types.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/timeb.h> /* for getting time in milliseconds*/

#define MAX_RCV_LEN 512    // max length of data block from serial interface
#define ITRON_DATA_LEN 164  // data length of telegram, data block ends with !\r\n

using namespace std;

class cPwrMeter
{
public:
    /** Default constructor */
    cPwrMeter(string &Portname, int iBaud, int iParity);
    /** Default destructor */
    virtual ~cPwrMeter();
    /** Access m_Counter
     * \return The current value of m_Counter
     */
    void Close()
    {
        close(m_Fd);
    }
    unsigned int GetCounter()
    {
        return m_Counter;
    }
    /** Set m_Counter
     * \param val New value to set
     */
    void SetCounter(unsigned int val)
    {
        m_Counter = val;
    }
    /** Access m_Fd
     * \return The current value of m_Fd
     */

    int GetData(char *szBuf); // read data from serial interafce
    void SendTestData();      // send test data, used with loop back connector

    int  RcvDataBlock(); //!< receive block of data
    int  SndDataBlock(); //!< send blok of data, used for test with loopback RS232 connector
    bool InitDevice();   //!< open device and set attributes

protected:
private:
    struct timeb tp;		//!< time structure with milliseconds
    unsigned int m_Counter; //!< Member variable "m_Counter"
    string m_sPort;         //!< device name
    int m_Baud;             //!< baud rate
    int m_Fd;               //!< file descriptor for data receive
    int m_Parity;
    char m_cBuf[256];         //!> receive buffer

    bool SetBlocking(bool bBlock, int iTimeOut);  //!< set blocking, wait time for receiving data"
    bool  SetAttribs();   //!< set attributes for communication"
};

#endif // CSERIAL_H
