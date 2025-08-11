#include <distributed/Buffer.hpp>
#include <distributed/Partition.hpp>

#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <unistd.h>
#include <vector>

using namespace distributed;

void
fill_data(std::vector<std::byte>& data, std::size_t element_size, std::size_t count)
{
  data.resize(element_size * count);
  for (std::size_t i = 0; i < count; ++i) {
    for (std::size_t b = 0; b < element_size; ++b) {
      data[i * element_size + b] = static_cast<std::byte>((i + b) % 256);
    }
  }
}

template<PartitionConcept partition>
void
verify_buffer(const Buffer<partition>& buf, const std::vector<std::byte>& ref, std::size_t element_size)
{
  for (std::size_t global_idx = 0; global_idx < buf.partition.n; ++global_idx) {
    if (!buf.partition.contains(global_idx))
      continue;
    const std::byte* el     = buf[global_idx];
    const std::byte* ref_el = &ref[global_idx * element_size];
    assert(std::memcmp(el, ref_el, element_size) == 0);
  }
}

void
test_in_memory_all_partitions(std::size_t n, std::size_t element_size)
{
  std::vector<std::byte> ref_data;
  fill_data(ref_data, element_size, n);

  for (const std::size_t world_size : std::initializer_list<std::size_t>{ 1, 2, 3, n }) {
    for (std::size_t world_rank = 0; world_rank < world_size; ++world_rank) {
      CyclicPartition cpart{ n, world_rank, world_size };
      Buffer buf(ref_data.data(), element_size, cpart);
      verify_buffer(buf, ref_data, element_size);

      BlockCyclicPartition bcpart{ n, world_rank, world_size, 2 };
      Buffer       buf2(ref_data.data(), element_size, bcpart);
      verify_buffer(buf2, ref_data, element_size);

      TrivialSlicePartition tspart{ n, world_rank, world_size };
      Buffer        buf3(ref_data.data(), element_size, tspart);
      verify_buffer(buf3, ref_data, element_size);

      BalancedSlicePartition bspart{ n, world_rank, world_size };
      Buffer         buf4(ref_data.data(), element_size, bspart);
      verify_buffer(buf4, ref_data, element_size);
    }
  }
}

void
test_file_backed_all_partitions(std::size_t n, std::size_t element_size)
{
  std::vector<std::byte> ref_data;
  fill_data(ref_data, element_size, n);

  char      tmp_filename[] = "/tmp/partition_buffer_test_XXXXXX";
  const int fd             = mkstemp(tmp_filename);
  assert(fd >= 0);

  const uint8_t header = static_cast<uint8_t>(element_size);
  assert(write(fd, &header, 1) == 1);
  assert(write(fd, ref_data.data(), ref_data.size()) == static_cast<ssize_t>(ref_data.size()));

  for (const std::size_t world_size : std::initializer_list<std::size_t>{ 1, 2, 3, n }) {
    for (std::size_t world_rank = 0; world_rank < world_size; ++world_rank) {
      CyclicPartition cpart{ n, world_rank, world_size };
      Buffer buf(fd, cpart);
      verify_buffer(buf, ref_data, element_size);

      BlockCyclicPartition bcpart{ n, world_rank, world_size, 2 };
      Buffer       buf2(fd, bcpart);
      verify_buffer(buf2, ref_data, element_size);

      TrivialSlicePartition tspart{ n, world_rank, world_size };
      Buffer        buf3(fd, tspart);
      verify_buffer(buf3, ref_data, element_size);

      BalancedSlicePartition bspart{ n, world_rank, world_size };
      Buffer         buf4(fd, bspart);
      verify_buffer(buf4, ref_data, element_size);
    }
  }
  close(fd);
  std::filesystem::remove(tmp_filename);
}

void
test_copy_move()
{
  constexpr std::size_t  n = 5, elem_size = 4;
  std::vector<std::byte> ref;
  fill_data(ref, elem_size, n);

  constexpr CyclicPartition part{ n, 0, 1 };
  const Buffer     buf(ref.data(), elem_size, part);

  Buffer<CyclicPartition> buf2 = buf;
  verify_buffer(buf2, ref, elem_size);

  const Buffer<CyclicPartition> buf3 = std::move(buf2);
  verify_buffer(buf3, ref, elem_size);

  Buffer buf4(ref.data(), elem_size, part);
  buf4 = buf3;
  verify_buffer(buf4, ref, elem_size);

  Buffer buf5(ref.data(), elem_size, part);
  buf5 = std::move(buf4);
  verify_buffer(buf5, ref, elem_size);
}

void
test_operator_indexing()
{
  constexpr std::size_t  n = 12, elem_size = 1;
  std::vector<std::byte> ref;
  fill_data(ref, elem_size, n);
  constexpr BlockCyclicPartition part{ n, 1, 3, 2 };
  Buffer                 buf(ref.data(), elem_size, part);

  for (std::size_t i = 0; i < n; ++i) {
    if (!part.contains(i))
      continue;
    const std::byte* b = buf[i];
    assert(*b == ref[i]);
  }
}

void
test_empty_singleton()
{
  std::vector<std::byte> ref;
  fill_data(ref, 1, 1);
  constexpr CyclicPartition cpart2{ 1, 0, 1 };
  Buffer           buf2(ref.data(), 1, cpart2);
  assert(!buf2.empty());
  assert(*buf2[0] == ref[0]);
}

void
test_invalid_file_handling()
{
  char      tmp_filename[] = "/tmp/partition_buffer_test_invalid_XXXXXX";
  const int fd             = mkstemp(tmp_filename);
  assert(fd >= 0);
  constexpr CyclicPartition part{ 2, 0, 1 };
  try {
    Buffer buf(fd, part);
    assert(false && "Should have thrown for missing header");
  } catch (...) {
  }

  close(fd);
  std::filesystem::remove(tmp_filename);
}

int
main()
{
  test_in_memory_all_partitions(15, 4);
  test_in_memory_all_partitions(16, 1);
  test_in_memory_all_partitions(32, 3);
  test_file_backed_all_partitions(12, 2);
  test_file_backed_all_partitions(5, 4);
  test_copy_move();
  test_operator_indexing();
  test_empty_singleton();
  test_invalid_file_handling();

  std::cout << "All partition_buffer tests passed.\n";
}
