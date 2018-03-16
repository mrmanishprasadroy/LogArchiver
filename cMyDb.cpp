#include <iomanip>
#include "cMyDb.h"

cMyDb::cMyDb(string Host, string User, string Passwd, string Db)
    : MySQLConnection(NULL), sHost(Host), sUser(User), sPasswd(Passwd), sDB(Db)
{
    //ctor
    nMode     = 0;
    fDCVolt   = 0.0;
    fDCAmps   = 0.0;
    iDCPower  = 0;
    fACVolt   = 0.0;
    fACAmps   = 0.0;
    iACPower  = 0;
    iTemp     = 0;

    fPowerBaught = -1.0;
    fPowerSold   = -1.0;

    lastHour  = -1;   // detection of hour change
    fLastTime = 11.0;	// used for calc. of actual power (W) baught and sold
    fPowerProd=  0.0; // total produced power
}

cMyDb::~cMyDb()
{
    //dtor
    mysql_close(MySQLConnection);
}
// --------------------------------------------------------------------
// Connect to database
bool cMyDb::Connect()
{
    MySQLConnection = mysql_init(NULL);

    try
    {
        MySQLConRet = mysql_real_connect( MySQLConnection,
                                          sHost.c_str(),
                                          sUser.c_str(),
                                          sPasswd.c_str(),
                                          sDB.c_str(),
                                          0,
                                          NULL,
                                          0 );

        if ( MySQLConRet == NULL )
            cout << (char*)mysql_error(MySQLConnection) << endl;
        // throw FFError( (char*) mysql_error(MySQLConnection) );

        printf("MySQL Connection Info: %s \n", mysql_get_host_info(MySQLConnection));
        printf("MySQL Client Info: %s \n", mysql_get_client_info());
        printf("MySQL Server Info: %s \n", mysql_get_server_info(MySQLConnection));

    }
    catch ( FFError e )
    {
        printf("%s\n",e.Label.c_str());
        return false;
    }
    return true;
}
// insert received data into database, fPwrUsed and fPwrSold in kWh from power meter
bool cMyDb::InsData(char *szData, double fTotBaught, double fTotSold)
{
    int     mysqlStatus = 0;
    double  fActPwrBaught = 0.0;   // actual power baught (mean val. in 10 sec. in W)
    double  fActPwrSold   = 0.0;   // actual power sold (mean val. in 10 sec. in W)

    MYSQL_RES  *mysqlResult = NULL;
    string     sDate, sTime;
    stringstream ssBuf;

    struct timeb tNow;
    double fNow;
    ftime(&tNow);
    fNow = (double)tNow.time + double(tNow.millitm/1000.0);
	
    if (fPowerBaught < 0.0)// first time after restart
    {
        cout << "Insert data. Get last values." << endl;
		GetLastValues();
    }
    float fTimeDiff = fNow - fLastTime;

	cout << "Insert data. Time diff: " << fTimeDiff << endl;
	
 // new value - last value, results in currently baught res. sold power
	if (fTotBaught != 0 || fTotSold != 0)
	{
		fActPwrBaught = (fTotBaught - fPowerBaught)*3600.0/fTimeDiff * 1000.0;// in W
		fActPwrSold   = (fTotSold   - fPowerSold)*3600.0/fTimeDiff * 1000.0;  // in W
	}	
	else	// nothing received
	{
		fTotBaught = fPowerBaught;  // take last value
		fTotSold   = fPowerSold;    // take last value
	}
	
	cout << "Insert data.  Tot. power baught:  " << fTotBaught << " sold: " << fTotSold << endl;
	cout << "Insert data. Act. power baught: " << fActPwrBaught << " sold: " << fActPwrSold << endl;
	
	fLastTime = fNow;
	
    if (szData[0] != '\0')
    {
        ssBuf << szData;

        ssBuf >> sDate >> sTime
        >> nMode
        >> fDCVolt >> fDCAmps >> iDCPower
        >> fACVolt >> fACAmps >> iACPower
        >> iTemp;


        // Kaco inverter send message intervall 10s -> produced energy in 10s.
        fPowerProd += (float)iACPower/360.0/1000.0;  // related to 1h and in kWh
    }
    else
    {
        nMode     = -1;
        fDCVolt   = 0.0;
        fDCAmps   = 0.0;
        iDCPower  = 0;
        fACVolt   = 0.0;
        fACAmps   = 0.0;
        iACPower  = 0;
        iTemp     = 0;
    }
    // --------------------------------------------------------------------
    // This block of code would be performed if this insert were in a loop
    // with changing data. Of course it is not necessary in this example.
    if (mysqlResult)
    {
        mysql_free_result(mysqlResult);
        mysqlResult = NULL;
    }

    // --------------------------------------------------------------------
    // Perform a SQL INSERT
    try
    {
        stringstream ssIns;

        ssIns  << "INSERT INTO RCV_DATA (RCV_DATE, MODE, DC_VOLTAGE, DC_CURRENT, DC_POWER, AC_VOLTAGE, AC_CURRENT, AC_POWER, "
        << "TEMPERATURE, POWER_BAUGHT_TOT, POWER_BAUGHT_ACT, POWER_SOLD_TOT, POWER_SOLD_ACT, POWER_PROD_TOT, TIME_DIFF) "
        << "VALUES (NOW()," << nMode << ","<< fDCVolt << "," << fDCAmps << "," << iDCPower << ","
        << fACVolt << "," << fACAmps << "," << iACPower << "," << iTemp << "," << setprecision(10) << fTotBaught << ","
        << fActPwrBaught << "," << fTotSold << "," << fActPwrSold << "," << fPowerProd << "," << fTimeDiff << ")";

        string sqlInsStatement = ssIns.str();			
		
        mysqlStatus = mysql_query(MySQLConnection, sqlInsStatement.c_str());
        if (mysqlStatus)
        {
            cout << (char*)mysql_error(MySQLConnection) << endl;
            return false;
        }
    }
    catch ( FFError e )
    {
        printf("%s\n",e.Label.c_str());
        return false;
    }
    fPowerBaught = fTotBaught;  // save for next insert
    fPowerSold   = fTotSold;    // save for next insert
    return true;
}

// insert mean values every hour
bool cMyDb::InsMeanValues()
{
    int         mysqlStatus = 0;
    MYSQL_RES   *mysqlResult = NULL;
    MYSQL_ROW   row;
    MYSQL_FIELD *field;
    int         iNumCols;       // no. of colums
    string      sStartDate, sEndDate;

    time_t oneHourAgo  = time(0)-3600;
    time_t now         = time(0);

    // --------------------------------------------------------------------
    // This block of code would be performed if this insert were in a loop
    // with changing data. Of course it is not necessary in this example.
    if (mysqlResult)
    {
        mysql_free_result(mysqlResult);
        mysqlResult = NULL;
    }

    // --------------------------------------------------------------------
    // Perform a SQL INSERT
    try
    {
        struct tm *tStart       = localtime ( &oneHourAgo );    // start time
        stringstream ssStart;

        ssStart << (tStart->tm_year+1900) << "-" << setfill('0') << setw(2) << (tStart->tm_mon+1)
        << "-" << setfill('0') << setw(2) <<  tStart->tm_mday
        << " " << tStart->tm_hour << ":00:00";
        sStartDate = ssStart.str();

        double fBaughtFirst = 0;
        double fSoldFirst   = 0;
        GetFirstValues(sStartDate, fBaughtFirst, fSoldFirst);

        struct tm *tEnd         = localtime ( &now );           // end time
        stringstream ssEnd;
        ssEnd << (tEnd->tm_year+1900) << "-" << setfill('0') << setw(2) << (tEnd->tm_mon+1)
        << "-" << setfill('0') << setw(2) << tEnd->tm_mday
        << " " << tEnd->tm_hour << ":00:00";
        sEndDate = ssEnd.str();

        stringstream ssSQL;
        // select mean values for one hour
        ssSQL << "SELECT avg(DC_VOLTAGE), avg(DC_CURRENT), avg(DC_POWER), avg(AC_VOLTAGE), avg(AC_CURRENT), avg(AC_POWER), avg(TEMPERATURE) "
        << "FROM RCV_DATA "
        << "WHERE RCV_DATE >= '" << sStartDate << "' AND RCV_DATE < '" << sEndDate << "'";

        stringstream ssIns;
        // insert the mean values
        ssIns  << "INSERT INTO RCV_DATA_MEAN (RCV_DATE_HOUR, DC_VOLTAGE, DC_CURRENT, DC_POWER, AC_VOLTAGE, AC_CURRENT, "
        << "AC_POWER, TEMPERATURE, POWER_BAUGHT_ACT, POWER_SOLD_ACT, POWER_BAUGHT_TOT, POWER_SOLD_TOT, POWER_PROD_TOT)"
        << " VALUES ('" << sStartDate << "',";

        string sqlSelStatement = ssSQL.str();
		
		// cout << "Select: " << sqlSelStatement << endl;
		
        mysqlStatus = mysql_query(MySQLConnection, sqlSelStatement.c_str());

        if (mysqlStatus)
        {
            cout << (char*)mysql_error(MySQLConnection) << endl;
            return false;
        }

        mysqlResult = mysql_store_result(MySQLConnection);
        iNumCols    = mysql_num_fields(mysqlResult);

        while ((row = mysql_fetch_row(mysqlResult)))
        {
            for(int i = 0; i < iNumCols; i++)
            {
                printf("%s  ", row[i] ? row[i] : "NULL");
                ssIns << row[i];    // add column value
                ssIns << ",";
            }
        }
        // last inserted total power and hour values
        ssIns << setprecision(10) << fPowerBaught-fBaughtFirst << "," << fPowerSold-fSoldFirst << ","
        << fPowerBaught << "," << fPowerSold << "," << fPowerProd << ")";

        string sqlInsStatement = ssIns.str();
		cout << "Insert: " << sqlInsStatement << endl;
        mysqlStatus = mysql_query(MySQLConnection, sqlInsStatement.c_str());

        if (mysqlStatus)
        {
            cout << (char*)mysql_error(MySQLConnection) << endl;
            return false;
        }
    }
    catch ( FFError e )
    {
        printf("%s\n",e.Label.c_str());
        return false;
    }

    printf("\n");
    mysql_free_result(mysqlResult);
    return true;
}

// get last inserted values of power used and power sold
bool cMyDb::GetLastValues()
{
    int         mysqlStatus = 0;
    MYSQL_RES   *mysqlResult = NULL;
    MYSQL_ROW   row;

    stringstream ssVals;
    stringstream ssDate;
    string strDate;
    int iYear, iMon, iDay, iH, iMi, iSec;

    stringstream ssSQL;
    // select mean values for one hour
    ssSQL << "SELECT RCV_DATE, POWER_BAUGHT_TOT, POWER_SOLD_TOT, POWER_PROD_TOT FROM kaco.RCV_DATA "
    << "WHERE RCV_DATE = (select max(RCV_DATE) FROM RCV_DATA)";

    string sqlSelStatement = ssSQL.str();
    mysqlStatus = mysql_query(MySQLConnection, sqlSelStatement.c_str());

    if (mysqlStatus)
    {
        // throw FFError( (char*)mysql_error(MySQLConnection) );
        cout << (char*)mysql_error(MySQLConnection) << endl;
        return false;
    }

    mysqlResult = mysql_store_result(MySQLConnection);
    int iNumCols    = mysql_num_fields(mysqlResult);

    row = mysql_fetch_row(mysqlResult);

    strDate = row[0];
    strDate.replace(strDate.find("-"), 1, " ");
    strDate.replace(strDate.find("-"), 1, " ");
    strDate.replace(strDate.find(":"), 1, " ");
    strDate.replace(strDate.find(":"), 1, " ");

    ssDate << strDate;
    ssDate >> iYear >> iMon >> iDay >> iH >> iMi >> iSec;
    struct tm tInf;
    tInf.tm_year = iYear-1900;
    tInf.tm_mon  = iMon-1;
    tInf.tm_mday = iDay;
    tInf.tm_hour = iH;
    tInf.tm_min  = iMi;
    tInf.tm_sec  = iSec;

    fLastTime = mktime(&tInf); // deutsche Zeit in sec

    ssVals << row[1] << " " << row[2] << " " << row[3];    // add column value
    ssVals >> fPowerBaught >> fPowerSold >> fPowerProd;
    cout << "Last total power baught = " << fPowerBaught << " total power sold = " << fPowerSold << endl;
    if (mysqlResult)
    {
        mysql_free_result(mysqlResult);
        mysqlResult = NULL;
    }
	return true;
}

// get last inserted values of power used and power sold in one hour intervall
bool cMyDb::GetFirstValues(string strStart, double &fBaught, double &fSold)
{
    int         mysqlStatus = 0;
    MYSQL_RES   *mysqlResult = NULL;
    MYSQL_ROW   row;

    stringstream ssVals;
    stringstream ssDate;
    string strDate;
    int iYear, iMon, iDay, iH, iMi, iSec;

    stringstream ssSQL;
    // select first values in one hour intervall
    ssSQL << "SELECT RCV_DATE, POWER_BAUGHT_TOT, POWER_SOLD_TOT FROM kaco.RCV_DATA "
    << "WHERE RCV_DATE >= '" << strStart << "' LIMIT 0,1";

    string sqlSelStatement = ssSQL.str();
    mysqlStatus = mysql_query(MySQLConnection, sqlSelStatement.c_str());

    if (mysqlStatus)
    {
        // throw FFError( (char*)mysql_error(MySQLConnection) );
        cout << (char*)mysql_error(MySQLConnection) << endl;
        return false;
    }

    mysqlResult = mysql_store_result(MySQLConnection);
    int iNumCols    = mysql_num_fields(mysqlResult);

    row = mysql_fetch_row(mysqlResult);

    ssVals << row[1] << " " << row[2];    // add column value
    ssVals >> fBaught >> fSold;
    if (mysqlResult)
    {
        mysql_free_result(mysqlResult);
        mysqlResult = NULL;
    }	
    return true;
}

// check if hour changed
bool cMyDb::CheckOneHourChange()
{
    bool bRet = true;
    time_t now  = time(0);
    struct tm *tNow  = localtime ( &now ); // current time

    if (lastHour < 0)
        lastHour = tNow->tm_hour;
	
    if (lastHour != tNow->tm_hour) // hour changed
    {
        if (!InsMeanValues())     // inserting mean values not successfull
        {
            cout << "Error inserting mean values" << endl;
            bRet = false;
        }
        else
            cout << "Mean values inserted." << endl;

        lastHour = tNow->tm_hour;
    }
    return bRet;
}
