#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "yaml_tree_walker.h"
#include "yaml_parser.h"
#include "yaml_node.h"

#include "model.h"
#include "modelslist.h"

Model model;

static int32_t to_signed(uint32_t i, uint8_t bits)
{
    if (i & (1 << (bits-1))) {
        i |= 0xFFFFFFFF << bits;
    }

    return i;
}

#define print_mixer_uattr(attr)                  \
    printf("\t" #attr "\t=\t%u\n", model.mixData[i].attr)

#define print_mixer_sattr(attr, bits)                                   \
    printf("\t" #attr "\t=\t%i\n", to_signed(model.mixData[i].attr, bits))

void print_model()
{
    printf("%s\n",model.header.name);
    for(int i=0; i < MAX_MIXERS; i++) {
        printf(" mixer %i\n",i);
        print_mixer_sattr(weight, 11);
        print_mixer_uattr(destCh);
        print_mixer_uattr(srcRaw);
        print_mixer_uattr(carryTrim);
        print_mixer_sattr(offset, 14);
        print_mixer_sattr(swtch, 9);
        print_mixer_uattr(flightModes);
        printf("\t" "name" "\t=\t" "%s" "\n", model.mixData[i].name);
    }
}

bool print_writer(void* opaque, const char* str, size_t len)
{
    printf("%.*s",(int)len,str);
    return true;
}

int main()
{
    YamlParser     yp;
#if 1
    YamlTreeWalker tree;

    tree.reset(&modelNode, (uint8_t*)&model);
    yp.init(YamlTreeWalker::get_parser_calls(), &tree);

    char   buffer[32];
    FILE * f = fopen("./test.yaml", "r");
    
    while(1) {

        size_t size = fread(buffer, 1, sizeof(buffer), f);
        if (size == 0) break;

        if (yp.parse(buffer,(unsigned int)size)
            != YamlParser::CONTINUE_PARSING)
            break;
    } // until file consumed

    printf("RESULTAT\n");

    print_model();

    printf("###############################\n");

    tree.reset(&modelNode, (uint8_t*)&model);
    tree.generate(print_writer, NULL);
#else
    yp.init(get_modelslist_parser_calls(), get_modelslist_iter());

    char   buffer[32];
    FILE * f = fopen("./modelslist.yml", "r");
    
    while(1) {

        size_t size = fread(buffer, 1, sizeof(buffer), f);
        if (size == 0) break;

        if (yp.parse(buffer,(unsigned int)size)
            != YamlParser::CONTINUE_PARSING)
            break;
    } // until file consumed

    modelslist.dump();
#endif

    return 0;
}

