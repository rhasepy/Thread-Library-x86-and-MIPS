/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
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


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "syscall.h"

#include "run.h"
#include "spim-utils.h"
#include "scanner.h"
#include "data.h"

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif


// My microkernel model extensions
#include <iostream>
#include <deque>

std::deque <Thread> threads;
Thread current_thread;
PThread_Mutex mutex;

bool get_kernel_up = false;

// For testing
bool print_msg_handler = false;
bool print_msg_handler2 = false;
// My microkernel model extensions

/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */

PThread_Mutex::PThread_Mutex() 
{
  this -> who = -1;
  this -> has_own = UNLOCKED; 
}

bool PThread_Mutex::lock(const Thread& who) 
{
  if(this -> who == -1) {

    this -> has_own = LOCKED;
    this -> who = who.getID();
    return true;
  }

  return false;
}

bool PThread_Mutex::unlock(const Thread& who) 
{
  if(this -> who != who.getID()) {
    return false;
  }

  this -> has_own = UNLOCKED;
  this -> who = -1;
  return true; 
}

int PThread_Mutex::getOwner() const { return this ->who; }

Thread::Thread() { this->program_counter = current_text_pc(); }
Thread::Thread(std::string thread_name)
{
  this -> program_counter = current_text_pc();

  char* temp = (char*) calloc(sizeof(char), thread_name.size() + 1);
  
  for(size_t i = 0; i < thread_name.size(); ++i)
    temp[i] = thread_name[i];
  
  read_assembly_file(temp);
  this -> name = thread_name;
}

Thread::Thread(const Thread& copy)
{
  this -> ID = copy.getID();
  this -> name = copy.getName();
  this -> program_counter = copy.getPC();
  this -> state = copy.getState();
      
  for(int i = 0; i < R_LENGTH; ++i)
    this -> R[i] = copy.R[i];
}

const Thread& Thread::operator =(const Thread& rvalue)
{
  this -> ID = rvalue.getID();
  this -> name = rvalue.getName();
  this -> program_counter = rvalue.getPC();
  this -> state = rvalue.getState();
      
  for(int i = 0; i < R_LENGTH; ++i)
    this -> R[i] = rvalue.R[i];

  return (*this);
}

void Thread::setState(const Thread_State_t state) { this -> state = state; }
void Thread::setPC(const mem_addr& program_counter) { this -> program_counter = program_counter; }
void Thread::setName(const std::string name) { this -> name = name; }
void Thread::setID(const int ID) { this -> ID = ID; }
void Thread::setStack(const mem_word R[R_LENGTH])
{
    for(int i = 0; i < R_LENGTH; ++i)
      this -> R[i] = R[i];
}

Thread_State_t Thread::getState() const { return this->state; }
const mem_addr& Thread::getPC() const { return this->program_counter; }
std::string Thread::getName() const { return this->name; }
int Thread::getID() const { return this->ID; }

void
Thread::toString() const
{
    std::cout << "----------------------------------------" << std::endl;
    printf("%-20s %-10s %-5s\n", "Name", "ID", "PC");
    printf("%-20s %-10d %-5d\n", this->name.c_str(), this->getID(), this->getPC());
    std::cout << "----------------------------------------" << std::endl;
}

bool
able_switch()
{
    int count = 0;

    for(size_t i = 0; i < threads.size(); ++i)
      if(threads[i].getState() != DEAD)
        ++count;

    return count != 1;
}

void
context_switch()
{
    if(false == get_kernel_up)
      return;
    
    std::cout << "\n**********************************" << std::endl;
    std::cout << "SPIMOS Message: Context Switch!" << std::endl;
    std::cout << "**********************************" << std::endl;

    std::cout << "Old Thread: " << std::endl;
    threads.front().toString();
    
    do 
    {
      Thread temp = threads.front();
      threads.pop_front();
      threads.push_back(temp);

      if(threads.back().getState() == RUN)
        threads[threads.size() - 1].setState(READY);

    } while(threads.front().getState() == DEAD);

    threads[0].setState(RUN);
    current_thread = threads.front();

    std::cout << std::endl << "New Thread: " << std::endl;
    current_thread.toString();

    for(int i = 0; i < R_LENGTH; ++i)
      R[i] = current_thread.R[i];

    PC = current_thread.getPC();
}

void
SPIM_timerHandler()
{
    if(false == get_kernel_up) return;
    else if(threads.size() < 2) return;
    else if(false == able_switch() && current_thread.getID() == 0) return; 
    //else if(false == able_switch()) return;
    
    threads.front().setPC(PC);
    threads.front().setStack(R);

    print_msg_handler2 = false;

    context_switch();
}

void
create_init_process()
{
    Thread kernel_thread("init.s");
    kernel_thread.setStack(R);
    kernel_thread.setID(0);
    kernel_thread.setName("init_process");
    kernel_thread.setState(RUN);

    kernel_thread.R[REG_V0] = R[REG_A0];
    R[REG_V0] = kernel_thread.R[REG_V0];

    threads.push_front(kernel_thread);
    current_thread = threads.front();
}

void
_pthread_create()
{
    std::string thread_name = "Thread ";
    int thread_id = threads.size();

    while(thread_id > 0)
    {
      thread_name.push_back((thread_id % 10) + '0');
      thread_id /= 10;
    }

    Thread new_thread;

    new_thread.setStack(R);
    new_thread.setID(threads.size());
    new_thread.setName(thread_name); 
    new_thread.setPC((mem_addr)R[REG_A0]);
    new_thread.setState(READY);

    new_thread.R[REG_V0] = R[REG_A0];
    new_thread.R[REG_A1] = R[REG_A1];
    R[REG_V0] = new_thread.R[REG_V0];

    R[REG_V0] = new_thread.getID();

    threads.push_back(new_thread);
}

void
_pthread_join(int thread_id)
{
  if(false == get_kernel_up) {
    std::cout << "Kernel (init) not created..."
    << std::endl << "Shutdown...\n" << std::endl;
    exit(-1);
  }

  else if(thread_id == 0) {
    std::cout << "The Main Thread can not join!!!" << std::endl;
    std::cout << "Kernel Pan!c..." << std::endl;
    exit(-1);
  }

  //If the thread is running, 
  // it should stay stuck there.
  for(size_t i = 0; i < threads.size(); ++i) {

    if(thread_id == threads[i].getID()) {

      if(DEAD == threads[i].getState())
        return;
      else {
        PC = PC - BYTES_PER_WORD;
      }
    }
  }
}

void
_exit_(int param)
{
  if(param == -1) {
    std::cout << "STDERR: exit main thread..." << std::endl;
  }

  for(size_t i = 0; i < threads.size(); ++i)
    threads[i].setState(DEAD);
  
  exit(1);
}

void
_pthread_exit(int thread_id)
{
  if(false == get_kernel_up) {
    std::cout << "Kernel (init) not created..."
    << std::endl << "Kernel Pan!c...\nShutdown...\n" << std::endl;
    exit(-1);

  } else if(thread_id == 0) {
    _exit_(-1);
  }

  for (size_t i = 0; i < threads.size(); i++) {
    if(thread_id == threads[i].getID())
      threads[i].setState(DEAD);
  }
}

bool
_pthread_mutex_lock() { 

  bool isLock = mutex.lock(current_thread);

  if((false == isLock) && (false == print_msg_handler))
  {
    std::cout << "Mutex has already locked..." 
    << std::endl  
    << "Thread " << current_thread.getID() 
    << " waiting to unlock mutex..." << std::endl;

    std::cout << std::endl
    << "Micro kernel 2 Race condition: " << std::endl;
    std::cout << "Note that: "
    << "if mutex was not used it would be race condition." << std::endl
    << "There would be corruptions in the buffer before the process is finished or incomplete." << std::endl;
  
    print_msg_handler = true;
  }

  if(true == isLock)
  {
    std::cout << "Thread "
    << current_thread.getID()
    << " take and lock mutex..."
    << std::endl;
  }

  return isLock;
}

bool
_pthread_mutex_unlock() { 
  bool isUnlock = mutex.unlock(current_thread);

  if(true == isUnlock)
  {
    std::cout << "Thread "
    << current_thread.getID()
    << " bring and unlock mutex..." 
    << std::endl;

    print_msg_handler = false;
  }

  return isUnlock;
}

int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

  /* Syscalls for the source-language version of SPIM.  These are easier to
     use than the real syscall and are portable to non-MIPS operating
     systems. */

  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      write_output (console_out, "%d", R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);

	write_output (console_out, "%.8f", val);
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	R[REG_RES] = atol (str);
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
	break;
      }

    case READ_STRING_SYSCALL:
      {
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = data_top;
	expand_data (R[REG_A0]);
	R[REG_RES] = x;
	data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];

	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	R[REG_RES] = (long) str[0];
	break;
      }

    case EXIT_SYSCALL:
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
	R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	R[REG_RES] = _close(R[REG_A0]);
#else
	R[REG_RES] = close(R[REG_A0]);
#endif
	break;
      }

      case INIT_SYSCAL:
      {
        create_init_process();
        PC = current_thread.getPC() - BYTES_PER_WORD;
        get_kernel_up = true;
        break;
      }

      case PTHREAD_CREATE:
      {
        _pthread_create();
        break;
      }

      case PTHREAD_JOIN:
      {
        if(false == print_msg_handler2) {
          
          std::cout << std::endl
          << "Inıt Process Actions: "
          << std::endl;

          print_msg_handler2 = true;
        }
        _pthread_join(R[REG_A0]);
        break;
      }

      case PTHREAD_EXIT:
      {
        _pthread_exit(current_thread.getID());
        break;
      }

      case MUTEX_LOCK:
      {
        if(!_pthread_mutex_lock()) PC = PC - BYTES_PER_WORD;
        break;
      }

      case MUTEX_UNLOCK:
      {
        if(!_pthread_mutex_unlock()) {
          std::cout << "Mutex Unlock Error: "
          << "You are trying to unlock a mutex that you did not lock." 
          << std::endl
          << "Undefined Behavior Kernel Pan!c"
          << std::endl;
          exit(-1);
        }
        break;
      }
 
      case PROCESS_EXIT:
      {
        _exit_(1);
        break;
      }

      case PTHREAD_RETURN:
      {
        std::cout << "Thread " 
        << current_thread.getID()
        << " return...\n"
        << std::endl;

        threads[0].setState(DEAD);
        current_thread.setState(DEAD);

        break;
      }

    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}

void
handle_exception ()
{
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}