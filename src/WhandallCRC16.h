#ifndef _WHANDALLCRC16_H_
#define _WHANDALLCRC16_H_
/*! \mainpage WhandallCRC16
 *
 * \section intro_sec Introduction
 *
 *	A small class to compute a 16 bit CRC (CRC-16-CCITT polynomial)
 *	like it is used by the Nordic nRF24L01+ 2.4GHz wireless transceiver
 *
 * - configurable start value (default 0xFFFF)
 *
 * - configurable polynom (default 0x1021)
 *
 * - feed a bit
 *
 * - feed bytes
 *
 * - feed bytes in reverse order
 *
 * \section hist_sec History
 *
 * - 2017 July, first version
 *
 * \section install_sec Installation
 *
 * standard Arduino library install
 *  
 */
 /***
	* This object holds the current CRC, the polynomial and the functions manipulating/retrieving that data.
	*/
class CRC16 {
	/// the current crc
    uint16_t  crc;
	/// the polynomial used
		uint16_t	polynom;
  public:
		/** Initialization
		 *
		 *	Setup for a new CRC computation. Initialize the CRC and the polynomial.
		 *
		 * NRF24L01: The polynomial for 2 byte CRC is X^16+ X^12 + X^5 + 1. Initial value 0xFFFF.  (==> equals CRC-16-CCITT polynomial)
		 *	@param initWith [in] startvalue of the CRC
		 *	@param poly [in] the polynomial to use (without the x^16 part)
		 */
    void init(uint16_t initWith = 0xFFFF, uint16_t poly = 0x1021);
		/** Stuff data
		 *
		 *	Feed a single bit (the highest bit of the passed byte)
		 *	@param topBit [in] the highest bit of tis value (0x80) will be fed to into the CRC
		 */
    void feed(uint8_t topBit);
 		/** Stuff data
		 *
		 *	Feed a range of bytes, in ascending order
		 *	@param ptr [in] points to data in RAM
		 *	@param len [in] length of the data
		 */
		void feed(const uint8_t* ptr, uint8_t len);
		/** Stuff data
		 *
		 *	Feed a range of bytes, in descending order
		 *	@param ptr [in] points to data in RAM
		 *	@param len [in] length of the data
		 */
    void feedReverse(const uint8_t* ptr, uint8_t len);
		/** Result
		 *
		 *	Returns the computed CRC
		 *	@return the computed CRC
		 */
    uint16_t getCRC();
};

#endif