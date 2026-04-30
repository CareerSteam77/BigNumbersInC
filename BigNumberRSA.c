#include "Platform.h" 
#include "BigNumberCriptography.h"
#include "BigNumberRSA.h"
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if(SUPPORT_MULTITHREADING)
   #include <pthread.h>
   typedef struct 
     {
        int NrOfDigitsKey;
        BigNumber **Prime;

     }PrimeGenerationThreadArgument;

   void* GeneratePrimeTheadFunc(void *ThreadArgument)
    {
        PrimeGenerationThreadArgument* argument=(PrimeGenerationThreadArgument *) ThreadArgument;
        *argument->Prime=GeneratePrime(argument->NrOfDigitsKey);
        return NULL;
    }
#endif

void GenerateRSAKeyValues(unsigned int KeyBitsSize, BigNumber **PublicExponent ,BigNumber **PrivateExponent ,BigNumber **N)
{
      if(PublicExponent==NULL || *PublicExponent==NULL)
        {   
            //DEFAULT PUBLIC EXPONENT IS E=65537=2^16+1 which is ideal because of low Hamming Weight
            *PublicExponent=Init(RSA_DEFAULT_PUBLIC_EXPONENT);
        }
      
      // log_10(2) is ~ 0.30103 ,PrimeNumbersLenghtInDecimal is Half of the RSA BINARY DIGITS STANDARD as we generate 2 such numbers
      //We generate 2 DIFFERENT prime numbers ,P and Q,
      //If we have more than 2 threads, we can assign 1 thread per prime number

      unsigned int PrimeNumbersLenghtInDecimal=(unsigned int)(KeyBitsSize*0.30103)/2 + 1;
      BigNumber* P=NULL;
      BigNumber* Q=NULL;
      #if(SUPPORT_MULTITHREADING)
        {
          do
          {
            if(Q) FreeMemory(Q);

            pthread_t ThreadP;
            pthread_t ThreadQ;
            PrimeGenerationThreadArgument ArgP; ArgP.NrOfDigitsKey=PrimeNumbersLenghtInDecimal;ArgP.Prime=&P;
            PrimeGenerationThreadArgument ArgQ; ArgQ.NrOfDigitsKey=PrimeNumbersLenghtInDecimal;ArgQ.Prime=&Q;

            if(pthread_create(&ThreadP, NULL, GeneratePrimeTheadFunc, &ArgP)!=0) 
            {
                perror("Creating Thread for Generating first Prime FAILED");
                exit(-2);
            }
            if(pthread_create(&ThreadQ, NULL, GeneratePrimeTheadFunc, &ArgQ)!=0) 
            {
                perror("Creating Thread for Generating second Prime FAILED");
                exit(-2);
            }

            if(pthread_join(ThreadP, NULL)!=0)
            {
                perror("Joining Thread for Generating first Prime FAILED");
                exit(-2);
            }  
            if(pthread_join(ThreadQ, NULL)!=0)
            {
                perror("Joining Thread for Generating second Prime FAILED");
                exit(-2);
            }

          }while(IsEqual(P,Q)==true);

          if(*N) FreeMemory(*N);
          *N=Multiply(P,Q);

        }
      #endif

      if(!SUPPORT_MULTITHREADING)
        {
            P=GeneratePrime(PrimeNumbersLenghtInDecimal);

            do{
                if(Q) FreeMemory(Q);
                Q=GeneratePrime(PrimeNumbersLenghtInDecimal);
            }while(IsEqual(P,Q)==true);

            if(*N) FreeMemory(*N);
            *N=Multiply(P,Q);
        }
      
      BigNumber *One=Init("1");
      BigNumber* TotientP=Subtract(P,One);
      BigNumber* TotientQ=Subtract(Q,One);
      BigNumber* CarmichaelLamda=LCM(TotientP,TotientQ);

      if(*PrivateExponent) FreeMemory(*PrivateExponent);
      *PrivateExponent=ModularInverse(PublicExponent,CarmichaelLamda);
      
      if(*PrivateExponent == NULL || *PublicExponent == NULL || *N == NULL)
        {
            perror("Generating RSA KEY VALUES FAILED");
            exit(-3);
        }
        
      //We clean the memory after usage to avoid any cold boot or memory dump attacks
      memset(P->Digits, '0', strlen(P->Digits));
      memset(Q->Digits, '0',  strlen(Q->Digits));
      memset(TotientP->Digits, '0',  strlen(TotientP->Digits));
      memset(TotientQ->Digits, '0',  strlen(TotientQ->Digits));
      memset(CarmichaelLamda->Digits, '0',  strlen(CarmichaelLamda->Digits));
    
      //Final memory Cleanup
      FreeMemory(One);
      FreeMemory(P);
      FreeMemory(Q);
      FreeMemory(TotientP);
      FreeMemory(TotientQ);
      FreeMemory(CarmichaelLamda);
     
}