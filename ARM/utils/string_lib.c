/*************************************************
 * Copyright (C) 2009 Aviad Zuck & Sivan Toledo
 * This file is part of NANDFS.
 *
 * To license NANDFS under a different license, please
 * contact the authors.
 *
 * NANDFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NANDFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with NANDFS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <utils/string_lib.h>
////////// string function /////////////////////////////////////////////////
// http://en.wikibooks.org/wiki/C_Programming/Strings

int32_t fsStrcmp(uint8_t *s1, uint8_t *s2){	
    uint32_t uc1, uc2;
    
    /* Move s1 and s2 to the first differing characters 
       in each string, or the ends of the strings if they
       are identical.  */
    while (*s1 != '\0' && *s1 == *s2) {
//    	PRINT_MSG_AND_NUM("\nfsStrcmp() - *s1=", *s1); 
//    	PRINT_MSG_AND_NUM(" *s2=", *s2);    	
    	s1++;
    	s2++;        
    }
    /* Compare the characters as unsigned char and
       return the difference.  */
    uc1 = (*(uint8_t *) s1);
    uc2 = (*(uint8_t *) s2);
    return ((uc1 < uc2) ? -1 : (uc1 > uc2));
}

int32_t fsStrncmp(uint8_t *s1, uint8_t *s2, uint32_t n){
    uint32_t uc1, uc2;
    
    /* Nothing to compare?  Return zero.  */
    if (n == 0)
        return 0;
    /* Loop, comparing bytes.  */
    while (n-- > 0 && *s1 == *s2) {
        /* If we've run out of bytes or hit a null, return zero
           since we already know *s1 == *s2.  */
        if (n == 0 || *s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    uc1 = (*(uint8_t *) s1);
    uc2 = (*(uint8_t *) s2);
    return ((uc1 < uc2) ? -1 : (uc1 > uc2));
}


uint8_t *fsStrcpy(uint8_t *s1, uint8_t *s2){
        uint8_t *orig_target = s1;

        while(*s2)
                *s1++ = *s2++;
        *s1 = '\0';

        return orig_target;
}
