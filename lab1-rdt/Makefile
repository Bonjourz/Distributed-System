# NOTE: Feel free to change the makefile to suit your own need.

# compile and link flags
CCFLAGS = -Wall -g
LDFLAGS = -Wall -g

# make rules
TARGETS = rdt_sim 

all: $(TARGETS)

.cc.o:
	g++ $(CCFLAGS) -c -o $@ $<

rdt_check.o:	rdt_struct.h rdt_check.h

rdt_sender.o: 	rdt_struct.h rdt_sender.h

rdt_receiver.o:	rdt_struct.h rdt_receiver.h 

rdt_sim.o: 	rdt_struct.h

rdt_sim: rdt_sim.o rdt_sender.o rdt_receiver.o rdt_check.o
	g++ $(LDFLAGS) -o $@ $^

handin:
	tar -czf id.tar.gz Makefile README rdt_check.* rdt_receiver.cc rdt_sender.cc

clean:
	rm -f *~ *.o $(TARGETS)
