#include "defs.h"

void ErrorMsg(string message) {
#ifdef ERRORS
    cout << endl << "ERROR: " <<  message << "!" << endl << "\a";
    exit(1);
#endif
}

void WarningMsg(string message) {
#ifdef WARNINGS
    cout << endl << "WARNING: " <<  message << "!" << endl << "\a";;
#endif
}

void IntegerToString(int n, string* s) {
    int digits_number,digit;
    char digit_char;

    if (n<0) ErrorMsg("IntegerToString manages only positive integer");
    if (n==0) *s="0";
    else {
        *s="";
        digits_number=1+((int) log10((double) n));
        for (int i=digits_number-1; i>=0; i--) {
            double pow10=pow(10,(double) i);
            digit=(int) n/pow10;
            digit_char='0'+digit;
            *s=*s+digit_char;
            n=n-(digit*pow10);
        }
    }
    return;
}
