#ifndef POOL_ALLOCATOR_H_INCLUDED
#define POOL_ALLOCATOR_H_INCLUDED

/** 
  @file
  @author Erik Boström <cewbostrom@gmail.com>
  @date 15.8.2015
  
  @section LICENSE
  
  MIT License (MIT)

  Copyright (c) 2015 Erik Boström

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  
  @section DESCRIPTION
  
  A simple pool allocator template. Improves allocation/deallocation times and locality
  of reference for the pooled objects.
  
  usage:
  
  simply declare a PoolAllocator object and overload the new and delete operators for
  the object you want to pool.
*/

/**
  @brief Pool allocator template.
  @param CellSize Size in bytes of each object allocated in the pool.
  @param PoolSize Number of objects per pool.
  @param MaxPools Maximum number of pools. When memory in the existing pools runs out
  PoolAllocator will allocate more memory with operator new. This limits the number of
  pools the allocator can allocate. default: 64
*/
template<unsigned CellSize, unsigned PoolSize, unsigned MaxPools = 64>
class PoolAllocator
{
private:

  union Block
  {
    Block* next;
    char padding[CellSize];
    //T t;
  };

  Block* memory_[MaxPools];
  Block* next_;

public:

  /**
    @brief Allocates memory from the pool
    
    To pool a specific type of object overload it's new operator to use this function.
    If there is not enough memory in the pool it will allocate another pool. If the
    number of pools exceeds MaxPools it will fail disgracefully.
    
    @return Pointer to a free cell of memory in the pool.
  */
  void* allocate()
  {
    if(next_ == nullptr)
      reserve();

    Block* block = next_;
    next_ = next_->next;
    return (void*)block;
  }

  /**
    @brief Allocates memory from the pool
    
    To pool a specific type of object overload it's new operator to use this function.
    If there is not enough memory in the pool it will allocate another pool. If the
    number of pools exceeds MaxPools it will fail disgracefully.
    
    @param size This argument is simply ignored.
    @return Memory adress of a free cell in the pool.
  */
  void* allocate(size_t size)
  {
    if(next_ == nullptr)
      reserve();

    Block* block = next_;
    next_ = next_->next;
    return (void*)block;
  }

  /**
    @brief deallocates memory
    
    To pool a specific type of object overload it's delete operaator to use this
    function.
    
    @param block Pointer to the cell to free. This must point to a cell allocated with
    the same PoolAllocator objects allocate function.
  */
  void deallocate(void* block)
  {
    ((Block*)block)->next = next_;
    next_ = (Block*)block;
  }

  /**
    @brief Preallocates a number of pools.
    
    Allocating more pools than MaxPools will fail disgracefully.
    
    @param num_pools Number of pools to preallocate. Default: 1
  */
  void reserve(unsigned num_pools = 1)
  {
    for(auto& chunk: memory_)
    {
      if(chunk == nullptr)
      {
        chunk = new Block[PoolSize];

        for(unsigned m = 0; m < PoolSize - 1; ++m)
          chunk[m].next = &chunk[m + 1];

        chunk[PoolSize - 1].next = next_;
        next_ = chunk;
        break;
      }
    }
    if(num_pools != 1) reserve(num_pools - 1);
  }

  /**
    @brief Deallocates all pools.
    
    Destructors of the objects already allocated will not be called. All pointers to
    memory in the pool will be invalidated.
  */
  void clear()
  {
    next_ = nullptr;

    for(auto& chunk: memory_)
    {
      delete chunk;
      chunk = nullptr;
    }
  }

  PoolAllocator()
  {
    next_ = nullptr;
    for(auto& c: memory_)
      c = nullptr;
  }
  PoolAllocator(const PoolAllocator&) = delete;
  void operator=(const PoolAllocator&) = delete;
  
  ~PoolAllocator()
  {
    this->clear();
  }
};

#endif