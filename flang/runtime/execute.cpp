//===-- runtime/execute.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flang/Runtime/execute.h"
#include "environment.h"
#include "stat.h"
#include "terminator.h"
#include "tools.h"
#include "flang/Runtime/descriptor.h"
#include <cstdlib>
#include <future>
#include <limits>
#ifdef _WIN32
#define LEAN_AND_MEAN
#define NOMINMAX
#include <stdio.h>
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

namespace Fortran::runtime {

// cmdstat specified in 16.9.73
// −1 if the processor does not support command line execution,
// a processor-dependent positive value if an error condition occurs
// −2 if no error condition occurs but WAIT is present with the value false
// and the processor does not support asynchronous execution. Otherwise it is
// assigned the value 0
enum CMD_STAT {
  ASYNC_NO_SUPPORT_ERR = -2,
  NO_SUPPORT_ERR = -1,
  CMD_EXECUTED = 0,
  FORK_ERR = 1,
  EXECL_ERR = 2,
  INVALID_CL_ERR = 3,
  SIGNAL_ERR = 4
};

static bool IsValidCharDescriptor(const Descriptor *value) {
  return value && value->IsAllocated() &&
      value->type() == TypeCode(TypeCategory::Character, 1) &&
      value->rank() == 0;
}

static bool IsValidIntDescriptor(const Descriptor *length) {
  auto typeCode{length->type().GetCategoryAndKind()};
  // Check that our descriptor is allocated and is a scalar integer with
  // kind != 1 (i.e. with a large enough decimal exponent range).
  return length->IsAllocated() && length->rank() == 0 &&
      length->type().IsInteger() && typeCode && typeCode->second != 1;
}

void CopyToDescriptor(const Descriptor &value, const char *rawValue,
    std::int64_t rawValueLength, std::size_t offset = 0) {
  std::int64_t toCopy{std::min(rawValueLength,
      static_cast<std::int64_t>(value.ElementBytes() - offset))};

  std::memcpy(value.OffsetElement(offset), rawValue, toCopy);
}

void CheckAndCopyToDescriptor(const Descriptor *value, const char *rawValue,
    std::int64_t rawValueLength, std::size_t offset = 0) {
  if (value) {
    CopyToDescriptor(*value, rawValue, rawValueLength, offset);
  }
}

static void StoreIntToDescriptor(
    const Descriptor *intVal, std::int64_t value, Terminator &terminator) {
  auto typeCode{intVal->type().GetCategoryAndKind()};
  int kind{typeCode->second};
  Fortran::runtime::ApplyIntegerKind<Fortran::runtime::StoreIntegerAt, void>(
      kind, terminator, *intVal, /* atIndex = */ 0, value);
}

static void CheckAndStoreIntToDescriptor(
    const Descriptor *intVal, std::int64_t value, Terminator &terminator) {
  if (intVal) {
    StoreIntToDescriptor(intVal, value, terminator);
  }
}

template <int KIND> struct FitsInIntegerKind {
  bool operator()([[maybe_unused]] std::int64_t value) {
    if constexpr (KIND >= 8) {
      return true;
    } else {
      return value <= std::numeric_limits<Fortran::runtime::CppTypeFor<
                          Fortran::common::TypeCategory::Integer, KIND>>::max();
    }
  }
};

// If a condition occurs that would assign a nonzero value to CMDSTAT but
// the CMDSTAT variable is not present, error termination is initiated.
int TerminationCheck(int status, const Descriptor *cmdstat,
    const Descriptor *cmdmsg, Terminator &terminator) {
  if (status == -1) {
    if (!cmdstat) {
      terminator.Crash("Execution error with system status code: %d", status);
    } else {
      CheckAndStoreIntToDescriptor(cmdstat, EXECL_ERR, terminator);
      CopyToDescriptor(*cmdmsg, "Execution error", 15);
    }
  }
#ifdef _WIN32
  // On WIN32 API std::system returns exit status directly
  int exitStatusVal{status};
  if (exitStatusVal == 1) {
#else
  int exitStatusVal{WEXITSTATUS(status)};
  if (exitStatusVal == 127 || exitStatusVal == 126) {
#endif
    if (!cmdstat) {
      terminator.Crash(
          "Invalid command quit with exit status code: %d", exitStatusVal);
    } else {
      CheckAndStoreIntToDescriptor(cmdstat, INVALID_CL_ERR, terminator);
      CopyToDescriptor(*cmdmsg, "Invalid command line", 20);
    }
  }
#if defined(WIFSIGNALED) && defined(WTERMSIG)
  if (WIFSIGNALED(status)) {
    if (!cmdstat) {
      terminator.Crash("killed by signal: %d", WTERMSIG(status));
    } else {
      CheckAndStoreIntToDescriptor(cmdstat, SIGNAL_ERR, terminator);
      CopyToDescriptor(*cmdmsg, "killed by signal", 18);
    }
  }
#endif
#if defined(WIFSTOPPED) && defined(WSTOPSIG)
  if (WIFSTOPPED(status)) {
    if (!cmdstat) {
      terminator.Crash("stopped by signal: %d", WSTOPSIG(status));
    } else {
      CheckAndStoreIntToDescriptor(cmdstat, SIGNAL_ERR, terminator);
      CopyToDescriptor(*cmdmsg, "stopped by signal", 17);
    }
  }
#endif
  return exitStatusVal;
}

const char *ensureNullTerminated(
    const char *str, size_t length, Terminator &terminator) {
  if (length < strlen(str)) {
    char *newCmd{(char *)malloc(length + 1)};
    if (newCmd == NULL) {
      terminator.Crash("Command not null-terminated, memory allocation failed "
                       "for null-terminated newCmd.");
    }

    strncpy(newCmd, str, length);
    newCmd[length] = '\0';
    return newCmd;
  } else {
    return str;
  }
}

void RTNAME(ExecuteCommandLine)(const Descriptor &command, bool wait,
    const Descriptor *exitstat, const Descriptor *cmdstat,
    const Descriptor *cmdmsg, const char *sourceFile, int line) {
  Terminator terminator{sourceFile, line};
  const char *newCmd{ensureNullTerminated(
      command.OffsetElement(), command.ElementBytes(), terminator)};

  if (exitstat) {
    RUNTIME_CHECK(terminator, IsValidIntDescriptor(exitstat));
    // If sync, assigned processor-dependent exit status. Otherwise unchanged
  }

  if (cmdstat) {
    RUNTIME_CHECK(terminator, IsValidIntDescriptor(cmdstat));
    // Assigned 0 as specifed in standard, if error then overwrite
    StoreIntToDescriptor(cmdstat, CMD_EXECUTED, terminator);
  }

  if (cmdmsg) {
    RUNTIME_CHECK(terminator, IsValidCharDescriptor(cmdmsg));
  }

  if (wait) {
    // either wait is not specified or wait is true: synchronous mode
    int status{std::system(newCmd)};
    int exitStatusVal{TerminationCheck(status, cmdstat, cmdmsg, terminator)};
    CheckAndStoreIntToDescriptor(exitstat, exitStatusVal, terminator);
  } else {
// Asynchronous mode
#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // append "cmd.exe /c " to the beginning of command
    const char *prefix{"cmd.exe /c "};
    char *newCmdWin{
        (char *)malloc(std::strlen(prefix) + std::strlen(newCmd) + 1)};
    if (newCmd != NULL) {
      std::strcpy(newCmdWin, prefix);
      std::strcat(newCmdWin, newCmd);
    } else {
      terminator.Crash("Memory allocation failed for newCmd");
    }

    // Convert the char to wide char
    const size_t sizeNeeded{mbstowcs(NULL, newCmdWin, 0) + 1};
    wchar_t *wcmd{new wchar_t[sizeNeeded]};
    if (std::mbstowcs(wcmd, newCmdWin, sizeNeeded) == static_cast<size_t>(-1)) {
      terminator.Crash("Char to wide char failed for newCmd");
    }
    free(newCmdWin);

    if (CreateProcess(nullptr, wcmd, nullptr, nullptr, FALSE, 0, nullptr,
            nullptr, &si, &pi)) {
      // Close handles so it will be removed when terminated
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    } else {
      if (!cmdstat) {
        terminator.Crash(
            "CreateProcess failed with error code: %lu.", GetLastError());
      } else {
        StoreIntToDescriptor(cmdstat, (uint32_t)GetLastError(), terminator);
        CheckAndCopyToDescriptor(cmdmsg, "CreateProcess failed.", 21);
      }
    }
    delete[] wcmd;
#else
    // terminated children do not become zombies
    signal(SIGCHLD, SIG_IGN);
    pid_t pid{fork()};
    if (pid < 0) {
      if (!cmdstat) {
        terminator.Crash("Fork failed with pid: %d.", pid);
      } else {
        StoreIntToDescriptor(cmdstat, FORK_ERR, terminator);
        CheckAndCopyToDescriptor(cmdmsg, "Fork failed", 11);
      }
    } else if (pid == 0) {
      int status{std::system(newCmd)};
      TerminationCheck(status, cmdstat, cmdmsg, terminator);
      exit(status);
    }
#endif
  }
}

} // namespace Fortran::runtime
