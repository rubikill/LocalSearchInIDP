# IDP solver with local search support
The repository consists of 4 folders. The folder _idpsourcecode_ includes the solver source code. 
The remaining three folders contain executable modelling files (.idp) and instances of three combinatorial optimization problems: 

- The Travelling Salesman Problem (TSP) consists of finding a shortest Hamiltonian cycle in a complete weighted graph.
- The assignment problem consists of finding a bijection between a set of agents and a set of tasks that minimizes the cost of assigning a certain task to a certain agent.
- The colouring violations problem consists of finding a graph colouring which minimizes the number of node-pairs that share an edge and are assigned the same colour.

Following are the instructions on how to build/install the software and run the example problems. 

## Building from source
Required software packages:
- C and C++ compiler, supporting most of the C++11 standard. Examples are GCC 4.4, Clang 3.1 and Visual Studio 11 or higher.
- Cmake build environment. 
- Bison and Flex parser generator software.
- Pdflatex for building the documentation.

Assume idp is unpacked in _idpdir_, you want to build in builddir (cannot be the same as _idpdir_) and install in _installdir_. Building and installing is then achieved by executing the following commands:

```
cd <builddir> cmake <idpdir> −DCMAKE_INSTALL_PREFIX=<installdir> −DCMAKE_BUILD_TYPE="Release"
make −j 4 
make check
make install
```

## Usage examples: 
For general usage, see [IDP manual](https://dtai.cs.kuleuven.be/krr/files/bib/manuals/idp3-manual.pdf). 

To execute the source files provided in this repository, the following command line can be used: 
```
Usage: 
	idp <source file> <instance file>

Examples: 
	idp TSP_localsearch.idp instances/br17.atsp.struc
```

