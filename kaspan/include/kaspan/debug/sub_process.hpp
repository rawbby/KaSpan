#pragma once

#include <cstdio>
#include <format>
#include <memory>
#include <print>
#include <sstream>
#include <string_view>

namespace kaspan {

inline auto
decode_system_status(int status) -> int
{
  if (status == -1) {
    return 127;
  }

  if (WIFEXITED(status)) {
    auto const code = WEXITSTATUS(status);
    if (code != 0) {
      std::println(stderr, "[SUBPROCESS]\n  normal termination\n  status {}\n  code {}\n", status, code);
    }
    return code;
  }

  if (WIFSIGNALED(status)) {
    auto const signal = WTERMSIG(status);
    auto const code   = 128 + signal;
#if defined(WCOREDUMP)
    std::println(stderr, "[SUBPROCESS]\n  terminated by signal\n  status {}\n  code {}\n  signal {}{}\n", status, code, signal, WCOREDUMP(status) ? " (core dumped)" : "");
#else
    std::println(stderr, "[SUBPROCESS]\n  terminated by signal\n  status {}\n  code {}\n  signal {}\n", status, code, signal);
#endif
    return code;
  }

  if (WIFSTOPPED(status)) {
    auto const signal = WSTOPSIG(status);
    auto const code   = 128 + signal;
    std::println(stderr, "[SUBPROCESS]\n  stopped by signal\n  status {}\n  code {}\n  signal {}\n", status, code, signal);
    return code;
  }

#ifdef WIFCONTINUED
  if (WIFCONTINUED(status)) {
    std::println(stderr, "[SUBPROCESS]\n  continued (unexpected)\n  status {}\n  code 1\n", status);
    return 1;
  }
#endif

  std::println(stderr, "[SUBPROCESS]\n  unknown status state\n  status {}\n  code 1\n", status);
  return 1;
}

inline void
mpi_sub_process(int argc, char** argv, int npc, int const* npv)
{
  constexpr std::string_view magic_flag = "--mpi-sub-process";

  for (int i = 1; i < argc; ++i) {
    if (magic_flag == argv[i]) {
      return;
    }
  }

  auto* const       env_launcher = std::getenv("MPI_LAUNCHER");
  auto const* const launcher     = (env_launcher != nullptr) ? env_launcher : "mpirun";

  for (int i = 0; i < npc; ++i) {

    std::ostringstream cmd_builder{};
    cmd_builder << launcher;
    cmd_builder << " -n " << npv[i];
    cmd_builder << " \"" << argv[0] << "\" ";
    cmd_builder << magic_flag;

    for (int j = 1; j < argc; ++j) {
      cmd_builder << " \"" << argv[j] << '"';
    }

    auto const cmd    = cmd_builder.str();
    auto const status = std::system(cmd.c_str());

    auto const exit_code = decode_system_status(status);
    if (exit_code != 0) {
      std::exit(exit_code);
    }
  }

  std::exit(0);
}

inline void
mpi_sub_process(int argc, char** argv)
{
  constexpr int npc      = 3;
  constexpr int npv[npc] = { 3, 1, 7 };
  mpi_sub_process(argc, argv, npc, npv);
}

} // namespace kaspan
