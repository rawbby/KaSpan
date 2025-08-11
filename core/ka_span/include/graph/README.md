There are several strategies that can be used to load an distributed graph

1. One of the most straight forward ways is to use memory mapping.
   With memory mapping one virtually load the whole graph into the memory
   but in reality only what is accessed is loaded and cached.
   This should be zero overhead when the before the benchmark starts all local data was forced to be physically loaded.
   - note that in case of compressed input data one can not simply transform the representation
2. One can also explicitly read all the local data into a buffer. This is not only straight forward and simple but also allows ono the fly transformations to uncompress data.

What we test here is the performance of random access and iteration over:
- memory mapped compressed data
- memory mapped uncompressed data
- buffered compressed data
- buffered uncompressed data
