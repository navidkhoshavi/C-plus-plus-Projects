I used Code::Blocks 13.12 tool to debug and compile my program. In order to enable OpenMP option in Code::Blocks, please follow below steps:
1)	Enable C++ISO C++ language standard [std=c++11]
	a.	Go to Toolbar -> Settings -> Compiler
	b.	In the "Selected compiler" drop-down menu, make sure "GNU GCC Compiler" is selected
	c.	Below that, select the "compiler settings" tab and then the "compiler flags" tab underneath
	d.	In the list below, make sure the box for "Have g++ follow the C++11 ISO C++ language standard 		[-std=c++11]" is checked
	e.	Click OK to save
2)	add "-fopenmp" to "Settings -> Compiler ... -> Compiler setting tab -> Other options"
3)	add “gomp” to "Settings -> Compiler  … -> Linker setting 
