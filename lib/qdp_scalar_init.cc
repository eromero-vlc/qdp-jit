
/*! @file
 * @brief Parscalar init routines
 * 
 * Initialization routines for parscalar implementation
 */

#include <stdlib.h>
#include <unistd.h>

#include "qdp.h"


#include "qdp_init.h"

#if defined(QDP_USE_COMM_SPLIT_INIT)
#error "Trying to use MPI comm split with scalar architecture"
#endif

namespace QDP {

namespace COUNT {
  int count = 0;
}
	

  //! Private flag for status
  static bool isInit = false;
  bool setPoolSize = false;
  bool setGeomP = false;
  bool setIOGeomP = false;
  multi1d<int> logical_geom(Nd);   // apriori logical geometry of the machine
  multi1d<int> logical_iogeom(Nd); // apriori logical 	
  static void setGammas();



#if 1
  int gamma_degrand_rossi[5][4][4][2] = 
    { { {{0,0}, {0,0}, {0,0},{0,-1}},
	{{0,0}, {0,0}, {0,-1},{0,0}},
	{{0,0}, {0,1},{0,0},{0,0}},
	{{0,1},{0,0}, {0,0},{0,0}} },

      { {{0,0}, {0,0}, {0,0},{-1,0}},
	{{0,0}, {0,0}, {1,0},{0,0}},
	{{0,0}, {1,0}, {0,0},{0,0}},
	{{-1,0},{0,0}, {0,0},{0,0}} },

      { {{0,0}, {0,0}, {0,-1},{0,0}},
	{{0,0}, {0,0}, {0,0},{0,1}},
	{{0,1}, {0,0}, {0,0},{0,0}},
	{{0,0}, {0,-1}, {0,0},{0,0}} },

      { {{0,0}, {0,0}, {1,0},{0,0}},
	{{0,0}, {0,0}, {0,0},{1,0}},
	{{1,0}, {0,0}, {0,0},{0,0}},
	{{0,0}, {1,0}, {0,0},{0,0}} },

      { {{1,0}, {0,0}, {0,0},{0,0}},
	{{0,0}, {1,0}, {0,0},{0,0}},
	{{0,0}, {0,0}, {1,0},{0,0}},
	{{0,0}, {0,0}, {0,0},{1,0}} } };

  SpinMatrix *QDP_Gamma_values = nullptr;

  extern SpinMatrix& Gamma(int i) {
    if (i<0 || i>15)
      QDP_error_exit("Gamma(%d) value out of range",i);
    if (!isInit) {
      std::cerr << "Gamma() used before QDP_init\n";
      exit(1);
    }
    if (!QDP_Gamma_values)
      QDP_error_exit("Gammas are not initialized!");
    //QDP_info("++++ returning gammas[%d]",i);
    //std::cout << gammas[i] << "\n";
    return QDP_Gamma_values[i];
  }
#endif

  //! Public flag for using the GPU or not
  bool QDPuseGPU = false;

  void QDP_startGPU(int dev)
  {
    std::cout << "Setting CUDA device to " << dev << "\n";
    CudaSetDevice( dev );
    QDP_info_primary("Getting GPU device properties");
    CudaGetDeviceProps();

    QDP_info_primary("Trigger GPU evaluation");
    QDPuseGPU=true;
    
    // Initialize the LLVM wrapper
    llvm_wrapper_init();

    // Set gammas
    setGammas();
  }

  //! Set the GPU device
  int QDP_setGPU()
  {
    int deviceCount;
    //int ret = 0;
    CudaGetDeviceCount(&deviceCount);
    if (deviceCount == 0) {
      QDP_error_exit("No CUDA devices found");
    }

    // Try MVapich fist
    char *rank = getenv( "MV2_COMM_WORLD_LOCAL_RANK"  );

    // Try OpenMPI
    if( ! rank ) {
       rank = getenv( "OMPI_COMM_WORLD_LOCAL_RANK" );
    } 

    int dev=0;
    if (rank) {
      int local_rank = atoi(rank);
      dev = local_rank % deviceCount;
    } else {
      if ( DeviceParams::Instance().getDefaultGPU() == -1 )
	{
	  std::cerr << "Couldnt determine local rank. Selecting device 0. In a multi-GPU per node run this is not what one wants.\n";
	  dev = 0;
	}
      else
	{
	  dev = DeviceParams::Instance().getDefaultGPU();
	  std::cerr << "Couldnt determine local rank. Selecting device " << dev << " as per user request.\n";
	}
#if 0
      // we don't have an initialized QMP at this point
       std::cerr << "Couldnt determine local rank. Selecting device based on global rank \n";
       std::cerr << "Please ensure that ranks increase fastest within the node for this to work \n";
       int rank_QMP = QMP_get_node_number();
       dev = rank_QMP % deviceCount;
#endif
    }

    return dev;
  }


  void QDP_initialize(int *argc, char ***argv) 
  {
    QDP_initialize_CUDA(argc, argv);
    int dev = QDP_setGPU();
    QDP_initialize_QMP(argc, argv);
    QDP_startGPU(dev);
  }

  static void setGammas() {
    // QDP_info_primary("Setting gamma matrices");

    SpinMatrix dgr[5];
    for (int i = 0; i < 5; i++) {
      for (int s = 0; s < 4; s++) {
        for (int s2 = 0; s2 < 4; s2++) {
          dgr[i].elem().elem(s, s2).elem().real() =
              (float)gamma_degrand_rossi[i][s2][s][0];
          dgr[i].elem().elem(s, s2).elem().imag() =
              (float)gamma_degrand_rossi[i][s2][s][1];
        }
      }
      // std::cout << i << "\n" << dgr[i] << "\n";
    }
    // QDP_info_primary("Finished setting gamma matrices");
    // QDP_info_primary("Multiplying gamma matrices");

    QDP_Gamma_values = new SpinMatrix[Ns*Ns];
    QDP_Gamma_values[0] = dgr[4]; // Unity
    for (int i = 1; i < 16; i++) {
      zero_rep(QDP_Gamma_values[i]);
      bool first = true;
      // std::cout << "gamma value " << i << " ";
      for (int q = 0; q < 4; q++) {
        if (i & (1 << q)) {
          // std::cout << q << " ";
          if (first)
            QDP_Gamma_values[i] = dgr[q];
          else
            QDP_Gamma_values[i] = QDP_Gamma_values[i] * dgr[q];
          first = false;
        }
      }
      // std::cout << "\n" << QDP_Gamma_values[i] << "\n";
    }
    // QDP_info_primary("Finished multiplying gamma matrices");
  }

  //! Turn on the machine
  void QDP_initialize_CUDA(int *argc, char ***argv)
  {
    if (sizeof(bool) != 1)
      {
	std::cout << "Error: sizeof(bool) == " << sizeof(bool) << "   (1 is required)" << endl;
	exit(1);
      }

    if (isInit)
      {
	QDPIO::cerr << "QDP already inited" << endl;
	QDP_abort(1);
      }

    CudaInit();

    // This defaults to mvapich2
#if 0
    // This is deprecated - direct envvars try several variables
    DeviceParams::Instance().setENVVAR("MV2_COMM_WORLD_LOCAL_RANK");
#endif 
		
    //
    // Process command line
    //
		
    // Look for help
    bool help_flag = false;
    for (int i=0; i<*argc; i++) 
      {
	if (strcmp((*argv)[i], "-h")==0)
	  help_flag = true;
      }
		
    setGeomP = false;
    logical_geom = 0;
		
    setIOGeomP = false;
    logical_iogeom = 0;
		
#ifdef USE_REMOTE_QIO
    int rtiP = 0;
#endif
    const int maxlen = 256;
    char rtinode[maxlen];
    strncpy(rtinode, "your_local_food_store", maxlen);
		
    // Usage
    if (Layout::primaryNode())  {
      if (help_flag) 
	{
	  fprintf(stderr,"Usage:    %s options\n",(*argv)[0]);
	  fprintf(stderr,"options:\n");
	  fprintf(stderr,"    -h        help\n");
#if defined(QDP_USE_PROFILING)   
	  fprintf(stderr,"    -p        %%d [%d] profile level\n", 
		  getProfileLevel());
#endif
				
	  // logical geometry info
	  fprintf(stderr,"    -geom     %%d");
	  for(int i=1; i < Nd; i++) 
	    fprintf(stderr," %%d");
				
	  fprintf(stderr," [-1");
	  for(int i=1; i < Nd; i++) 
	    fprintf(stderr,",-1");
	  fprintf(stderr,"] logical machine geometry\n");
				
#ifdef USE_REMOTE_QIO
	  fprintf(stderr,"    -cd       %%s [.] set working dir for QIO interface\n");
	  fprintf(stderr,"    -rti      %%d [%d] use run-time interface\n", 
		  rtiP);
	  fprintf(stderr,"    -rtinode  %%s [%s] run-time interface fileserver node\n", 
		  rtinode);
#endif
				
	  QDP_abort(1);
	}
    }


    for (int i=1; i<*argc; i++) 
      {
	if (strcmp((*argv)[i], "-sync")==0) 
	  {
	    DeviceParams::Instance().setSyncDevice(true);
	  }
	else if (strcmp((*argv)[i], "-sm")==0) 
	  {
	    int sm;
	    sscanf((*argv)[++i], "%d", &sm);
	    DeviceParams::Instance().setSM(sm);
	  }
	else if (strcmp((*argv)[i], "-gpudirect")==0) 
	  {
	    DeviceParams::Instance().setGPUDirect(true);
	  }
	else if (strcmp((*argv)[i], "-envvar")==0) 
	  {
	    char buffer[1024];
	    sscanf((*argv)[++i],"%s",&buffer[0]);
	    DeviceParams::Instance().setENVVAR(buffer);
	  }
	else if (strcmp((*argv)[i], "-poolsize")==0) 
	  {
	    float f;
	    char c;
	    sscanf((*argv)[++i],"%f%c",&f,&c);
	    double mul;
	    switch (tolower(c)) {
	    case 'k': 
	      mul=1024.; 
	      break;
	    case 'm': 
	      mul=1024.*1024; 
	      break;
	    case 'g': 
	      mul=1024.*1024*1024; 
	      break;
	    case 't':
	      mul=1024.*1024*1024*1024;
	      break;
	    case '\0':
	      break;
	    default:
	      QDP_error_exit("unknown multiplication factor");
	    }
	    size_t val = (size_t)((double)(f) * mul);

	    //CUDADevicePoolAllocator::Instance().setPoolSize(val);
	    //QDP_get_global_cache().get_allocator().setPoolSize(val);
	    QDP_get_global_cache().setPoolSize(val);
	    
	    setPoolSize = true;
	  }
	else if (strcmp((*argv)[i], "-llvm-opt")==0) 
	  {
	    char tmp[1024];
	    sscanf((*argv)[++i], "%s", &tmp[0]);
	    llvm_set_opt(tmp);
	  }
	else if (strcmp((*argv)[i], "-ptxdb")==0) 
	  {
	    char tmp[1024];
	    sscanf((*argv)[++i], "%s", &tmp[0]);
	    llvm_set_ptxdb(tmp);
	  }
	else if (strcmp((*argv)[i], "-defaultgpu")==0) 
	  {
	    int ngpu;
	    sscanf((*argv)[++i], "%d", &ngpu);
	    DeviceParams::Instance().setDefaultGPU(ngpu);
	    std::cout << "Default GPU set to " << ngpu << "\n";
	  }
	else if (strcmp((*argv)[i], "-geom")==0) 
	  {
	    setGeomP = true;
	    for(int j=0; j < Nd; j++) 
	      {
		int uu;
		sscanf((*argv)[++i], "%d", &uu);
		logical_geom[j] = uu;
	      }
	  }
	else if (strcmp((*argv)[i], "-iogeom")==0) 
	  {
	    setIOGeomP = true;
	    for(int j=0; j < Nd; j++) 
	      {
		int uu;
		sscanf((*argv)[++i], "%d", &uu);
		logical_iogeom[j] = uu;
	      }
	  }
#ifdef USE_REMOTE_QIO
	else if (strcmp((*argv)[i], "-cd")==0) 
	  {
	    /* push the dir into the environment vars so qio.c can pick it up */
	    setenv("QHOSTDIR", (*argv)[++i], 0);
	  }
	else if (strcmp((*argv)[i], "-rti")==0) 
	  {
	    sscanf((*argv)[++i], "%d", &rtiP);
	  }
	else if (strcmp((*argv)[i], "-rtinode")==0) 
	  {
	    int n = strlen((*argv)[++i]);
	    if (n >= maxlen)
	      {
		QDPIO::cerr << __func__ << ": rtinode name too long" << endl;
		QDP_abort(1);
	      }
	    sscanf((*argv)[i], "%s", rtinode);
	  }
#endif
#if 0
	else 
	  {
	    QDPIO::cerr << __func__ << ": Unknown argument = " << (*argv)[i] << endl;
	    QDP_abort(1);
	  }
#endif
			
	if (i >= *argc) 
	  {
	    QDPIO::cerr << __func__ << ": missing argument at the end" << endl;
	    QDP_abort(1);
	  }
      }
		

    if (!setPoolSize) {
      // It'll be set later in CudaGetDeviceProps
      //QDP_error_exit("Run-time argument -poolsize <size> missing. Please consult README.");
    }

		
#if QDP_DEBUG >= 1
    // Print command line args
    for (int i=0; i<*argc; i++) 
      QDP_info("QDP_init: arg[%d] = XX%sXX",i,(*argv)[i]);
#endif
		
  }




		// -------------------------------------------------------------------------------------------

	void QDP_initialize_QMP(int *argc, char ***argv)
	{
	  Layout::init();   // setup extremely basic functionality in Layout
		
	  isInit = true;

	  // Initialize the LLVM wrapper
	  //llvm_wrapper_init();
		
	  // initialize the global streams
	  QDPIO::cin.init(&std::cin);
	  QDPIO::cout.init(&std::cout);
	  QDPIO::cerr.init(&std::cerr);
		
	  initProfile(__FILE__, __func__, __LINE__);
		
	  QDPIO::cout << "Initialize done" << std::endl;
	}


	
	//! Is the machine initialized?
	bool QDP_isInitialized() {return isInit;}
	
	//! Turn off the machine
	void QDP_finalize()
	{
		if ( ! QDP_isInitialized() )
		{
			QDPIO::cerr << "QDP is not inited" << std::endl;
			QDP_abort(1);
		}
		
		// Deallocate gammas
		delete [] QDP_Gamma_values;
		QDP_Gamma_values = nullptr;
		
		QDPIO::cout << "------------------\n";
		QDPIO::cout << "-- JIT statistics:\n";
		QDPIO::cout << "------------------\n";
		QDPIO::cout << "lattices changed to device layout:     " << get_jit_stats_lattice2dev() << "\n";
		QDPIO::cout << "lattices changed to host layout:       " << get_jit_stats_lattice2host() << "\n";
		QDPIO::cout << "functions jit-compiled:                " << get_jit_stats_jitted() << "\n";
		if (get_ptx_db_enabled())
		  {
		    QDPIO::cout << "PTX DB, file:                          " << get_ptx_db_fname() << "\n";
		    QDPIO::cout << "PTX DB, size (number of functions):    " << get_ptx_db_size() << "\n";
		  }
		else
		  {
		    QDPIO::cout << "PTX DB: (not used)\n";
		  }
		
		FnMapRsrcMatrix::Instance().cleanup();

#if defined(QDP_USE_HDF5)
                H5close();
#endif

		// Finalize pool allocator
		Allocator::theQDPAllocator::DestroySingleton();

		printProfile();
		
		isInit = false;
	}
	
	//! Panic button
	void QDP_abort(int status)
	{
	}
	
	//! Resumes QDP communications
	void QDP_resume() {}
	
	//! Suspends QDP communications
	void QDP_suspend() {}
	
	
} // namespace QDP;
