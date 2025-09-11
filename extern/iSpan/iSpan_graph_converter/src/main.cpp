#include <buffer/Buffer.hpp>
#include <graph/ConvertGraph.hpp>
#include <graph/Edge.hpp>

#include <stxxl.h>

auto
converter_main(int argc, char** argv) -> VoidResult
{
  using namespace convert_graph_internal;

  if (argc != 3 and argc != 4) {
    std::cout << "usage: ./iSan_graph_converter <edgelist.txt> <graph_name> [<memory_in_gb>]" << std::endl;
    std::exit(-1);
  }

  std::ifstream in{ argv[1] };
  RESULT_ASSERT(in, IO_ERROR);

  u64 max_node  = 0;
  u64 m         = 0;
  u64 n         = 0;
  u64 mem_bytes = (argc == 4 ? std::stoull(argv[3]) : 8) * 1000000000ULL;

  std::string const code         = argv[2];
  std::string const fw_head_path = code + ".fw.head.bin";
  std::string const fw_csr_path  = code + ".fw.csr.bin";
  std::string const bw_head_path = code + ".bw.head.bin";
  std::string const bw_csr_path  = code + ".bw.csr.bin";

  // first pass for forward
  {
    stxxl::sorter<Edge, EdgeLess> fw{ EdgeLess{}, mem_bytes };

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() or line[0] == '%')
        continue;

      RESULT_TRY(auto const uv, parse_uv(line));
      auto const [u, v] = uv;

      fw.push(Edge{ u, v });
      ++m;

      max_node = std::max(u, max_node);
      max_node = std::max(v, max_node);
    }

    n = m == 0 ? 0 : max_node + 1;

    RESULT_ASSERT(not in.bad(), IO_ERROR);

    fw.sort();

    RESULT_TRY(auto head_file, CU32Buffer<FileBuffer>::create_w(fw_head_path.c_str(), n + 1, 4, true, std::endian::little));
    RESULT_TRY(auto csr_file, CU32Buffer<FileBuffer>::create_w(fw_csr_path.c_str(), m, 4, true, std::endian::little));

    size_t i = 0;
    size_t j = 0;

    head_file.set(i++, 0);

    u64 current_node = 0;
    u64 current_off  = 0;

    for (; !fw.empty(); ++fw) {
      auto const [u, v] = *fw;

      while (current_node < u) {
        ++current_node;
        head_file.set(i++, current_off);
      }

      csr_file.set(j++, v);
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      head_file.set(i++, current_off);
    }
  }

  // second pass backward
  {
    in.clear();
    in.seekg(0);

    stxxl::sorter<Edge, EdgeLess> bw{ EdgeLess{}, mem_bytes };

    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() or line[0] == '%')
        continue;

      RESULT_TRY(auto const uv, parse_uv(line));
      auto const [u, v] = uv;

      bw.push(Edge{ v, u });
    }

    RESULT_ASSERT(not in.bad(), IO_ERROR);

    bw.sort();

    RESULT_TRY(auto head_file, CU32Buffer<FileBuffer>::create_w(bw_head_path.c_str(), n + 1, 4, true, std::endian::little));
    RESULT_TRY(auto csr_file, CU32Buffer<FileBuffer>::create_w(bw_csr_path.c_str(), m, 4, true, std::endian::little));

    size_t i = 0;
    size_t j = 0;

    head_file.set(i++, 0);

    u64 current_node = 0;
    u64 current_off  = 0;

    for (; !bw.empty(); ++bw) {
      auto const [u, v] = *bw;

      while (current_node < u) {
        ++current_node;
        head_file.set(i++, current_off);
      }

      csr_file.set(j++, v);
      ++current_off;
    }

    // close remaining head entries up to n
    while (current_node < n) {
      ++current_node;
      head_file.set(i++, current_off);
    }
  }

  return VoidResult::success();
}

int
main(int argc, char** argv)
{
  return static_cast<int>(converter_main(argc, argv).error_or_ok());
}
