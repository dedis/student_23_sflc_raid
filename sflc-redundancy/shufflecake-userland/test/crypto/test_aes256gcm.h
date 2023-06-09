/*
 *  Copyright The Shufflecake Project Authors (2022)
 *  Copyright The Shufflecake Project Contributors (2022)
 *  Copyright Contributors to the The Shufflecake Project.
 *  
 *  See the AUTHORS file at the top-level directory of this distribution and at
 *  <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *  
 *  This file is part of the program shufflecake-userland, which is part of the 
 *  Shufflecake Project. Shufflecake is a plausible deniability (hidden storage) 
 *  layer for Linux. See <https://www.shufflecake.net>.
 *  
 *  This program is free software: you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the Free 
 *  Software Foundation, either version 3 of the License, or (at your option) 
 *  any later version. This program is distributed in the hope that it will be 
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
 *  Public License for more details. You should have received a copy of the 
 *  GNU General Public License along with this program. 
 *  If not, see <https://www.gnu.org/licenses/>.
 */

// Test vectors taken from test case 15 at
// https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/gcm-spec.pdf

#ifndef _TEST_CRYPTO_AES256GCM_H_
#define _TEST_CRYPTO_AES256GCM_H_


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

#define AES256GCM_TEST_KEY							\
{													\
	0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, \
	0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08, \
	0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, \
	0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08	\
}

#define AES256GCM_TEST_IV							\
{													\
	0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, \
	0xde, 0xca, 0xf8, 0x88							\
}

#define AES256GCM_TEST_PT							\
{													\
	0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5, \
	0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a, \
	0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda, \
	0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72, \
	0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53, \
	0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25, \
	0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57, \
	0xba, 0x63, 0x7b, 0x39, 0x1a, 0xaf, 0xd2, 0x55	\
}

#define AES256GCM_TEST_CT							\
{													\
	0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07, \
	0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d, \
	0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9, \
	0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa, \
	0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d, \
	0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38, \
	0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a, \
	0xbc, 0xc9, 0xf6, 0x62, 0x89, 0x80, 0x15, 0xad, \
}

#define AES256GCM_TEST_TAG							\
{													\
	0xb0, 0x94, 0xda, 0xc5, 0xd9, 0x34, 0x71, 0xbd, \
	0xec, 0x1a, 0x50, 0x22, 0x70, 0xe3, 0xcc, 0x6c,	\
}


/*****************************************************
 *          PUBLIC FUNCTIONS DECLARATIONS            *
 *****************************************************/

// Exported test cases

char *test_aes256gcm_encrypt();
char *test_aes256gcm_decrypt_good();
char *test_aes256gcm_decrypt_fail();


#endif /* _TEST_CRYPTO_AES256GCM_H_ */
