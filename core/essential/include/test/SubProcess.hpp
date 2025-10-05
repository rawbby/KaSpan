#pragma once

#include <cstdio>
#include <iosfwd>
#include <mpi.h>
#include <sstream>

inline auto
decode_system_status(int status) -> int
{
  char msg_buffer[512]{};

  if (status == -1) {
    std::fputs(msg_buffer, stderr);
    return 127;
  }

  if (WIFEXITED(status)) {
    auto const code = WEXITSTATUS(status);
    if (code != 0) {
      std::sprintf(msg_buffer, "[SUBPROCESS]\n  normal termination\n  status %d\n  code %d\n", status, code);
      std::fputs(msg_buffer, stderr);
    }
    return code;
  }

  if (WIFSIGNALED(status)) {
    auto const signal = WTERMSIG(status);
    auto const code   = 128 + signal;
#if defined(WCOREDUMP)
    std::sprintf(msg_buffer, "[SUBPROCESS]\n  terminated by signal\n  status %d\n  code %d\n  signal %d%s\n", status, code, signal, WCOREDUMP(status) ? " (core dumped)" : "");
#else
    std::sprintf(msg_buffer, "[SUBPROCESS]\n  terminated by signal\n  status %d\n  code %d\n  signal %d\n", status, code, signal);
#endif
    std::fputs(msg_buffer, stderr);
    return code;
  }

  if (WIFSTOPPED(status)) {
    auto const signal = WSTOPSIG(status);
    auto const code   = 128 + signal;
    std::sprintf(msg_buffer, "[SUBPROCESS]\n  stopped by signal\n  status %d\n  code %d\n  signal %d\n", status, code, signal);
    std::fputs(msg_buffer, stderr);
    return code;
  }

#ifdef WIFCONTINUED
  if (WIFCONTINUED(status)) {
    std::sprintf(msg_buffer, "[SUBPROCESS]\n  continued (unexpected)\n  status %d\n  code 1\n", status);
    std::fputs(msg_buffer, stderr);
    return 1;
  }
#endif

  std::sprintf(msg_buffer, "[SUBPROCESS]\n  unknown status state\n  status %d\n  code 1\n", status);
  std::fputs(msg_buffer, stderr);
  return 1;
}

inline void
mpi_sub_process(int argc, char** argv, int npc, int const* npv)
{
  constexpr std::string_view magic_flag = "--mpi-sub-process";

  for (int i = 1; i < argc; ++i)
    if (magic_flag == argv[i])
      return;

  auto const env_launcher = std::getenv("MPI_LAUNCHER");
  auto const launcher     = env_launcher ? env_launcher : "mpirun";

  for (int i = 0; i < npc; ++i) {

    std::ostringstream cmd_builder{};
    cmd_builder << launcher;
    cmd_builder << " -n " << npv[i];
    cmd_builder << " \"" << argv[0] << "\" ";
    cmd_builder << magic_flag;

    for (int j = 1; j < argc; ++j)
      cmd_builder << " \"" << argv[j] << '"';

    auto const cmd    = cmd_builder.str();
    auto const status = std::system(cmd.c_str());

    auto const exit_code = decode_system_status(status);
    if (exit_code != 0)
      std::exit(exit_code);
  }

  std::exit(0);
}

inline void
mpi_sub_process(int argc, char** argv)
{
  constexpr int npc      = 6;
  constexpr int npv[npc] = { 1, 2, 4, 8, 16, 32 };
  mpi_sub_process(argc, argv, npc, npv);
}
