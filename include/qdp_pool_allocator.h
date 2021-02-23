// -*- C++ -*-

#ifndef QDP_POOL_ALLOCATOR
#define QDP_POOL_ALLOCATOR

#include <string>
#include <list>
#include <iostream>
#include <algorithm>

using namespace std;


namespace QDP
{


  template<class Allocator>
  class QDPPoolAllocator {
  public:
    struct entry_t;

    static QDPPoolAllocator& Instance();

    typedef typename std::list< entry_t >              listEntry_t;
    typedef std::list< typename  listEntry_t::iterator> listEntryIter_t;

  public:

    void suspend();
    void resume();
    
    void registerMemory();
    void unregisterMemory();

    void   printListPool();
    void   printPoolInfo();
    size_t getPoolSize();

    bool allocate( void** ptr, size_t n_bytes );

    void free(const void *mem);
    void setPoolSize(size_t s);

    void enableMemset(unsigned val);

    QDPPoolAllocator();
    ~QDPPoolAllocator();

    bool allocateInternalBuffer();
    
  private:
    friend class QDPCache;


    QDPPoolAllocator(const QDPPoolAllocator&);                 // Prevent copy-construction
    QDPPoolAllocator& operator=(const QDPPoolAllocator&);

  };


  template<class Allocator>
  void QDPPoolAllocator<Allocator>::registerMemory() {
    }


  template<class Allocator>
  void QDPPoolAllocator<Allocator>::unregisterMemory() {
    }


  template<class Allocator>
  QDPPoolAllocator<Allocator>& QDPPoolAllocator<Allocator>::Instance()
  {
    static QDPPoolAllocator singleton;
    return singleton;
  }


  template<class Allocator>
  struct QDPPoolAllocator<Allocator>::entry_t {
    void * ptr;
    size_t size;
    bool allocated;
  };


  template<class Allocator>
    QDPPoolAllocator<Allocator>::QDPPoolAllocator() {
    }


  template<class Allocator>
  QDPPoolAllocator<Allocator>::~QDPPoolAllocator()
  { 
  }



  



  template<class T>
  struct SizeNotAllocated: public std::binary_function< typename T::entry_t, size_t , bool > {
    bool operator () ( const typename T::entry_t & ent , const size_t & size ) const {
      return ( 
	      (!ent.allocated) &&
	      (ent.size >= size) 
	       );
    }
  };


  template<class Allocator>
  void QDPPoolAllocator<Allocator>::suspend()
  {
  }

  template<class Allocator>
  void QDPPoolAllocator<Allocator>::resume()
  {
  }


  template<class Allocator>
  bool QDPPoolAllocator<Allocator>::allocateInternalBuffer()
  {
    return true;
  }
    

  template<class Allocator>
  void QDPPoolAllocator<Allocator>::printPoolInfo() {
  }


  template<class Allocator>
  void QDPPoolAllocator<Allocator>::printListPool() {
  }


  template<class Allocator>
  bool QDPPoolAllocator<Allocator>::allocate( void ** ptr , size_t n_bytes ) {
    return Allocator::allocate(ptr, n_bytes);
  }




  template<class Allocator>
  void QDPPoolAllocator<Allocator>::free(const void *mem) {
    Allocator::free(mem);
  }



  template<class Allocator>
  void QDPPoolAllocator<Allocator>::setPoolSize(size_t s) {
  }

    template<class Allocator>
  void QDPPoolAllocator<Allocator>::enableMemset(unsigned val) {
    }


  template<class Allocator>
  size_t QDPPoolAllocator<Allocator>::getPoolSize() {
    return 0;
  }

} // namespace QDP



#endif


