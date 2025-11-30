if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_101291890_101303925_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_101291890_101303925_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_101291890_101303925_EP_RR.cpp