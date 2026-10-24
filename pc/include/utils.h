#ifndef UTILS_H
#define UTILS_H

#include "wipeout.h"
#include "vector_length.h"

/* utils.c */
int abs(int value);
void CrossProduct(VECTOR *out, VECTOR a, VECTOR b);
int GetAng(VECTOR vec1, VECTOR vec2);
void MinusVector(VECTOR v1, VECTOR v2, VECTOR *result);
int ScalarProduct(VECTOR v, VECTOR w);
int ang(int angle);
int arccos(int value);
void assign(VECTOR *out, VECTOR v);
int newRand(void);
int sar(int a, int n);
int scalar_product(const VECTOR *a, const VECTOR *b);

#endif
