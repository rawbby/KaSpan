Portions of this project are derived from [iSpan](https://github.com/iHeartGraph/iSpan),
Copyright © 2018 Yuede Ji, Hang Liu, and H. Howie Huang, licensed under [GPL-3.0](LICENSE).

---

# KaSpan

## Dependencies

- MPI
- KaMPIng
- BriefKAsten

### Graph Generator

- KaGen

### Graph Converter

- stxxl

# TODO

- [ ] Try do the trim1 operation before tarjan
- [ ] Try redistributing the graph after ecl step (problem: map (fw_label, bw_label) => rank, redistribute small enough
  components, and map results back)
- [ ] Try asynchronous communication in ecl step (and reimplement async communication in forward backward search)
- [ ] (NOT PLANNED) Improve forward backward search by implementing top-down / bottom-up mechanism
- [ ] (NOT PLANNED) Implement openmp parallelism to exploit locality (hpc graph suggests benefits in certain cases)
