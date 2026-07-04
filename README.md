HEVD exploit - There are a few exploits for HEVD, ArbitraryWrite is made to unprotect processes and UaF is only the PoC, so it only makes a BSOD.

throttlestop-exploit-rw - It is a modified version of (https://github.com/AmrHuss/throttlestop-exploit-rw), instead of elevating privileges it unprotects lsass, I have a version to disable DSE (HVCI disabled) if anyone is interested in it, send me a DM.

unprotect - This was my first driver, it has two IOCTLs, to unprotect any process and another one to elevate privileges.

Keylogger - It is the code I talked about in my blog (https://astharot15.github.io/posts/keylogger/), It is only a CTF challenge so it doesn't do too much, if you want to use it modify OnReadCompletion function, but I am pretty sure that it is caught by EDRs (not by basic AVs, of course).
