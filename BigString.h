#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_STRING_SIZE SIZE_MAX

typedef struct {
   uint8_t *Data;
   size_t   Length;
}BigString;

BigString* InitBigString(const char *Text);
BigString* Constructor(uint8_t *Text,size_t Length);

BigString *CloneBigString(const BigString *StringToBeCloned);
BigString *ConcatenateBigString(const BigString *String1, const BigString *String2 ,size_t* NrOfCharactersToBeConcatenated);
BigString *CopyFromBigString(BigString *Source,size_t *NumberOfCharactersToBeCopied,size_t *Offset);

void ConcatenateInPlace(BigString *Destination, const BigString *Source ,size_t* NrOfCharactersToBeConcatenated);
bool IsEqualBigString(const BigString *String1, const BigString *String2);
size_t BigStringStrlen(BigString* Message);
int CompareBigString(const BigString *String1, const BigString *String2); //Compares strings in Lexicographic Order

void SanitizeMemory(BigString *Message);
void FreeMemoryString(BigString *Message);

void PrintBigString(const BigString *Message);
void PrintBigStringInHexaDecimalFormat(const BigString *Message);
