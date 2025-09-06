#pragma once

#include <buffer/Buffer.hpp>
#include <util/Arithmetic.hpp>

#include <set>

inline auto
process_wcc(
  i64              vert_beg,
  i64              vert_end,
  I64Buffer&       wcc_fq,
  I64Buffer const& color,
  i64&             wcc_fq_size) -> VoidResult
{
  std::set<i64> s_fq;
  for (i64 i = vert_beg; i < vert_end; ++i)
    if (not s_fq.contains(i))
      s_fq.insert(color[i]);

  wcc_fq_size = s_fq.size();

  i64 i = 0;
  for (auto it = s_fq.begin(); it != s_fq.end(); ++it, ++i)
    wcc_fq[i] = *it;

  return VoidResult::success();
}
