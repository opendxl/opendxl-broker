#ifndef _SEARCH_OPTIMIZATION_H_
#define _SEARCH_OPTIMIZATION_H_

#include "mosquitto_broker.h"
#include "uthash.h"

struct subheir_hash {
    const char *topic;
    struct _mosquitto_subhier *subheir;
    UT_hash_handle hh;
};

void add_heir_to_hash(struct subheir_hash **subheir_hash, const char *topic_name, struct _mosquitto_subhier *subheir);
void remove_heir_from_hash(struct subheir_hash **subheir_hash, const char *topic_name);
void clear_hash_table(struct subheir_hash **subheir_hash);
bool topic_is_wild_card(const char *topic_name, const char *wild_card);

#endif
