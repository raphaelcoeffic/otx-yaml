
CXXFLAGS += -g
LDFLAGS  += -lstdc++ -g

otx-yaml: otx-yaml.o yaml_parser.o yaml_node.o model.o
