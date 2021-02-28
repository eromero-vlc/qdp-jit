// -*- C++ -*-

#ifndef QDP_INIT
#define QDP_INIT

#ifndef QDP_CONFIG_H
#include "qdp_config.h"
#endif

/*! \file
 * \brief Routines for top level QDP management
 *
 * Fundamental routines for turning on/off and inserting/extracting
 * variables.
 */

// Info/error routines
namespace QDP {

#ifdef QDP_IS_QDPJIT
  extern bool QDPuseGPU;
  extern bool setPoolSize;
  int QDP_setGPU(); // returns the CUDA device number that was actually set
#ifdef QDP_USE_COMM_SPLIT_INIT
  int QDP_setGPUCommSplit();
#endif
  void QDP_startGPU(int);
  #define QDP_FIX_GPU_SETTING
#endif

//! Turn on the machine
void QDP_initialize (int *argc, char ***argv);
void QDP_initialize_CUDA (int *argc, char ***argv);
void QDP_initialize_QMP (int *argc, char ***argv);

//! Is the machine initialized?
bool QDP_isInitialized ();

//! Turn off the machine
void QDP_finalize ();

//! Panic button
void QDP_abort (int status);

//! Simple information display routine
int  QDP_info (const char* format, ...);

//! Simple information display routine
int  QDP_info_primary (const char* format, ...);

//! Simple debug display routine
int  QDP_debug (const char* format, ...);

//! Simple deep debug display routine
int  QDP_debug_deep (const char* format, ...);

//! Simple error display routine
int  QDP_error (const char* format, ...);

//! Simple error display and abort routine
void QDP_error_exit (const char *format, ...);

//! Resumes QDP communications
void QDP_resume();

//! Suspends QDP communications
void QDP_suspend();


  template<class T>
  void printme() {
    std::cout << __PRETTY_FUNCTION__ << "\n";
  }
  template<class T>
  void printme(const std::string& txt) {
    std::cout << txt << " " << __PRETTY_FUNCTION__ << "\n";
  }


}

#endif
