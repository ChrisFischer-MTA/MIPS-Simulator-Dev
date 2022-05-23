#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "avl.h"
using namespace std;

/* permissions is 00000rwe in the byte
 * (for example, to set r you would express permissions |= 0b100)
 */
typedef struct allo
{
    int start;
    int finish;
    char* bytes;
    int length;
    char permissions;

} allocation;




