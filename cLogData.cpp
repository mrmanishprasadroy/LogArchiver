#include "cLogData.h"

using namespace std;
extern void ShowError(const char *szBuf);

cLogData::cLogData() : m_Counter(0)
{
  //ctor
}

cLogData::~cLogData()
{
  //dtor
}

// check path for log files format: ~/yyyy/mm, if not existing -> create
bool cLogData::CreateFilePath()
{
    bool bRet = true;
    stringstream ssFileName;
    time_t now = time(0);
    timeinfo   = localtime ( &now );
    struct stat st = {0};   // for existing path check

    ssFileName << "/home/pi/" << (timeinfo->tm_year+1900) << "/"
               << setfill('0') << setw(2) << (timeinfo->tm_mon+1);  // directory

    sPath = ssFileName.str();

    ssFileName << "/"
               << (timeinfo->tm_year+1900) << setfill('0') << setw(2) << (timeinfo->tm_mon+1) << timeinfo->tm_mday
               << ".log";  // file name

    sFileName  = ssFileName.str();

    if (stat(sPath.c_str(), &st) == -1)
    {
      mkdir(sPath.c_str(), 0777);
    }
    // check for success, tra again
    if (stat(sPath.c_str(), &st) == -1)   // did not work go up in dir. tree
    {
      string sPathUp = sPath.substr(0,sPath.size()-3);
      mkdir(sPathUp.c_str(), 0777);
      if (stat(sPathUp.c_str(), &st) != -1)
      {
        mkdir(sPath.c_str(), 0777);
        return true;
      }
      else
      {
        bRet = false;
      }
    }
    return bRet;
}

// write receided data into file
bool cLogData::WriteLog(char *szData, size_t iCnt)
{
    size_t iSize;
    FILE *pLog;         // log file pointer
	time_t now = time(0);
	timeinfo   = localtime (&now);
    if (!CreateFilePath())   // create path and filen name
    {
      cout << "Error: " << strerror (errno) << " creating directory: " << sPath << endl;
      return false;
    }
    // chek if date exists, since Kaco Pwodor is not sending the date
    
    sprintf(&szData[2], "%02d.%02d.%4d %02d:%02d:%02d", timeinfo->tm_mday,timeinfo->tm_mon+1,timeinfo->tm_year+1900,
													      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    szData[21] = ' ';
	szData[22] = ' ';
    	
	pLog = fopen (sFileName.c_str(), "a");

    if (pLog == NULL)
    {
        cout << "File: " << sFileName << " open error: " << strerror (errno) << endl;
        return false;
    }
    iSize = fprintf(pLog, "%s", szData);
    if (iSize != iCnt-1)
    {
      cout << "Error writing data: " << strerror (errno) << endl;
	  cout << "iSize: " << iSize << " iCnt: " << iCnt << endl;
	  fclose(pLog);
      return false;
    }
    fflush(pLog);
    fclose(pLog);
    return true;
}

