#ifndef CLOGDATA_H
#define CLOGDATA_H

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

class cLogData
{
public:
    /** Default constructor */
    cLogData();
    /** Default destructor */
    virtual ~cLogData();
    /** Access m_Counter
     * \return The current value of m_Counter
     */
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

    bool CreateFilePath();
    bool WriteLog(char *szData, size_t iCnt);

protected:
private:
    unsigned int m_Counter; //!< Member variable "m_Counter"
    struct tm  *timeinfo;   // time and date structure
    std::string sPath;      // path to log fil
    std::string sFileName;  // filename including path
};

#endif // CLOGDATA_H
