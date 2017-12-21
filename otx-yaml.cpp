#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "yaml_parser.h"
#include "yaml_node.h"
#include "model.h"


Model model;

#define print_mixer_attr(attr)                  \
    printf("\t" #attr "\t=\t%i\n", model.mixData[i].attr)

void print_model()
{
    printf("%s\n",model.name);
    for(int i=0; i < MAX_MIXERS; i++) {
        printf(" mixer %i\n",i);
        print_mixer_attr(weight);
        print_mixer_attr(destCh);
        print_mixer_attr(carryTrim);
        print_mixer_attr(offset);
        print_mixer_attr(swtch);
        print_mixer_attr(flightModes);
        print_mixer_attr(srcRaw);
        printf("\t" "name" "\t=\t" "%s" "\n", model.mixData[i].name);
    }
}


int main()
{
    YamlParser yp(&modelNode);

    char   buffer[32];
    FILE * f = fopen("./test.yaml", "r");

    while(1) {

        size_t size = fread(buffer, 1, sizeof(buffer), f);
        if (size == 0) break;

        if (yp.parse(buffer,(unsigned int)size,
                     (attr_set_func)model_set_attr, &model) != YamlParser::CONTINUE_PARSING)
            break;
    } // until file consumed

    printf("RESULTAT\n");

    print_model();

    return 0;
}

