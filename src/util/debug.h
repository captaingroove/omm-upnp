#ifndef DEBUG_H
#define DEBUG_H

// uncomment this to turn debugging on (or provide it on the compiler command line)
#define __DEBUG__

#include <cstdio>
//#include "debugthread.h"


// using namespace std;
// 
// class Debug
// {
// public:
// //  Debug ();
// //  ~Debug ();
// 
// //    void trace (string s, bool newline = true);
// //    void trace (const char* s1, const char* s2, bool newline = true);
// //    void trace (const char* s, long i, unsigned int radix = 10, bool newline = true);
//     static void trace(string s);
// 
// 
// //private:
// //    DebugThread* dt;
// };

//extern Debug* dp;

#ifdef __DEBUG__
#define TRACE(args) printf args
#else
#define TRACE(args) 
#endif  // __DEBUG__

#endif  // DEBUG_H
