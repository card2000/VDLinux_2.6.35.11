/**
 * \file	CC_Constants.h
 * \brief	define constants for crypto library
 *
 * - Copyright : Samsung Electronics CO.LTD.,
 *
 * \internal
 * Author : Jisoon, Park
 * Dept : DRM Lab, Digital Media Laboratory
 * Creation date : 2006/09/28
 * $Id$
 */


#ifndef _DRM_CONSTANTS_H
#define _DRM_CONSTANTS_H

////////////////////////////////////////////////////////////////////////////
// Algorithm Identifier
////////////////////////////////////////////////////////////////////////////
enum CryptoAlgorithm {

	/*!	\brief	Asymmetric Encryption Algorithms	*/
	ID_RSA								= 1051,
	ID_RSA1024							= 1054,

	ID_NO_PADDING						= 1204,

};

////////////////////////////////////////////////////////////////////////////
// Constants Definitions
////////////////////////////////////////////////////////////////////////////
/*!	\brief	Endianness	*/
#define CRYPTO_LITTLE_ENDIAN			0
#define CRYPTO_BIG_ENDIAN				1

////////////////////////////////////////////////////////////////////////////
// Crypto Error Code
////////////////////////////////////////////////////////////////////////////
#define CRYPTO_SUCCESS					0				/**<	no error is occured	*/
#define CRYPTO_ERROR					-3000			/**<	error is occured	*/
#define CRYPTO_MEMORY_ALLOC_FAIL		-3001			/**<	malloc is failed	*/
#define CRYPTO_NULL_POINTER				-3002			/**<	parameter is null pointer	*/
#define CRYPTO_INVALID_ARGUMENT			-3003			/**<	argument is not correct	*/
#define CRYPTO_MSG_TOO_LONG				-3004			/**<	length of input message is too long	*/
#define CRYPTO_VALID_SIGN				CRYPTO_SUCCESS	/**<	valid sign	*/
#define CRYPTO_INVALID_SIGN				-3011			/**<	invalid sign	*/
#define CRYPTO_ISPRIME					CRYPTO_SUCCESS	/**<	prime number	*/
#define CRYPTO_INVERSE_NOT_EXIST		-3012			/**<	inverse is no exists	*/
#define CRYPTO_NEGATIVE_INPUT			-3013			/**<	argument is negative	*/
#define CRYPTO_INFINITY_INPUT			-3014			/**<	input is infinity	*/
#define CRYPTO_BUFFER_TOO_SMALL			-3015			/**<	buffer to small	*/

#endif

/***************************** End of File *****************************/
