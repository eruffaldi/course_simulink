/*
 * Real-Time Clock and Synchronization for Simulink
 * Emanuele Ruffaldi @ Scuola Superiore Sant'Anna 2010-2016
 *
 * Outputs:
 * - time absolute in epoch seconds
 * - slept time
 * 
 */
#define S_FUNCTION_NAME                 SyncTime
#define S_FUNCTION_LEVEL                2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"

/*
 * Specific header file(s) required by the legacy code function.
 */

// includes
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#else 
#ifdef WIN32
#include <windows.h>
#pragma comment(lib,"winmm.lib")
#endif
#endif

#ifndef WIN32
#define __int64 int64_t
#define _POSIX
#define DWORD uint32_t
#include <sys/time.h>
#include <unistd.h>
#endif

// compute
static double unixtimed_s()
{
#ifdef __APPLE__
  #if 1
  struct timeval TV;
  gettimeofday(&TV, NULL);
  return TV.tv_sec+TV.tv_usec/1000000.0;
  #else
  //mach_absolute_time only for relative time
  auto start = mach_absolute_time();
  mach_timebase_info_data_t    sTimebaseInfo;
  mach_timebase_info(&sTimebaseInfo);
  double orwl_timebase = sTimebaseInfo.numer/(double)sTimebaseInfo.denom;
  orwl_timebase /= 1000000; // nano to milliseconds
  return (uint64_t)(start * orwl_timebase);
  #endif
#else 
#ifdef WIN32
  __int64 wintime; 
   GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -= 116444736000000000;  //1jan1601 to 1jan1970
   return wintime / 10000000.0;           // seconds
   /*
    SYSTEMTIME time;
    GetSystemTime(&time);
    return  (time.wSeconds * 1000) + time.wMilliseconds;
  */
#else
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec + spec.tv_nsec/1E9.0;
#endif
#endif
}

#ifdef MATLAB_MEX_FILE
#define myPrintf mexPrintf
#else
#define myPrintf printf
#endif

#ifndef _POSIX

static LARGE_INTEGER m_freq;
static bool hires;
static double relstart;

#endif

static double epochstart;
static bool started=false;
static double next; // last relative

static double GetCountRel()
{
#ifndef _POSIX
    if (hires)
    {
        __int64 curtime;
        QueryPerformanceCounter( (LARGE_INTEGER *)&curtime );
        return (double)(curtime-relstart) / (double)m_freq.QuadPart;
    }
	  else
		  return ((double)(GetTickCount()-relstart)) / 1000.0;
#else
   return unixtimed_s()-epochstart;
#endif
}

static double GetCount()
{
#ifndef _POSIX
    return epochstart+GetCountRel();
#else
    return unixtimed_s();
#endif
}


/* Function: mdlInitializeSizes ===========================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{

  /*
   * Set the number of input ports. 
   */
  if (!ssSetNumInputPorts(S, 0)) return;

  /*
   * Set the number of output ports.
   */
  if (!ssSetNumOutputPorts(S, 2)) return;
  ssSetNumSFcnParams(S,1);
  
  /*
   * Configure the output port 1
   */
  ssSetOutputPortWidth(S, 0, 1);
  ssSetOutputPortComplexSignal(S, 0, COMPLEX_NO);
  ssSetOutputPortOptimOpts(S, 0, SS_REUSABLE_AND_LOCAL);
  ssSetOutputPortOutputExprInRTW(S, 0, 0);

  ssSetOutputPortWidth(S, 1, 1);
  ssSetOutputPortComplexSignal(S, 1, COMPLEX_NO);
  ssSetOutputPortOptimOpts(S, 1, SS_REUSABLE_AND_LOCAL);
  ssSetOutputPortOutputExprInRTW(S, 1, 0);
  
  /*
   * Set the number of sample time. 
   */
  ssSetNumSampleTimes(S, 1);

  /*
   * All options have the form SS_OPTION_<name> and are documented in
   * matlabroot/simulink/include/simstruc.h. The options should be
   * bitwise or'd together as in
   *   ssSetOptions(S, (SS_OPTION_name1 | SS_OPTION_name2))
   */
  ssSetOptions(S,SS_OPTION_CAN_BE_CALLED_CONDITIONALLY | SS_OPTION_EXCEPTION_FREE_CODE |SS_OPTION_WORKS_WITH_CODE_REUSE |SS_OPTION_DISALLOW_CONSTANT_SAMPLE_TIME);
}


#define MDL_START
#if defined(MDL_START)
/* Function: mdlStart =====================================================
 * Abstract:
 *    This function is called once at start of model execution. If you
 *    have states that should be initialized once, this is the place
 *    to do it.
 */
static void mdlStart(SimStruct *S)
{
  double dt = *(double*)mxGetData(ssGetSFcnParam(S,0));
#ifndef _POSIX  
  FILETIME abs;
    if(started)
        return;
  timeBeginPeriod(1);
  QueryPerformanceFrequency (&m_freq);
  hires = m_freq.QuadPart > 0;
    if (hires)
    {
      LARGE_INTEGER tmp;
        QueryPerformanceCounter( (LARGE_INTEGER *)&tmp );
        relstart = (double)(start) / (double)m_freq.QuadPart;
    }
  else
    {
        relstart = timeGetTime()/1000.0;
    }
#endif
  epochstart = unixtimed_s();
  started = true;
  next = epochstart + dt;    
}

#endif

/* Function: mdlOutputs ===================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block. Generally outputs are placed in the output vector(s),
 *    ssGetOutputPortSignal.
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
  double *outnow = (double *) ssGetOutputPortSignal(S, 0);
  double *slept = (double *) ssGetOutputPortSignal(S, 1);
  double now = GetCount();
  double dt = *(double*)mxGetData(ssGetSFcnParam(S,0));

  if(dt != 0)
  {   
    if(now < next)
    {
      DWORD dw = (DWORD)1000.0*(next-now);
      if(dw == 0)
        dw = 1;
#ifdef _POSIX
      usleep(dw*1000);
#else
      Sleep(dw);
#endif
      double post = GetCount();
      *slept = post-now;
      now = post;
    }
    next += dt;
    while(next < now)
      next += dt;
  } 
  *outnow = now;
}

/* Function: mdlTerminate =================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.
 */
static void mdlTerminate(SimStruct *S)
{
#ifndef _POSIX
  timeEndPeriod(1); // in modern windows is not more needed
#endif
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
  ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
  ssSetOffsetTime(S, 0, FIXED_IN_MINOR_STEP_OFFSET);  
}

/*
 * Required S-function trailer
 */
#ifdef MATLAB_MEX_FILE
# include "simulink.c"
#else
# include "cg_sfun.h"
#endif
