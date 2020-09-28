/** @file conv.h
 * @brief Public interface for faux convert functions.
 */

#ifndef _faux_conv_h
#define _faux_conv_h

#include <faux/faux.h>

C_DECL_BEGIN

int faux_conv_atol(const char *str, long int *val, int base);
int faux_conv_atoul(const char *str, unsigned long int *val, int base);

int faux_conv_atoll(const char *str, long long int *val, int base);
int faux_conv_atoull(const char *str, unsigned long long int *val, int base);

int faux_conv_atoi(const char *str, int *val, int base);
int faux_conv_atoui(const char *str, unsigned int *val, int base);

int faux_conv_atos(const char *str, short *val, int base);
int faux_conv_atous(const char *str, unsigned short *val, int base);

int faux_conv_atoc(const char *str, short *val, int base);
int faux_conv_atouc(const char *str, unsigned short *val, int base);

C_DECL_END

#endif
