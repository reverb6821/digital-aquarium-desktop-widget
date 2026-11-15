#include "fixmath.h"
#include <math.h>

float fm_sin_table[FM_TABLE_SIZE + 1];

void fm_init(void)
{
    for (int i = 0; i <= FM_TABLE_SIZE; i++) fm_sin_table[i] = sinf((float)i * FM_TAU / FM_TABLE_SIZE);
}
