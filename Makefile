
CXXFLAGS += -g -std=c++11
#-DYAML_GENERATOR
LDFLAGS  += -lstdc++ -g

otx-yaml: otx-yaml.o yaml_parser.o yaml_node.o yaml_bits.o model.o modelslist.o

clean:
	rm -f *.o otx-yaml
