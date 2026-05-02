#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#define MAX_STRING_SIZE SIZE_MAX
#define IS_CHAR_SIGNED (char)-1 > 0
#define BIGSTRING_NOT_FOUND ((size_t)-1)

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
void OverwriteBigString(BigString *Destination, const BigString *Source ,size_t *OffsetDestination,size_t *OffsetSource, size_t *Length);
bool IsEqualBigString(const BigString *String1, const BigString *String2);
size_t BigStringStrlen(BigString* Message);
int CompareBigString(const BigString *String1, const BigString *String2); //Compares strings in Lexicographic Order
size_t BigStringSearch(const BigString *String, const BigString *SubString); //Return the index(or BIGSTRING_NOT_FOUND ) of the first apparition of SubString in String

char *FromBigStringToCHAR(const BigString *String);

void SanitizeMemory(BigString *Message);
void FreeMemoryString(BigString *Message);

void PrintBigString(const BigString *Message);
void PrintBigStringInHexaDecimalFormat(const BigString *Message);
void PrintBigStringInBinaryFormat(const BigString *Message);
