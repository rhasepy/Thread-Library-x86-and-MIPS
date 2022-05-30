/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.

   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>

enum Thread_State_t
{
   READY,
   RUN,
   DEAD
};

class Thread
{
   private:

      Thread_State_t state;
      mem_addr program_counter;
      std::string name;
      int ID;
   
   public:

      mem_word R[R_LENGTH];

      Thread();
      Thread(std::string thread_name);
      Thread(const Thread& copy);

      const Thread& operator =(const Thread& rvalue);

      void setState(const Thread_State_t stat);
      void setPC(const mem_addr& program_counter);
      void setName(const std::string name);
      void setStack(const mem_word R[R_LENGTH]);
      void setID(const int ID);

      Thread_State_t getState() const;
      const mem_addr& getPC() const;
      std::string getName() const;
      int getID() const;
      
      void toString() const;
};

enum Lock_State_t
{
   LOCKED,
   UNLOCKED
};

class PThread_Mutex
{
   private:
      Lock_State_t has_own;
      int who;

   public:
      PThread_Mutex();

      bool lock(const Thread& who);
      bool unlock(const Thread& who);

      int getOwner() const;
};


/* Exported functions. */
void SPIM_timerHandler();

int do_syscall ();
void handle_exception ();

#define PRINT_INT_SYSCALL	      1
#define PRINT_FLOAT_SYSCALL	   2
#define PRINT_DOUBLE_SYSCALL	   3
#define PRINT_STRING_SYSCALL	   4

#define READ_INT_SYSCALL	      5
#define READ_FLOAT_SYSCALL	      6
#define READ_DOUBLE_SYSCALL	   7
#define READ_STRING_SYSCALL	   8

#define SBRK_SYSCALL		         9

#define EXIT_SYSCALL		         10

#define PRINT_CHARACTER_SYSCALL	11
#define READ_CHARACTER_SYSCALL	12

#define OPEN_SYSCALL		         13
#define READ_SYSCALL		         14
#define WRITE_SYSCALL		      15
#define CLOSE_SYSCALL		      16

#define EXIT2_SYSCALL		      17

/* Thread Syscalls */

#define INIT_SYSCAL        0  // OK
#define PTHREAD_CREATE     18 // OK
#define PTHREAD_JOIN       19 // OK
#define PTHREAD_EXIT       20 // OK
#define MUTEX_LOCK         21 // OK
#define MUTEX_UNLOCK       22 // OK
#define PROCESS_EXIT       23 // OK
#define PTHREAD_RETURN     24 // OK

/* Thread Syscalls */