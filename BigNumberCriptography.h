#pragma once

#include "BigNumber.h"

BigNumber* ModularExponentiation(BigNumber *Base,BigNumber *Exponent,BigNumber *Modulus); //Calculates (Base^Exponent) mod Modulus
BigNumber *GCD(BigNumber *A, BigNumber *B); //Implementation of Stein`s BinaryGCD algorithm
BigNumber* ExtendedEuclidean(BigNumber *A, BigNumber *B, BigNumber **X, BigNumber **Y);  //Calculates GreatestCommonDivizor + (Optional)Bezout Coefficients
BigNumber * ModularInverse(BigNumber *A ,BigNumber *M); //Calculates the Modular inverser of A mod M => Inverse=A^(-1) (mod M)
BigNumber *LCM(BigNumber *A,BigNumber *B); //Calculates Least Common Multiple
BigNumber *GenerateRandomPositiveBigNumber(unsigned int NrOfDigits);//Generates a Random Positive BigNumber of NrOfDigits digits
bool MillerRabin(BigNumber *Number,unsigned int NrOfBases); //Probabilistic primality test ,return true if prime, false if composite
BigNumber *GeneratePrime(unsigned int NrOfDigits); //Generates a Prime Number with NrOfDigits digits 