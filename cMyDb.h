#ifndef CMYDB_H
#define CMYDB_H

#include <iostream>
#include <string>
#include <sstream>
#include <sys/timeb.h>  // for time diff. calculation of act. power
#include <my_global.h>
#include <mysql.h>

using namespace std;

class FFError
{
public:
    std::string    Label;

    FFError( )
    {
        Label = (char *)"Generic Error";
    }
    FFError( char *message )
    {
        Label = message;
    }
    ~FFError() { }
    inline const char*   GetMessage  ( void )
    {
        return Label.c_str();
    }
};

class cMyDb
{
public:
    /** Default constructor */
    cMyDb(string host, string User, string Passwd, string Db);

    /** Default destructor */
    virtual ~cMyDb();
    /** Access Connect()
     * \return The current value of Connect()
     */

    bool Connect();             //!< Connect() to database"
    bool InsData(char *szData, double fTotBaught, double fTotSold); //!< InsData() insert data
    bool CheckOneHourChange();  //!< check for change of hour for inserting mean values

protected:
private:
    MYSQL   *MySQLConRet;
    MYSQL   *MySQLConnection;

    string  sHost;
    string  sUser;
    string  sPasswd;
    string  sDB;

    // data from Kaco Powador inverter
    short nMode;    // mode
    float fDCVolt;  // DC voltage
    float fDCAmps;  // DC current
    int   iDCPower; // DC power in W
    float fACVolt;  // AC voltage
    float fACAmps;  // AC current
    int   iACPower; // AC power in W, currently produced
    int   iTemp;    // temperature

    double fPowerBaught; // total baught power from power meter
    double fPowerSold;   // total sold power from power meter
    double fPowerProd;   // total produced power by solar system

    int   lastHour; // lastHour for checkin if to insert mean vals.

    bool InsMeanValues();  //!< insert mean values every hour
    bool GetLastValues();  //!< get last inserted values of power used and power sold
    bool GetFirstValues(string strStart, double &fBaught, double &fSold);
    double fLastTime;	     //!< time diff. between receiving of 2 telegrams from power meter
};

#endif // CMYDB_H
