/****************************************************************************
CRC_32.cpp : implementation file for the CRC_32 class
written by PJ Arends
pja@telus.net

based on the CRC-32 code found at
http://www.createwindow.com/programming/crc32/crcfile.htm

For updates check http://www3.telus.net/pja/crc32.htm

-----------------------------------------------------------------------------
This code is provided as is, with no warranty as to it's suitability or usefulness
in any application in which it may be used. This code has not been tested for
UNICODE builds, nor has it been tested on a network ( with UNC paths ).

This code may be used in any way you desire. This file may be redistributed by any
means as long as it is not sold for profit, and providing that this notice and the
authors name are included.

If any bugs are found and fixed, a note to the author explaining the problem and
fix would be nice.
-----------------------------------------------------------------------------

created : October 2001

Revision History:

October 11, 2001   - changed SendMessage to PostMessage in CRC32ThreadProc

****************************************************************************/

#include "CRC_32.h"
#include <tchar.h>      // UNICODE compatibility

// use a 100 KB buffer (a larger buffer does not seem to run
// significantly faster, but takes more RAM)
#define BUFFERSIZE 102400

/////////////////////////////////////////////////////////////////////////////
//
//  CRC_32 constructor  (public member function)
//    Sets up the CRC-32 reference table
//
//  Parameters :
//    None
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

CRC_32::CRC_32()
{
    // This is the official polynomial used by CRC-32
    // in PKZip, WinZip and Ethernet.
    ULONG ulPolynomial = 0x04C11DB7;

    // 256 values representing ASCII character codes.
    for (int i = 0; i <= 0xFF; i++)
    {
        Table[i] = Reflect(i, 8) << 24;
        for (int j = 0; j < 8; j++)
            Table[i] = (Table[i] << 1) ^ (Table[i] & (1 << 31) ? ulPolynomial : 0);
        Table[i] = Reflect(Table[i], 32);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  CRC_32::Reflect  (private member function)
//    used by the constructor to help set up the CRC-32 reference table
//
//  Parameters :
//    ref [in] : the value to be reflected
//    ch  [in] : the number of bits to move
//
//  Returns :
//    the new value
//
/////////////////////////////////////////////////////////////////////////////

ULONG CRC_32::Reflect(ULONG ref, char ch)
{
    ULONG value = 0;

    // Swap bit 0 for bit 7
    // bit 1 for bit 6, etc.
    for(int i = 1; i < (ch + 1); i++)
    {
        if (ref & 1)
            value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CRC_32::Calculate  (private member function)
//    Calculates the CRC-32 value for the given buffer
//
//  Parameters :
//    buffer [in] : pointer to the data bytes
//    size   [in] : the size of the buffer
//    CRC    [in] : the initial CRC-32 value
//          [out] : the new CRC-32 value
//
//  Returns :
//    Nothing
//
/////////////////////////////////////////////////////////////////////////////

void CRC_32::Calculate(const LPBYTE buffer, UINT size, ULONG &CRC)
{   // calculate the CRC
    LPBYTE pbyte = buffer;

    while (size--)
        CRC = (CRC >> 8) ^ Table[(CRC & 0xFF) ^ *pbyte++];
}

/////////////////////////////////////////////////////////////////////////////
//
//  CRC_32::CalcCRC  (public member function)
//    calculates the CRC-32 value for the given buffer
//
//  Parameters :
//    buffer      [in] : a pointer to the data bytes
//    size        [in] : the size of the buffer
//
//  Returns :
//    CRC-32 value of the buffer
//
/////////////////////////////////////////////////////////////////////////////

DWORD CRC_32::CalcCRC(LPVOID buffer, UINT size) {
    if (!buffer || !size) {
        return 0;
    }

    DWORD CRC = 0xFFFFFFFF;
    Calculate ((LPBYTE)buffer, size, CRC);
    return CRC ^ 0xFFFFFFFF;
}

/////////////////////////////////////////////////////////////////////////////
//
//  End of CRC_32.cpp
//
/////////////////////////////////////////////////////////////////////////////




