# NANDFS

NANDFS is a file system for NAND flash memories. It mananges flash chips directly, and exposes a POSIX-like API to the operating system. NANDFS performs garbage collection, wear leveling and ECC ans is mostly intended for embedded systems with limited RAM, i.e. a few KBs up to a few MBs of RAM.

For more information on the design of NANDFS see the EMSOFT 2009 paper "NANDFS: A Flexible Flash File System for RAM-Constrained Systems". To download the paper check out <http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.187.5568>.

### What's included

The repository includes three directories:
* NANDFS - the core source code for running NANDFS
* SIM - used for running NANDFS in simulation mode over a large file simulating the actual chip
* ARM - a port of NANDFS to the LPC2119 micro-controller with a K9F5608U0D 32MB NAND flash chip

### Getting Started

* Set SIM_MODE NANDFS/tests.mk to either SIM or ARM to compile against the relevant code
* From the NANDFS directory run either `make` or `make rebuild`
* Run `testsAll`. This will run any unit tests set in the main() of NANDFS/testsAll.c

### Documentation

NANDFS is documented with doxygen-style comments. To use doxygen enter the NANDFS directory and run `doxygen nandfs.cnf`.

### Creators

**Aviad Zuck**

* <http://www.cs.technion.ac.il/~aviadzuc>

**Sivan Toledo**

* <http://www.cs.tau.ac.il/~stoledo>

### License

Code released under [LGPL license](http://www.gnu.org/licenses/lgpl-3.0.en.html)
