/* ========================================================================== */
/*                                                                            */
/*   KacoLog.c                                                                */
/*   (c) 2012 Paul Lunter                                                     */
/*                                                                            */
/*   Description                                                              */
/*   Serial interface for receiving data from power meter Itron               */
/*   The received data is stored in database table                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* ========================================================================== */

#include "cPwrMeter.h"

cPwrMeter::cPwrMeter(string &Portname, int iBaud, int iParity)
    : m_sPort(Portname), m_Baud(iBaud), m_Parity(iParity)
{
    //ctor

}

cPwrMeter::~cPwrMeter()
{
    close(m_Fd);
}
// open serial device for data receive
bool cPwrMeter::InitDevice()
{
    m_Fd = open (m_sPort.c_str(), O_RDWR | O_NOCTTY);

    cout << "Initialize power meter device " << endl;
    if (m_Fd < 0)
    {
        cout << "Device: " << m_sPort << " open error: " << strerror (errno) << endl;
        return false;
    }
    if (!SetAttribs())    // block or time out in s
        return false;

    if (!SetBlocking(0, 4))
        return false;

    fcntl(m_Fd, F_SETFL, O_RDWR);
    return true;
}

//!< set blocking, wait time for receiving data
bool cPwrMeter::SetBlocking(bool bBlock, int iTimeOut)    // block or time out in s
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    cout << "Set blocking for power meter interface " << endl;

    if (tcgetattr (m_Fd, &tty) != 0)
    {
        cout << "Power meter device error from tcgetattr: " << strerror (errno) << endl;
        return false;
    }

    tty.c_cc[VMIN]  = bBlock ? 1 : 0;
    tty.c_cc[VTIME] = iTimeOut*10;            //  in 100 ms seconds read timeout

    if (tcsetattr (m_Fd, TCSANOW, &tty) != 0)
    {
        cout << "Power meter device error setting attributes: " << strerror (errno) << endl;
        return false;
    }
    return true;
}

//!< set attributes for communication
bool  cPwrMeter::SetAttribs()
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);

    cout << "Set attributes for power meter interface " << endl;
    if (tcgetattr (m_Fd, &tty) != 0)
    {
        cout << "Power meter device setting attribs error from tcgetattr: " << strerror (errno) << endl;
        return false;
    }

    cfsetospeed (&tty, m_Baud);
    cfsetispeed (&tty, m_Baud);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7;   // 7-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    /*
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);        // ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity

    tty.c_cflag |= PARENB;                  // even parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (m_Fd, TCSANOW, &tty) != 0)
    {
        cout << "Power meter device setting attribs error from tcsetattr: " << strerror (errno) << endl;
        return false;
    }

    return true;
}

// read data from serial interafce
int cPwrMeter::GetData(char *szBuf)
{
    int iCnt = 0;       // no. of bytes read
    int iPos = 1;       // position in buffer to store data block

    m_cBuf[0] = '\0';
    cout << "Start reading power meter interface" << endl;
	tcflush( m_Fd, TCIFLUSH );
	cout << "Start reading power meter buffer flushed" << endl;
	
    // read leading \n chars
    do
    {
        iCnt = read (m_Fd, &m_cBuf[0], 1);
		if (iCnt > 0)
			cout << "Rec.: " << m_cBuf[0] << endl;
    }
    while (iCnt != 0 && (m_cBuf[0] == '\n' || m_cBuf[0] == '\r' || m_cBuf[0] == '\0'));

    if (iCnt == 0)		// nothing received
        return 0;

    if (m_cBuf[0] != '/')
    {
        do
        {
            iCnt = read (m_Fd, &m_cBuf[0], 1);
        }
        while (iCnt != 0 && m_cBuf[0] != '/');		// read 1st valid character in message
    }
    if (iCnt == 0)		// nothing received
        return 0;

    szBuf[0] = m_cBuf[0];
    do
    {
        iCnt = read (m_Fd, &szBuf[iPos], ITRON_DATA_LEN);  // read up to 165 characters if ready to read
        if (szBuf[iPos+iCnt-1] == '\n' && szBuf[iPos+iCnt-2] == '\r' && szBuf[iPos+iCnt-3] == '!'); // ?? überprüfen || szBuf[iPos+iCnt-1] == '\0')
        {
            szBuf[iPos+iCnt] = '\0'; // set end of buufer            
            m_Counter++;             // just a message counter
        }

        iPos += iCnt;

    }
    while (iPos < ITRON_DATA_LEN-1);
    ftime(&tp);		// get time with milliseconds
	printf("Itron Daten: %s\n", &szBuf[0]);   // output data to screen
    return iPos+1;    // dta size
}

// read data from serial interafce
void cPwrMeter::SendTestData()
{
    write (m_Fd, "/HAG5eHZ010C_IRWEZA20\r\n\r\n1-0:0.0.0*255(797810-5000900)\r\n1-0:1.8.1*255(007359.7748)\r\n1-0:2.8.1*255(003891.0604)\r\n1-0:96.5.5*255(82)\r\n0-0:96.1.255*255(0000134493)\r\n!\r\n", ITRON_DATA_LEN);
}

