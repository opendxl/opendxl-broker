#include "search_optimization.h"

bool topic_is_wild_card(const char *topic_name, const char *wild_card)
{
    return !strcmp(topic_name, wild_card);
}

// DXL Start
void add_heir_to_hash(struct subheir_hash **subheir_hash, const char *topic_name, struct _mosquitto_subhier *subheir)
{
    struct subheir_hash *s = 0;
    HASH_FIND_STR(*subheir_hash, topic_name, s);
    if(!s){
        s = (struct subheir_hash *)malloc(sizeof(struct subheir_hash));
        s->topic = topic_name;
        s->subheir = subheir;
        HASH_ADD_KEYPTR(hh, *subheir_hash, s->topic, (unsigned int)strlen(s->topic), s);
    }
}

void remove_heir_from_hash(struct subheir_hash **subheir_hash, const char *topic_name)
{
    struct subheir_hash *s = 0;
    if(*subheir_hash && topic_name /*&& *topic_name != '\0'*/){
        HASH_FIND_STR(*subheir_hash, topic_name, s);
        if(s){
            HASH_DEL(*subheir_hash, s);
            free(s);
        }
    }
}

void clear_hash_table(struct subheir_hash **subheir_hash)
{
    struct subheir_hash *current, *tmp;
    HASH_ITER(hh, *subheir_hash, current, tmp){
        HASH_DEL(*subheir_hash, current);
        free(current);
    }
}
// DXL End