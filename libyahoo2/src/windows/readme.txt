Here resides various files for native Windows support for libyahoo2.

Supported compilers includes:
* Microsoft Visual C++ 6.0 [vc6]
* Microsoft Visual C++ .NET 2003 [vc7]
* Microsoft Visual C++ 2005 (tested with the Express edition) [vc8]
* Microsoft Visual C++ 2008 (tested with the Express edition) [vc9]

Of course there is a plan to support more compilers in the future.

You can use the project/solution file of the compiler of your choice to build
both a static as well as a dynamic libyahoo2 libary.

All projects were configured for compiling libyahoo2 in Multithreaded DLL, so
you might want to change the configuration to better suit your need.

A sample Y!M client using libyahoo2 is given in sample_client.c, and so far,
it is the best reference available in order to implement your own Y!M client.

If you have difficulties using libyahoo2, you can ask for help in our forum at
http://sourceforge.net/mailarchive/forum.php?forum_name=libyahoo2-users

Thanks,
Tri S.