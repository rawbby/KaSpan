#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef std::int64_t vertex_t;
typedef std::int64_t index_t;

inline off_t
fsize(const char* filename)
{
  struct stat st{};
  if (stat(filename, &st) == 0)
    return st.st_size;
  return -1;
}

int
main(int argc, char** argv)
{
  int fd;
  char* ss_head;
  char* ss;

  size_t file_size = fsize(argv[1]);

  fd = open(argv[1], O_CREAT | O_RDWR, 00666);

  ss_head = (char*)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  size_t head_offset = 0;
  while (ss_head[head_offset] == '%') {
    while (ss_head[head_offset] != '\n') {
      head_offset++;
    }
    head_offset++;
  }
  ss = &ss_head[head_offset];
  file_size -= head_offset;

  size_t curr = 0;
  size_t next = 0;

  size_t edge_count = 0;
  size_t vert_count;
  vertex_t v_max = 0;
  vertex_t v_min = 999999;
  vertex_t a;
  while (next < file_size) {
    char* sss = ss + curr;
    a = atoi(sss);

    if (v_max < a) {
      v_max = a;
    }
    if (v_min > a) {
      v_min = a;
    }

    while ((ss[next] != ' ') && (ss[next] != '\n') && (ss[next] != '\t')) {
      next++;
    }
    while ((ss[next] == ' ') || (ss[next] == '\n') || (ss[next] == '\t')) {
      next++;
    }
    curr = next;
    edge_count++;
  }
  edge_count /= 2;
  vert_count = v_max - v_min + 1;

  int fd4 = open("fw_adjacent.bin", O_CREAT | O_RDWR, 00666);
  ftruncate(fd4, edge_count * sizeof(vertex_t));
  auto* adj = (vertex_t*)mmap(NULL, edge_count * sizeof(vertex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd4, 0);

  int fd5 = open("fw_head.bin", O_CREAT | O_RDWR, 00666);
  ftruncate(fd5, edge_count * sizeof(vertex_t));
  auto* head = (vertex_t*)mmap(NULL, edge_count * sizeof(vertex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd5, 0);

  int fd2 = open("out_degree.bin", O_CREAT | O_RDWR, 00666);
  ftruncate(fd2, vert_count * sizeof(index_t));
  auto* degree = (index_t*)mmap(NULL, vert_count * sizeof(index_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);

  int fd3 = open("fw_begin.bin", O_CREAT | O_RDWR, 00666);
  ftruncate(fd3, (vert_count + 1) * sizeof(index_t));
  auto* begin = (index_t*)mmap(NULL, (vert_count + 1) * sizeof(index_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd3, 0);

  for (int i = 0; i < vert_count; i++) {
    degree[i] = 0;
  }
  vertex_t index;
  size_t offset = 0;
  curr = 0;
  next = 0;
  while (offset < edge_count) {
    char* sss = ss + curr;
    index = atoi(sss) - v_min;
    while ((ss[next] != ' ') && (ss[next] != '\n') && (ss[next] != '\t')) {
      next++;
    }
    while ((ss[next] == ' ') || (ss[next] == '\n') || (ss[next] == '\t')) {
      next++;
    }

    while ((ss[next] != ' ') && (ss[next] != '\n') && (ss[next] != '\t')) {
      next++;
    }
    while ((ss[next] == ' ') || (ss[next] == '\n') || (ss[next] == '\t')) {
      next++;
    }
    curr = next;
    degree[index]++;

    offset++;
  }

  begin[0] = 0;
  begin[vert_count] = edge_count;
  for (size_t i = 1; i < vert_count; i++) {
    begin[i] = begin[i - 1] + degree[i - 1];

    degree[i - 1] = 0;
  }
  degree[vert_count - 1] = 0;

  vertex_t v_id;
  offset = 0;
  next = 0;
  curr = 0;
  while (offset < edge_count) {
    char* sss = ss + curr;
    index = atoi(sss) - v_min;
    while ((ss[next] != ' ') && (ss[next] != '\n') && (ss[next] != '\t')) {
      next++;
    }
    while ((ss[next] == ' ') || (ss[next] == '\n') || (ss[next] == '\t')) {
      next++;
    }
    curr = next;

    char* sss1 = ss + curr;
    v_id = atoi(sss1) - v_min;
    adj[begin[index] + degree[index]] = v_id;
    head[begin[index] + degree[index]] = index;

    while ((ss[next] != ' ') && (ss[next] != '\n') && (ss[next] != '\t')) {
      next++;
    }
    while ((ss[next] == ' ') || (ss[next] == '\n') || (ss[next] == '\t')) {
      next++;
    }
    curr = next;
    degree[index]++;

    offset++;
  }

  for (size_t i = 0; i <= vert_count && i < 8; i++) {

    // std::cout << begin[i] << " " << degree[i] << " ";
    for (index_t j = 0; j < degree[i]; j++) {
      // std::cout << head[begin[i] + j] << "-" << adj[begin[i] + j] << " ";
    }

    // std::cout << std::endl;
  }
  // std::cout << begin[vert_count] << std::endl;

  for (int i = 0; i < edge_count && i < 64; i++) {
    // std::cout << head[i] << "\t" << adj[i] << std::endl;
  }

  munmap(ss, sizeof(char) * file_size);
  munmap(adj, sizeof(vertex_t) * edge_count);
  munmap(head, sizeof(vertex_t) * edge_count);
  munmap(begin, sizeof(index_t) * vert_count + 1);
  munmap(degree, sizeof(index_t) * vert_count);
  close(fd2);
  close(fd3);
  close(fd4);
  close(fd5);
  return 0;
}
