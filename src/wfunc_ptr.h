#pragma once
#include <cstdint>
#include <ostream>
#include "p32.h"
#include "ppc.h"

#pragma pack(push, 1)

template<typename ReturnType, typename... Args>
struct wfunc_ptr
{
   wfunc_ptr() :
      address(0)
   {
   }

   wfunc_ptr(std::nullptr_t) :
      address(0)
   {
   }

   wfunc_ptr(int addr) :
      address(addr)
   {
   }

   wfunc_ptr(uint32_t addr) :
      address(addr)
   {
   }

   wfunc_ptr(p32<void> addr) :
      address(addr)
   {
   }

   operator uint32_t() const
   {
      return address;
   }

   uint32_t address;

   ReturnType call(ThreadState *state, Args... args);
};

#pragma pack(pop)


template<typename ReturnType, typename... Args>
static inline std::ostream&
operator<<(std::ostream& os, const wfunc_ptr<ReturnType, Args...>& val)
{
   return os << static_cast<uint32_t>(val);
}

// Late include of ppcinvoke due to circular reference of wfunc_ptr inside arg_converter.
#include "ppcinvoke.h"
#include "interpreter.h"

template<typename ReturnType, typename... Args>
ReturnType wfunc_ptr<ReturnType, Args...>::call(ThreadState *state, Args... args) {
   // Push args
   ppctypes::applyArguments(state, args...);

   // Save LR to stack
   state->gpr[1] -= 4;
   gMemory.write(state->gpr[1], state->lr);

   // Save NIA to stack
   state->gpr[1] -= 4;
   gMemory.write(state->gpr[1], state->nia);

   // Set LR to return to emulator code
   state->lr = CALLBACK_ADDR;

   // Update NIA to Target
   state->cia = 0;
   state->nia = address;

   gInterpreter.execute(state);

   // Restore NIA from Stack
   state->nia = gMemory.read<uint32_t>(state->gpr[1]);
   state->gpr[1] += 4;

   // Restore LR from Stack
   state->lr = gMemory.read<uint32_t>(state->gpr[1]);
   state->gpr[1] += 4;

   // Return the result
   return ppctypes::getResult<ReturnType>(state);
}
