#pragma once

#include "Config.h"
#include <stdbool.h>
#include <stdint.h>

unsigned int GetHardwareThreadCount(void); //Return the number of Threads that are available for MultiThreaded algoritms O(1)
                                           //In case of single core CPU`s the return value is 1

int8_t GetSecureRandomDigit(bool allowZero); //Generate one truly random digit from [0-9] or [1-9] depending on allowZero