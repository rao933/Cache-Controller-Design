# Cache-Controller-Design

The goal of this project is to determine the best architecture for a cache controller that interfaces to a 32-
bit microprocessor with a 32-bit data bus. While the microprocessor is general purpose in design and
performs a number of functions, it is desired to speed up certain signal processing functions, such as a
fast Fourier transform (FFT) routine. 

project should seek to allow 4GiB of SDRAM to be interfaced using a 32-bit data bus (arranged as
30 x 32-bits) and the cache should be limited to 256 KiB in size. The size of the FFT will be 32768 points
(512KiB) in normal operation.

By adding extra code to the FFT function as shown in class, it is possible to determine exactly how
memory is accessed.

The metric being used to evaluate the best performance is the total access time required to run the
Radix2FFT(), including the time required to flush the cache after the FFT is computed. You may assume
that the cache is empty before the FFT function is called.

Please use the assumption that the miss penalty is 60 ns (for first memory access) and 17ns for
subsequent accesses in the same SDRAM word line and the cache hit time is 1 ns.
Please describe your choice of the set size (N-way), number of cache lines (L), block size (B), write
strategy (write-back allocate, write-through allocate, and write-through non-allocate), and cache line block
replacement strategy. (round robin or LRU) You do not need to build the controller, just determine the
best architecture. Determine which 3 configurations result in the best performance. At a minimum, n-way
associativity of 1, 2, 4, 8, and 16 and burst lengths of 1, 2, 4, and 8 should be evaluated for all 3 write
strategies (60 permutations).
