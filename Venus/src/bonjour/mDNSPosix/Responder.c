/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2004 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if __APPLE__
// In Mac OS X 10.5 and later trying to use the daemon function gives a “‘daemon’ is deprecated”
// error, which prevents compilation because we build with "-Werror".
// Since this is supposed to be portable cross-platform code, we don't care that daemon is
// deprecated on Mac OS X 10.5, so we use this preprocessor trick to eliminate the error message.
#define daemon yes_we_know_that_daemon_is_deprecated_in_os_x_10_5_thankyou
#endif

#include <assert.h>
#include <stdio.h>			// For printf()
#include <stdlib.h>			// For exit() etc.
#include <string.h>			// For strlen() etc.
#ifdef WIN32
#else
#include <unistd.h>			// For select()
#endif
#include <errno.h>			// For errno, EINTR
#include <signal.h>
#include <fcntl.h>

#if __APPLE__
#undef daemon
extern int daemon(int, int);
#endif

#include "mDNSEmbeddedAPI.h"// Defines the interface to the client layer above
#ifdef WIN32
#include "mDNSWin32.h"		// Defines the specific types needed to run mDNS on this platform
#else
#include "mDNSPosix.h"		// Defines the specific types needed to run mDNS on this platform
#include "mDNSUNP.h"		// For daemon()
#endif

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Globals
#endif

mDNS mDNSStorage;       // mDNS core uses this to store its globals
static mDNS_PlatformSupport PlatformStorage;  // Stores this platform's globals

mDNSexport const char ProgramName[] = "mDNSResponderPosix";

static const char *gProgramName = ProgramName;

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Signals
#endif

static volatile mDNSBool gReceivedSigUsr1;
static volatile mDNSBool gReceivedSigHup;
static volatile mDNSBool gStopNow;

// We support 4 signals.
//
// o SIGUSR1 toggles verbose mode on and off in debug builds
// o SIGHUP  triggers the program to re-read its preferences.
// o SIGINT  causes an orderly shutdown of the program.
// o SIGQUIT causes a somewhat orderly shutdown (direct but dangerous)
// o SIGKILL kills us dead (easy to implement :-)
//
// There are fatal race conditions in our signal handling, but there's not much 
// we can do about them while remaining within the Posix space.  Specifically, 
// if a signal arrives after we test the globals its sets but before we call 
// select, the signal will be dropped.  The user will have to send the signal 
// again.  Unfortunately, Posix does not have a "sigselect" to atomically 
// modify the signal mask and start a select.

static void HandleSigUsr1(int sigraised)
    // If we get a SIGUSR1 we toggle the state of the 
    // verbose mode.
{
    assert(sigraised == SIGUSR1);
    gReceivedSigUsr1 = mDNStrue;
}

static void HandleSigHup(int sigraised)
    // A handler for SIGHUP that causes us to break out of the 
    // main event loop when the user kill 1's us.  This has the 
    // effect of triggered the main loop to deregister the 
    // current services and re-read the preferences.
{
    assert(sigraised == SIGHUP);
	gReceivedSigHup = mDNStrue;
}

static void HandleSigInt(int sigraised)
    // A handler for SIGINT that causes us to break out of the 
    // main event loop when the user types ^C.  This has the 
    // effect of quitting the program.
{
    assert(sigraised == SIGINT);
    
    if (gMDNSPlatformPosixVerboseLevel > 0) {
        fprintf(stderr, "\nSIGINT\n");
    }
    gStopNow = mDNStrue;
}

static void HandleSigQuit(int sigraised)
    // If we get a SIGQUIT the user is desperate and we 
    // just call mDNS_Close directly.  This is definitely 
    // not safe (because it could reenter mDNS), but 
    // we presume that the user has already tried the safe 
    // alternatives.
{
    assert(sigraised == SIGQUIT);

    if (gMDNSPlatformPosixVerboseLevel > 0) {
        fprintf(stderr, "\nSIGQUIT\n");
    }
    mDNS_Close(&mDNSStorage);
    exit(0);
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Parameter Checking
#endif

static mDNSBool CheckThatRichTextNameIsUsable(const char *richTextName, mDNSBool printExplanation)
    // Checks that richTextName is reasonable 
    // label and, if it isn't and printExplanation is true, prints 
    // an explanation of why not.
{
    mDNSBool result = mDNStrue;
    if (result && strlen(richTextName) > 63) {
        if (printExplanation) {
            fprintf(stderr, 
                    "%s: Service name is too long (must be 63 characters or less)\n", 
                    gProgramName);
        }
        result = mDNSfalse;
    }
    if (result && richTextName[0] == 0) {
        if (printExplanation) {
            fprintf(stderr, "%s: Service name can't be empty\n", gProgramName);
        }
        result = mDNSfalse;
    }
    return result;
}

static mDNSBool CheckThatServiceTypeIsUsable(const char *serviceType, mDNSBool printExplanation)
    // Checks that serviceType is a reasonable service type 
    // label and, if it isn't and printExplanation is true, prints 
    // an explanation of why not.
{
    mDNSBool result;
    
    result = mDNStrue;
    if (result && strlen(serviceType) > 63) {
        if (printExplanation) {
            fprintf(stderr, 
                    "%s: Service type is too long (must be 63 characters or less)\n", 
                    gProgramName);
        }
        result = mDNSfalse;
    }
    if (result && serviceType[0] == 0) {
        if (printExplanation) {
            fprintf(stderr, 
                    "%s: Service type can't be empty\n", 
                    gProgramName);
        }
        result = mDNSfalse;
    }
    return result;
}

static mDNSBool CheckThatPortNumberIsUsable(long portNumber, mDNSBool printExplanation)
    // Checks that portNumber is a reasonable port number
    // and, if it isn't and printExplanation is true, prints 
    // an explanation of why not.
{
    mDNSBool result;
    
    result = mDNStrue;
    if (result && (portNumber <= 0 || portNumber > 65535)) {
        if (printExplanation) {
            fprintf(stderr, 
                    "%s: Port number specified by -p must be in range 1..65535\n", 
                    gProgramName);
        }
        result = mDNSfalse;
    }
    return result;
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Command Line Arguments
#endif

static const char kDefaultPIDFile[]     = "/var/run/mDNSResponder.pid";
static const char kDefaultServiceType[] = "_afpovertcp._tcp.";
static const char kDefaultServiceDomain[] = "local.";
enum {
    kDefaultPortNumber = 548
};

static void PrintUsage()
{
    fprintf(stderr, 
            "Usage: %s [-v level ] [-r] [-n name] [-t type] [-d domain] [-p port] [-f file] [-b] [-P pidfile] [-x name=val ...]\n", 
            gProgramName);
    fprintf(stderr, "          -v verbose mode, level is a number from 0 to 2\n");
    fprintf(stderr, "             0 = no debugging info (default)\n");
    fprintf(stderr, "             1 = standard debugging info\n");
    fprintf(stderr, "             2 = intense debugging info\n");
    fprintf(stderr, "             can be cycled kill -USR1\n");
    fprintf(stderr, "          -r also bind to port 53 (port 5353 is always bound)\n");
    fprintf(stderr, "          -n uses 'name' as the service name (required)\n");
    fprintf(stderr, "          -t uses 'type' as the service type (default is '%s')\n", kDefaultServiceType);
    fprintf(stderr, "          -d uses 'domain' as the service domain (default is '%s')\n", kDefaultServiceDomain);
    fprintf(stderr, "          -p uses 'port' as the port number (default is '%d')\n",  kDefaultPortNumber);
    fprintf(stderr, "          -f reads a service list from 'file'\n");
    fprintf(stderr, "          -b forces daemon (background) mode\n");
    fprintf(stderr, "          -P uses 'pidfile' as the PID file\n");
    fprintf(stderr, "             (default is '%s')\n",  kDefaultPIDFile);
    fprintf(stderr, "             only meaningful if -b also specified\n");
    fprintf(stderr, "          -x stores name=val in TXT record (default is empty).\n");
    fprintf(stderr, "             MUST be the last command-line argument;\n");
    fprintf(stderr, "             all subsequent arguments after -x are treated as name=val pairs.\n");
}

static   mDNSBool  gAvoidPort53      = mDNStrue;
static const char *gServiceName      = "";
static const char *gServiceType      = kDefaultServiceType;
static const char *gServiceDomain    = kDefaultServiceDomain;
static mDNSu8      gServiceText[sizeof(RDataBody)];
static mDNSu16     gServiceTextLen   = 0;
static        int  gPortNumber       = kDefaultPortNumber;
static const char *gServiceFile      = "";
static   mDNSBool  gDaemon           = mDNSfalse;
static const char *gPIDFile          = kDefaultPIDFile;

static void ParseArguments(int argc, char **argv)
    // Parses our command line arguments into the global variables 
    // listed above.
{
    int ch;
    
    // Set gProgramName to the last path component of argv[0]
    
    gProgramName = strrchr(argv[0], '/');
    if (gProgramName == NULL) {
        gProgramName = argv[0];
    } else {
        gProgramName += 1;
    }
    
    // Parse command line options using getopt.
    
    do {
        ch = getopt(argc, argv, "v:rn:t:d:p:f:dP:bx");
        if (ch != -1) {
            switch (ch) {
                case 'v':
                    gMDNSPlatformPosixVerboseLevel = atoi(optarg);
                    if (gMDNSPlatformPosixVerboseLevel < 0 || gMDNSPlatformPosixVerboseLevel > 2) {
                        fprintf(stderr, 
                                "%s: Verbose mode must be in the range 0..2\n", 
                                gProgramName);
                        exit(1);
                    }
                    break;
                case 'r':
                    gAvoidPort53 = mDNSfalse;
                    break;
                case 'n':
                    gServiceName = optarg;
                    if ( ! CheckThatRichTextNameIsUsable(gServiceName, mDNStrue) ) {
                        exit(1);
                    }
                    break;
                case 't':
                    gServiceType = optarg;
                    if ( ! CheckThatServiceTypeIsUsable(gServiceType, mDNStrue) ) {
                        exit(1);
                    }
                    break;
                case 'd':
                    gServiceDomain = optarg;
                    break;
                case 'p':
                    gPortNumber = atol(optarg);
                    if ( ! CheckThatPortNumberIsUsable(gPortNumber, mDNStrue) ) {
                        exit(1);
                    }
                    break;
                case 'f':
                    gServiceFile = optarg;
                    break;
                case 'b':
                    gDaemon = mDNStrue;
                    break;
                case 'P':
                    gPIDFile = optarg;
                    break;
                case 'x':
                	while (optind < argc)
                		{
                		gServiceText[gServiceTextLen] = strlen(argv[optind]);
                		mDNSPlatformMemCopy(gServiceText+gServiceTextLen+1, argv[optind], gServiceText[gServiceTextLen]);
                		gServiceTextLen += 1 + gServiceText[gServiceTextLen];
                		optind++;
                		}
                	ch = -1;
                	break;
                case '?':
                default:
                    PrintUsage();
                    exit(1);
                    break;
            }
        }
    } while (ch != -1);

    // Check for any left over command line arguments.
    
    if (optind != argc) {
	    PrintUsage();
        fprintf(stderr, "%s: Unexpected argument '%s'\n", gProgramName, argv[optind]);
        exit(1);
    }
    
    // Check for inconsistency between the arguments.
    
    if ( (gServiceName[0] == 0) && (gServiceFile[0] == 0) ) {
    	PrintUsage();
        fprintf(stderr, "%s: You must specify a service name to register (-n) or a service file (-f).\n", gProgramName);
        exit(1);
    }
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark ***** Registration
#endif

typedef struct PosixService PosixService;

struct PosixService {
    ServiceRecordSet coreServ;
    PosixService *next;
    int serviceID;
};

static PosixService *gServiceList = NULL;

static void RegistrationCallback(mDNS *const m, ServiceRecordSet *const thisRegistration, mStatus status)
    // mDNS core calls this routine to tell us about the status of 
    // our registration.  The appropriate action to take depends 
    // entirely on the value of status.
{
    switch (status) {

        case mStatus_NoError:      
            debugf("Callback: %##s Name Registered",   thisRegistration->RR_SRV.resrec.name->c); 
            // Do nothing; our name was successfully registered.  We may 
            // get more call backs in the future.
            break;

        case mStatus_NameConflict: 
            debugf("Callback: %##s Name Conflict",     thisRegistration->RR_SRV.resrec.name->c); 

            // In the event of a conflict, this sample RegistrationCallback 
            // just calls mDNS_RenameAndReregisterService to automatically 
            // pick a new unique name for the service. For a device such as a 
            // printer, this may be appropriate.  For a device with a user 
            // interface, and a screen, and a keyboard, the appropriate response 
            // may be to prompt the user and ask them to choose a new name for 
            // the service.
            //
            // Also, what do we do if mDNS_RenameAndReregisterService returns an 
            // error.  Right now I have no place to send that error to.
            
            status = mDNS_RenameAndReregisterService(m, thisRegistration, mDNSNULL);
            assert(status == mStatus_NoError);
            break;

        case mStatus_MemFree:      
            debugf("Callback: %##s Memory Free",       thisRegistration->RR_SRV.resrec.name->c); 
            
            // When debugging is enabled, make sure that thisRegistration 
            // is not on our gServiceList.
            
            #if !defined(NDEBUG)
                {
                    PosixService *cursor;
                    
                    cursor = gServiceList;
                    while (cursor != NULL) {
                        assert(&cursor->coreServ != thisRegistration);
                        cursor = cursor->next;
                    }
                }
            #endif
            free(thisRegistration);
            break;

        default:                   
            debugf("Callback: %##s Unknown Status %ld", thisRegistration->RR_SRV.resrec.name->c, status); 
            break;
    }
}

static int gServiceID = 0;

static mStatus RegisterOneService(const char *  richTextName, 
                                  const char *  serviceType, 
                                  const char *  serviceDomain, 
                                  const mDNSu8  text[],
                                  mDNSu16       textLen,
                                  long          portNumber)
{
    mStatus             status;
    PosixService *      thisServ;
    domainlabel         name;
    domainname          type;
    domainname          domain;
    
    status = mStatus_NoError;
    thisServ = (PosixService *) malloc(sizeof(*thisServ));
    if (thisServ == NULL) {
        status = mStatus_NoMemoryErr;
    }
    if (status == mStatus_NoError) {
        MakeDomainLabelFromLiteralString(&name,  richTextName);
        MakeDomainNameFromDNSNameString(&type, serviceType);
        MakeDomainNameFromDNSNameString(&domain, serviceDomain);
        status = mDNS_RegisterService(&mDNSStorage, &thisServ->coreServ,
                &name, &type, &domain,				// Name, type, domain
                NULL, mDNSOpaque16fromIntVal(portNumber),
                text, textLen,						// TXT data, length
                NULL, 0,							// Subtypes
                mDNSInterface_Any,					// Interface ID
                RegistrationCallback, thisServ, 0);	// Callback, context, flags
    }
    if (status == mStatus_NoError) {
        thisServ->serviceID = gServiceID;
        gServiceID += 1;

        thisServ->next = gServiceList;
        gServiceList = thisServ;

        if (gMDNSPlatformPosixVerboseLevel > 0) {
            fprintf(stderr, 
                    "%s: Registered service %d, name '%s', type '%s', port %ld\n", 
                    gProgramName, 
                    thisServ->serviceID, 
                    richTextName,
                    serviceType,
                    portNumber);
        }
    } else {
        if (thisServ != NULL) {
            free(thisServ);
        }
    }
    return status;
}

static mDNSBool ReadALine(char *buf, size_t bufSize, FILE *fp)
// Read a line, skipping over any blank lines or lines starting with '#'
{
	mDNSBool good, skip;
	do {
		good = (fgets(buf, bufSize, fp) != NULL);
		skip = (good && (buf[0] == '#'));
	} while (good && skip);
	if (good)
	{
		int		len = strlen( buf);
		if ( buf[len - 1] == '\r' || buf[len - 1] == '\n')
			buf[len - 1] = '\0';
	}
    return good;
}

static mStatus RegisterServicesInFile(const char *filePath)
{
    mStatus     status = mStatus_NoError;
    FILE *      fp = fopen(filePath, "r");
    int         junk;
    
    if (fp == NULL) {
        status = mStatus_UnknownErr;
    }
    if (status == mStatus_NoError) {
        mDNSBool good = mDNStrue;
        do {
			int         ch;
			char name[256];
			char type[256];
			const char *dom = kDefaultServiceDomain;
			char rawText[1024];
			mDNSu8  text[sizeof(RDataBody)];
			unsigned int textLen = 0;
			char port[256];

            // Skip over any blank lines.
            do ch = fgetc(fp); while ( ch == '\n' || ch == '\r' );
            if (ch != EOF) good = (ungetc(ch, fp) == ch);
            
            // Read three lines, check them for validity, and register the service.
			good = ReadALine(name, sizeof(name), fp);               
			if (good) {
				good = ReadALine(type, sizeof(type), fp);
			}
			if (good) {
				char *p = type;
				while (*p && *p != ' ') p++;
				if (*p) {
					*p = 0;
					dom = p+1;
				}
			}
			if (good) {
				good = ReadALine(port, sizeof(port), fp);
			}
			if (good) {
				good =     CheckThatRichTextNameIsUsable(name, mDNSfalse)
						&& CheckThatServiceTypeIsUsable(type, mDNSfalse)
						&& CheckThatPortNumberIsUsable(atol(port), mDNSfalse);
			}
			if (good) {
				while (1) {
					int len;
					if (!ReadALine(rawText, sizeof(rawText), fp)) break;
					len = strlen(rawText);
					if (len <= 255)
						{
						unsigned int newlen = textLen + 1 + len;
						if (len == 0 || newlen >= sizeof(text)) break;
						text[textLen] = len;
						mDNSPlatformMemCopy(text + textLen + 1, rawText, len);
						textLen = newlen;
						}
					else
						fprintf(stderr, "%s: TXT attribute too long for name = %s, type = %s, port = %s\n", 
							gProgramName, name, type, port);
				}
			}
			if (good) {
				status = RegisterOneService(name, type, dom, text, textLen, atol(port));
				if (status != mStatus_NoError) {
					fprintf(stderr, "%s: Failed to register service, name = %s, type = %s, port = %s\n", 
							gProgramName, name, type, port);
					status = mStatus_NoError;       // keep reading
				}
			}
        } while (good && !feof(fp));

        if ( ! good ) {
            fprintf(stderr, "%s: Error reading service file %s\n", gProgramName, filePath);
        }
    }
    
    if (fp != NULL) {
        junk = fclose(fp);
        assert(junk == 0);
    }
    
    return status;
}

static mStatus RegisterOurServices(void)
{
    mStatus status;
    
    status = mStatus_NoError;
    if (gServiceName[0] != 0) {
        status = RegisterOneService(gServiceName, 
                                    gServiceType, 
                                    gServiceDomain, 
                                    gServiceText, gServiceTextLen, 
                                    gPortNumber);
    }
    if (status == mStatus_NoError && gServiceFile[0] != 0) {
        status = RegisterServicesInFile(gServiceFile);
    }
    return status;
}

static void DeregisterOurServices(void)
{
    PosixService *thisServ;
    int thisServID;
    
    while (gServiceList != NULL) {
        thisServ = gServiceList;
        gServiceList = thisServ->next;

        thisServID = thisServ->serviceID;
        
        mDNS_DeregisterService(&mDNSStorage, &thisServ->coreServ);

        if (gMDNSPlatformPosixVerboseLevel > 0) {
            fprintf(stderr, 
                    "%s: Deregistered service %d\n",
                    gProgramName, 
                    thisServ->serviceID);
        }
    }
}

#if COMPILER_LIKES_PRAGMA_MARK
#pragma mark **** Main
#endif

int main(int argc, char **argv)
{
    mStatus status;
    int     result;

    // Parse our command line arguments.  This won't come back if there's an error.
    
    ParseArguments(argc, argv);

    // If we're told to run as a daemon, then do that straight away.
    // Note that we don't treat the inability to create our PID 
    // file as an error.  Also note that we assign getpid to a long 
    // because printf has no format specified for pid_t.
    
    if (gDaemon) {
    	int result;
        if (gMDNSPlatformPosixVerboseLevel > 0) {
            fprintf(stderr, "%s: Starting in daemon mode\n", gProgramName);
        }
        result = daemon(0,0);
        if (result == 0) {
            FILE *fp;
            int  junk;
            
            fp = fopen(gPIDFile, "w");
            if (fp != NULL) {
                fprintf(fp, "%ld\n", (long) getpid());
                junk = fclose(fp);
                assert(junk == 0);
            }
        } else {
            fprintf(stderr, "%s: Could not run as daemon - exiting\n", gProgramName);
            exit(result);
        }
    } else {
        if (gMDNSPlatformPosixVerboseLevel > 0) {
            fprintf(stderr, "%s: Starting in foreground mode, PID %ld\n", gProgramName, (long) getpid());
        }
    }

    status = mDNS_Init(&mDNSStorage, &PlatformStorage,
    	mDNS_Init_NoCache, mDNS_Init_ZeroCacheSize,
    	mDNS_Init_AdvertiseLocalAddresses,
    	mDNS_Init_NoInitCallback, mDNS_Init_NoInitCallbackContext);
    if (status != mStatus_NoError) return(2);

	status = RegisterOurServices();
    if (status != mStatus_NoError) return(2);
    
    signal(SIGHUP,  HandleSigHup);      // SIGHUP has to be sent by kill -HUP <pid>
    signal(SIGINT,  HandleSigInt);      // SIGINT is what you get for a Ctrl-C
    signal(SIGQUIT, HandleSigQuit);     // SIGQUIT is what you get for a Ctrl-\ (indeed)
    signal(SIGUSR1, HandleSigUsr1);     // SIGUSR1 has to be sent by kill -USR1 <pid>

	while (!gStopNow)
		{
		int nfds = 0;
		fd_set readfds;
		struct timeval timeout;
		int result;
		
		// 1. Set up the fd_set as usual here.
		// This example client has no file descriptors of its own,
		// but a real application would call FD_SET to add them to the set here
		FD_ZERO(&readfds);
		
		// 2. Set up the timeout.
		// This example client has no other work it needs to be doing,
		// so we set an effectively infinite timeout
		timeout.tv_sec = 0x3FFFFFFF;
		timeout.tv_usec = 0;
		
		// 3. Give the mDNSPosix layer a chance to add its information to the fd_set and timeout
		mDNSPosixGetFDSet(&mDNSStorage, &nfds, &readfds, &timeout);
		
		// 4. Call select as normal
		verbosedebugf("select(%d, %d.%06d)", nfds, timeout.tv_sec, timeout.tv_usec);
		result = select(nfds, &readfds, NULL, NULL, &timeout);
		
		if (result < 0)
			{
			verbosedebugf("select() returned %d errno %d", result, errno);
			if (errno != EINTR) gStopNow = mDNStrue;
			else
				{
				if (gReceivedSigUsr1)
					{
					gReceivedSigUsr1 = mDNSfalse;
					gMDNSPlatformPosixVerboseLevel += 1;
					if (gMDNSPlatformPosixVerboseLevel > 2)
						gMDNSPlatformPosixVerboseLevel = 0;
					if ( gMDNSPlatformPosixVerboseLevel > 0 )
						fprintf(stderr, "\nVerbose level %d\n", gMDNSPlatformPosixVerboseLevel);
					}
				if (gReceivedSigHup)
					{
					if (gMDNSPlatformPosixVerboseLevel > 0)
						fprintf(stderr, "\nSIGHUP\n");
					gReceivedSigHup = mDNSfalse;
					DeregisterOurServices();
					status = mDNSPlatformPosixRefreshInterfaceList(&mDNSStorage);
					if (status != mStatus_NoError) break;
					status = RegisterOurServices();
					if (status != mStatus_NoError) break;
					}
				}
			}
		else
			{
			// 5. Call mDNSPosixProcessFDSet to let the mDNSPosix layer do its work
			mDNSPosixProcessFDSet(&mDNSStorage, &readfds);
			
			// 6. This example client has no other work it needs to be doing,
			// but a real client would do its work here
			// ... (do work) ...
			}
		}

	debugf("Exiting");
    
	DeregisterOurServices();
	mDNS_Close(&mDNSStorage);

    if (status == mStatus_NoError) {
        result = 0;
    } else {
        result = 2;
    }
    if ( (result != 0) || (gMDNSPlatformPosixVerboseLevel > 0) ) {
        fprintf(stderr, "%s: Finished with status %d, result %d\n", gProgramName, (int)status, result);
    }
    
    return result;
}
