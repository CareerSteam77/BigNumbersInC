#include "BigString.h"

BigString* InitBigString(const char *Text)
{
    if(Text==NULL) return NULL;
    
    BigString *Message=(BigString *)malloc(sizeof(BigString));
    if(Message==NULL)
      {
        perror("Allocating memeory for Message inside InitString failed");
        exit(-1);
      }
    
    size_t TextLength=strlen(Text);
    
    if(TextLength==0)
      {
         uint8_t *Data=(uint8_t*)malloc(sizeof(uint8_t)*1);
         Data[0]=0;
         Message->Data=Data;
         Message->Length=0;
         return Message;
      }
    
    uint8_t *Data=(uint8_t*)malloc(sizeof(uint8_t)*TextLength);
    if(Data==NULL)
      {
        perror("Allocating memory for Message Data inside InitString failed");
        exit(-1);
      }
    
    memcpy(Data,Text,TextLength);
    Message->Data=Data;
    Message->Length=TextLength;

    return Message;

}

BigString* Constructor(uint8_t *Text,size_t Length)
{
    if(Text==NULL) return NULL;

    BigString *Message=malloc(sizeof(BigString));
    if(Message==NULL)
      {
        perror("Allocating memeory for Message inside InitString failed");
        exit(-1);
      }
    
    Message->Data=Text;
    Message->Length=Length;

    return Message;
}

BigString *CloneBigString(const BigString *StringToBeCloned)  //Creates an Exact Replica of StringToBeCopied without altering the original BigString
{
     if(StringToBeCloned==NULL) return NULL;

     BigString *Clone=(BigString *)malloc(sizeof(BigString));
     if(Clone==NULL)
       {
         perror("Allocating memory for Clone inside failed");
         exit(-1);
       }
    
    uint8_t *CloneData=(uint8_t*)malloc(sizeof(uint8_t)*(StringToBeCloned->Length));
    if(CloneData==NULL)
      {
        perror("Allocating memory for CloneData inside CloneString failed");
        exit(-1);
      }
    
    memcpy(CloneData,StringToBeCloned->Data,StringToBeCloned->Length);
    Clone->Length=StringToBeCloned->Length;
    Clone->Data=CloneData;

    return Clone;

}

BigString *ConcatenateBigString(const BigString *String1, const BigString *String2 ,size_t* NrOfCharactersToBeConcatenated)
{
    if(String1==NULL || String2==NULL) return NULL;

    size_t LocalNrOfCharactersToBeConcatenated=String2->Length;
    if(NrOfCharactersToBeConcatenated != NULL)
      {
        LocalNrOfCharactersToBeConcatenated=*NrOfCharactersToBeConcatenated;
      }

    if(String1->Length+LocalNrOfCharactersToBeConcatenated > MAX_STRING_SIZE) return NULL;
    if(LocalNrOfCharactersToBeConcatenated > String2->Length) LocalNrOfCharactersToBeConcatenated=String2->Length;

    BigString *Result=(BigString *)malloc(sizeof(BigString));
    if(Result==NULL)
     {
        perror("Allocating memory for Concatenated String failed");
        exit(-1);
     }
    
    size_t ConcatSize=String1->Length+LocalNrOfCharactersToBeConcatenated;
    uint8_t *ConcatData=(uint8_t *)malloc(sizeof(uint8_t)*ConcatSize);
    if(ConcatData==NULL)
      {
        free(Result);
        perror("Allocating memory for ConcatData failed");
        exit(-1);
      }
    
    memcpy(ConcatData,String1->Data,String1->Length);
    memcpy(ConcatData+String1->Length,String2->Data,LocalNrOfCharactersToBeConcatenated);

    Result->Length=ConcatSize;
    Result->Data  =ConcatData;

    return Result;
}

void ConcatenateInPlace(BigString *Destination, const BigString *Source ,size_t* NrOfCharactersToBeConcatenated)
{
    if(Destination == NULL || Source == NULL) return;

    size_t LocalNrOfCharactersToBeConcatenated=Source->Length;
    if(NrOfCharactersToBeConcatenated != NULL)
     {
        LocalNrOfCharactersToBeConcatenated=*NrOfCharactersToBeConcatenated;
     }
    
    if(Destination->Length+LocalNrOfCharactersToBeConcatenated > MAX_STRING_SIZE) return;
    if(LocalNrOfCharactersToBeConcatenated > Source->Length) LocalNrOfCharactersToBeConcatenated=Source->Length;

    uint8_t *TempData=(uint8_t *)realloc(Destination->Data,sizeof(uint8_t)*(Destination->Length+LocalNrOfCharactersToBeConcatenated));
    if(TempData==NULL)
      {
        perror("Reallocating memory for Destination Data inside ConcatenateInPlace failed");
        exit(-1);
      }
    Destination->Data=TempData;

    memcpy(Destination->Data+Destination->Length,Source->Data,LocalNrOfCharactersToBeConcatenated);
    Destination->Length+=LocalNrOfCharactersToBeConcatenated;
}

bool IsEqualBigString(const BigString *String1, const BigString *String2)
{
    if(String1 == NULL && String2 == NULL) return true;
    if(String1 == NULL || String2 == NULL) return false;
    if(String1->Length != String2->Length) return false;

    return (memcmp(String1->Data, String2->Data, String1->Length) == 0);
}

int CompareBigString(const BigString *String1, const BigString *String2) //Compares strings in Lexicographic Order
{
    if(String1 == NULL && String2 == NULL) return 0;
    if(String1 == NULL) return -1;
    if(String2 == NULL) return  1;

    if(String1->Length > String2->Length)
      {
         size_t index=0;
         for(index=0;index<String2->Length;index++)
           {
              if(String1->Data[index] != String2->Data[index])
                 return (int)(String1->Data[index]-String2->Data[index]);
           }

        return String1->Data[String2->Length];
      }
    else
      {
         size_t MatchingCharacters=0;
         size_t index=0;
         for(index=0;index<String2->Length;index++)
           {
              if(String1->Data[index] != String2->Data[index])
                 return (int)(String1->Data[index]-String2->Data[index]);
              else
                 MatchingCharacters++;
           }
        
        if(String1->Length==String2->Length && String1->Length==MatchingCharacters)
           return 0;

        return String2->Data[String1->Length];  
      } 
}

BigString *CopyFromBigString(BigString *Source,size_t *NumberOfCharactersToBeCopied,size_t *Offset)
{
    if(Source==NULL) return NULL;

    size_t LocalOffset=0;
    size_t LocalNumberOfCharactersToBeCopied=Source->Length;

    if(NumberOfCharactersToBeCopied!=NULL)
      {
         LocalNumberOfCharactersToBeCopied=*NumberOfCharactersToBeCopied;
      } 
    if(Offset!=NULL)
      {
         LocalOffset=*Offset;
      }
    
    if(LocalOffset >= Source->Length)
    {
        return NULL; 
    }
    if(LocalOffset + LocalNumberOfCharactersToBeCopied > Source->Length)
    {
        LocalNumberOfCharactersToBeCopied = Source->Length - LocalOffset;
    }

    BigString *Copy=(BigString *)malloc(sizeof(BigString));
    if(Copy==NULL)
      {
        perror("Allocating memory for Copy inside CopyBigString failed");
        exit(-1);
      }
    
    uint8_t *CopyData=(uint8_t *)malloc(sizeof(uint8_t)*LocalNumberOfCharactersToBeCopied);
    if(CopyData==NULL)
      {
        free(Copy);
        perror("Allocating memory for CopyData inside CopyBigString failed");
        exit(-1);
      }
    
    Copy->Length=LocalNumberOfCharactersToBeCopied;
    memcpy(CopyData,Source->Data+LocalOffset,LocalNumberOfCharactersToBeCopied);
    Copy->Data=CopyData;

    return Copy;
}

/*
void OverwriteBigString(BigString *Destination,BigString *Source,size_t *OffsetDestination,size_t *OffsetSource, size_t *Lenght)
{
    if(Destination == NULL || Source == NULL) return;

}
*/

void SanitizeMemory(BigString *Message)
{
   if(Message==NULL) return;
   if(Message->Data!=NULL)
    {
       memset(Message->Data,0,Message->Length);
    }
}

void FreeMemoryString(BigString *Message)
{   
    if(Message==NULL) return;

    SanitizeMemory(Message);
    Message->Length = 0;
    if(Message->Data!=NULL) free(Message->Data);
    free(Message);
}

size_t BigStringStrlen(BigString* Message)
{
    if(Message==NULL) return 0;
    return Message->Length;
}

void PrintBigString(const BigString *Message)
{
    if(Message==NULL)
      {
        printf("Message is NULL");
        return;
      }
    
    size_t index=0;
    for(index=0;index<Message->Length;index++)
       {
         printf("%c",Message->Data[index]);
       }
}

void PrintBigStringInHexaDecimalFormat(const BigString *Message)
{
    if(Message==NULL)
      {
        printf("Message is NULL");
        return;
      }
    
    size_t index=0;
    for(index=0;index<Message->Length;index++)
       {
         printf("%02X ",Message->Data[index]);
       }
}