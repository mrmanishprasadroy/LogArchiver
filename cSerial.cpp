#include "cSerial.h"

cSerial::cSerial(string &Portname, int iBaud, int iParity)
  : m_sPort(Portname), m_Baud(iBaud), m_Parity(iParity)
{
  //ctor

}

cSerial::~cSerial()
{
  close(m_Fd);
}
// open serial device for data receive
bool cSerial::InitDevice()
{
    m_Fd = open (m_sPort.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY); // 

    if (m_Fd < 0)
    {
        // error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        cout << "Device: " << m_sPort << " open error: " << strerror (errno) << endl;
        return false;
    }
    if (!SetAttribs())   // block or time out in s
      return false;
	  
	if (!SetBlocking(0, 10))
	  return false;
	  
	fcntl(m_Fd, F_SETFL, O_RDWR);  
    return true;
}

//!< set blocking, wait time for receiving data
bool cSerial::SetBlocking(bool bBlock, int iTimeOut)    // block or time out in s
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
	cout << "Set blocking for Kaco interface" << endl;
    if (tcgetattr (m_Fd, &tty) != 0)
    {
        cout << "Device error from tcgetattr: " << strerror (errno) << endl;
        return false;
    }

    tty.c_cc[VMIN]  = bBlock ? 1 : 0;
    tty.c_cc[VTIME] = iTimeOut*10;            //  in 100 ms seconds read timeout

    if (tcsetattr (m_Fd, TCSANOW, &tty) != 0)
	{
	   cout << "Device error setting attributes: " << strerror (errno) << endl;
	   return false;
	}
	tcflush(m_Fd, TCIOFLUSH);
	return true;
}

//!< set attributes for communication
bool  cSerial::SetAttribs()
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
	cout << "Set attributes for Kaco interface " << endl;
	
    if (tcgetattr (m_Fd, &tty) != 0)
    {
        cout << "Device setting attribs error from tcgetattr: " << strerror (errno) << endl;
        return false;
    }

    cfsetospeed (&tty, m_Baud);
    cfsetispeed (&tty, m_Baud);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;   // 8-bit chars
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
    tty.c_cflag |= m_Parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	
    if (tcsetattr (m_Fd, TCSANOW, &tty) != 0)
    {
        cout << "Device setting attribs error from tcsetattr: " << strerror (errno) << endl;
        return false;
    }

    return true;
}

// read data from serial interafce
int cSerial::GetData(char *szBuf)
{
  int iCnt = 0;       // no. of bytes read
  int iPos = 1;       // position in buffer to store data block
 
  // read leading \n chars
  do
  {
    iCnt = read (m_Fd, &m_cBuf[0], 1);
  } while (iCnt != 0 && (m_cBuf[0] == '\n' || m_cBuf[0] == '\r' || m_cBuf[0] == '\0'));
  
  if (iCnt == 0)		// nothing received
	return 0;
	
  szBuf[0] = m_cBuf[0];	
  
  do
  {   
    iCnt = read (m_Fd, &szBuf[iPos], KACO_DATA_LEN);  // read up to 65 characters if ready to read
    if (szBuf[iPos+iCnt-1] == '\n')
    {
      szBuf[iPos+iCnt] = '\0'; // set end of buufer
      printf("Daten: %s\n", &szBuf[0]);   // output data to screen
      m_Counter++;             // just a message counter
    }
	cout << "Kaco number of chars: " << iCnt << endl;
    iPos += iCnt;
  } while (iCnt != 0 && iPos < KACO_DATA_LEN-1);
  
  szBuf[iPos] = '\0';
  printf("Daten: %s\n", &szBuf[0]);   // output data to screen
  cout << "Length: " << iPos+1 << endl;
  return iPos+1;    // dta size
}

// read data from serial interafce
void cSerial::SendTestData()
{
  write (m_Fd, "00.00.0000 16:50:20   3 486.4  0.23   113 232.3  2.40   109  37 \r\n", KACO_DATA_LEN);
}
