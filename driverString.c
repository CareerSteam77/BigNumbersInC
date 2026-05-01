#include "BigString.h"

int main(void)
{
    size_t NrOfCharacters=4;
    size_t Offset=1;

    BigString* mesaj1=InitBigString("Salut");
    BigString* mesaj2=InitBigString("I love CS2");
    BigString* mesaj3=ConcatenateBigString(mesaj1,mesaj2,NULL);
    BigString* mesaj4=ConcatenateBigString(mesaj1,mesaj2,&NrOfCharacters);
    BigString* mesaj5=CloneBigString(mesaj1);
    BigString* mesaj6=CopyFromBigString(mesaj1,&NrOfCharacters,&Offset);
    BigString* mesaj0=InitBigString("TOBESANITIZED"); SanitizeMemory(mesaj0);

    printf("After Initializing BigStrings\n");

    printf("Sanitazied String:");PrintBigStringInHexaDecimalFormat(mesaj0);printf("\n");
    PrintBigString(mesaj1);printf("\n");
    PrintBigString(mesaj2);printf("\n");
    PrintBigString(mesaj3);printf("\n");
    PrintBigString(mesaj4);printf("\n");
    PrintBigString(mesaj5);printf("\n");
    ConcatenateInPlace(mesaj2,mesaj1,NULL);ConcatenateInPlace(mesaj2,mesaj1,NULL);PrintBigString(mesaj2);printf("\n");
    PrintBigString(mesaj6);printf("\n");

    FreeMemoryString(mesaj0);
    FreeMemoryString(mesaj1);
    FreeMemoryString(mesaj2);
    FreeMemoryString(mesaj3);
    FreeMemoryString(mesaj4);
    FreeMemoryString(mesaj5);
    FreeMemoryString(mesaj6);

    printf("After Sanitazing and Clearing the memory\n");
    return 0;
}