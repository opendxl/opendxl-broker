/*
Copyright (c) 2010-2014 Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <mosquitto_broker.h>
#include <memory_mosq.h>
#include <util_mosq.h>

// DXL
#include "search_optimization.h"
#include "dxl.h"
#include "DxlFlags.h"

struct _sub_token {
    struct _sub_token *next;
    char *topic;
};

// DXL Begin
static int _sub_add_full_topic(
    struct mosquitto_db *db, struct mosquitto *context, int qos,
    struct _mosquitto_subhier *subhier, struct _sub_token *tokens,
    char* fulltopic, unsigned int fulltopiclean, unsigned int depth);

static int _sub_remove_full_topic(
    struct mosquitto_db *db, struct mosquitto *context,
    struct _mosquitto_subhier *subhier, struct _sub_token *tokens,
    char* fulltopic, unsigned int fulltopiclean, unsigned int depth);

static int _subs_clean_session_full_topic(
    struct mosquitto_db *db, struct _mosquitto_subhier *root,
    char* fulltopic, unsigned int fulltopiclen, unsigned int depth);

// This is much less than the MQTT standard (64k).
// We can increase this if necessary, but there will be a performance impact.
#define MAX_TOPIC_LEN 1024
static char* _fulltopic = NULL;

int mqtt3_sub_init()
{
    _fulltopic = (char*)_mosquitto_malloc(MAX_TOPIC_LEN);
    if(_fulltopic){
        _fulltopic[0] = '\0';
        return 1;
    }
    return 0;
}

void mqtt3_sub_cleanup()
{
    if(_fulltopic){
        _mosquitto_free(_fulltopic);
    }
}

static void _add_topic_to_path(
    struct _mosquitto_subhier *subhier, char* fulltopic, unsigned int fulltopiclen, unsigned int depth)
{    
    if(subhier && subhier->topic && fulltopic && fulltopiclen > 0){
        unsigned int pos = (int)strlen(fulltopic);
        char* topic = subhier->topic;
        if(pos < fulltopiclen){
            if(depth > 2 || (depth == 2 && !strcmp("", topic) && pos > 0)){
                fulltopic[pos++] = '/';
            }

            unsigned int remaining = (fulltopiclen - pos);
            if(remaining > 0){
                strncpy((fulltopic+pos), topic, remaining);
            }                    
        }
        fulltopic[fulltopiclen-1] = '\0';
    }
}

static bool _is_topic_removed(struct mosquitto *context, struct _mosquitto_subhier *root)
{
    if(!root) return true;

    if(context && context->is_bridge){
        return false;
    }

    struct _mosquitto_subleaf *leaf = root->subs;
    while(leaf){
        if(!leaf->context->is_bridge){
            return false;
        }
        leaf = leaf->next;
    }
    return true;
}
// DXL End

static int _subs_process(
    struct mosquitto_db *db, struct _mosquitto_subhier *hier, const char *source_id,
    const char* /*topic*/, int qos, int retain, struct mosquitto_msg_store *stored,
    bool set_retain)
{
    int rc = 0;
    int client_qos, msg_qos;
    uint16_t mid;
    struct _mosquitto_subleaf *leaf;
    bool client_retain;

    // When this is null, it means no one has subscribed to the message
    leaf = hier->subs;
    if(retain && set_retain){
        if(hier->retained){
            hier->retained->ref_count--;
            /* FIXME - it would be nice to be able to remove the
               message from the store at this point if ref_count == 0 */
            db->retained_count--;
        }
        if(stored->msg.payloadlen){
            hier->retained = stored;
            hier->retained->ref_count++;
            db->retained_count++;
        }else{
            hier->retained = NULL;
        }
    }
    while(source_id && leaf){
        if(leaf->context->is_bridge && !strcmp(leaf->context->id, source_id)){
            leaf = leaf->next;
            continue;
        }
        client_qos = leaf->qos;

        if(db->config->upgrade_outgoing_qos){
            msg_qos = client_qos;
        }else{
            if(qos > client_qos){
                msg_qos = client_qos;
            }else{
                msg_qos = qos;
            }
        }
        if(msg_qos){
            mid = _mosquitto_mid_generate(leaf->context);
        }else{
            mid = 0;
        }
        if(leaf->context->is_bridge){
            /* If we know the client is a bridge then we should set retain
             * even if the message is fresh. If we don't do this, retained
             * messages won't be propagated. */
            client_retain = (retain != 0);
        }else{
            /* Client is not a bridge and this isn't a stale message so
             * retain should be false. */
            client_retain = false;
        }
        if(mqtt3_db_message_insert(db, leaf->context, mid, mosq_md_out, msg_qos, client_retain, stored) == 1) rc = 1;
        leaf = leaf->next;
    }

    return rc;
}

static int _sub_topic_tokenise(const char *subtopic, struct _sub_token **topics)
{
    struct _sub_token *new_topic, *tail = NULL;
    int len;
    int start, stop, tlen;
    int i;

    assert(subtopic);
    assert(topics);

    if(subtopic[0] != '$'){
        new_topic = (struct _sub_token *)_mosquitto_malloc(sizeof(struct _sub_token));
        if(!new_topic) goto cleanup;
        new_topic->next = NULL;
        new_topic->topic = _mosquitto_strdup("");
        if(!new_topic->topic) goto cleanup;

        if(tail){
            tail->next = new_topic;
            tail = tail->next;
        }else{
            *topics = new_topic;
            tail = new_topic;
        }
    }

    len = (int)strlen(subtopic);

    if(subtopic[0] == '/'){
        new_topic = (struct _sub_token *)_mosquitto_malloc(sizeof(struct _sub_token));
        if(!new_topic) goto cleanup;
        new_topic->next = NULL;
        new_topic->topic = _mosquitto_strdup("");
        if(!new_topic->topic) goto cleanup;

        if(tail){
            tail->next = new_topic;
            tail = tail->next;
        }else{
            *topics = new_topic;
            tail = new_topic;
        }

        start = 1;
    }else{
        start = 0;
    }

    stop = 0;
    for(i=start; i<len+1; i++){
        if(subtopic[i] == '/' || subtopic[i] == '\0'){
            stop = i;
            new_topic = (struct _sub_token *)_mosquitto_malloc(sizeof(struct _sub_token));
            if(!new_topic) goto cleanup;
            new_topic->next = NULL;

            if(start != stop){
                tlen = stop-start + 1;

                new_topic->topic = (char*)_mosquitto_calloc(tlen, sizeof(char));
                if(!new_topic->topic) goto cleanup;
                memcpy(new_topic->topic, &subtopic[start], tlen-1);
            }else{
                new_topic->topic = _mosquitto_strdup("");
                if(!new_topic->topic) goto cleanup;
            }
            if(tail){
                tail->next = new_topic;
                tail = tail->next;
            }else{
                tail = new_topic;
                *topics = tail;
            }
            start = i+1;
        }
    }

    return MOSQ_ERR_SUCCESS;

cleanup:
    tail = *topics;
    *topics = NULL;
    while(tail){
        if(tail->topic) _mosquitto_free(tail->topic);
        new_topic = tail->next;
        _mosquitto_free(tail);
        tail = new_topic;
    }
    return 1;
}

static int _sub_add(struct mosquitto_db *db, struct mosquitto *context, int qos,
    struct _mosquitto_subhier *subhier, struct _sub_token *tokens)
{
    _fulltopic[0] = '\0';
    return _sub_add_full_topic(db, context, qos, subhier, tokens, _fulltopic, MAX_TOPIC_LEN, 0);
}

static int _sub_add_full_topic(struct mosquitto_db *db, struct mosquitto *context,
    int qos, struct _mosquitto_subhier *subhier, struct _sub_token *tokens,
    char* fulltopic, unsigned int fulltopiclen, unsigned int depth)
{
    struct _mosquitto_subhier *tmp, *branch, *last = NULL;
    struct _mosquitto_subleaf *leaf, *last_leaf;    
    bool first_leaf = true;        

    // DXL Begin
    _add_topic_to_path(subhier, fulltopic, fulltopiclen, depth);

    if( dxl_is_multi_tenant_mode_enabled() &&
        !( context->dxl_flags & DXL_FLAG_OPS ) &&
        !dxl_is_tenant_subscription_allowed( context ) )
    {
        return 1;
    }
    // DXL End

    if(!tokens){
        if(context){
            leaf = subhier->subs;
            last_leaf = NULL;

// TODO (DXL): This needs to be optimized. Add contexts into a set, so we can
// do a quick check to see if they exist. Further, instead of walking to find last, 
// we should just track last (or add to first)
// Unsubscribe needs to be optimized as well.

            while(leaf){
                if(!strcmp(leaf->context->id, context->id)){
                    /* Client making a second subscription to same topic. Only
                     * need to update QoS. Return -1 to indicate this to the
                     * calling function. */
                    leaf->qos = qos;
                    return -1;
                }
                // DXL Begin
                if(!leaf->context->is_bridge){
                    first_leaf = false;        
                }
                // DXL End
                last_leaf = leaf;
                leaf = leaf->next;
            }
            leaf = (struct _mosquitto_subleaf *)_mosquitto_malloc(sizeof(struct _mosquitto_subleaf));
            if(!leaf) return MOSQ_ERR_NOMEM;
            leaf->next = NULL;
            leaf->context = context;
            leaf->qos = qos;
            if(last_leaf){
                last_leaf->next = leaf;
                leaf->prev = last_leaf;
            }else{
                subhier->subs = leaf;
                leaf->prev = NULL;
            }
            db->subscription_count++;

            // DXL Begin
            context->subscription_count++;

            if(first_leaf && !context->is_bridge){
                dxl_on_topic_added_to_broker(fulltopic);
            }
            // DXL End
        }
        return MOSQ_ERR_SUCCESS;
    }

    // DXL Begin
    struct subheir_hash *s = 0;
    HASH_FIND_STR(subhier->children_hash_table, tokens->topic, s);
    if(s){
        return _sub_add_full_topic(db, context, qos, s->subheir,
            tokens->next, fulltopic, fulltopiclen, depth + 1);
    }

    /* Not found */
    branch = (struct _mosquitto_subhier *)_mosquitto_calloc(1, sizeof(struct _mosquitto_subhier));
    if(!branch) return MOSQ_ERR_NOMEM;
    branch->topic = _mosquitto_strdup(tokens->topic);

    if(!branch->topic){
        _mosquitto_free(branch);
        return MOSQ_ERR_NOMEM;
    }

    branch->children_hash_table = NULL;
    branch->hash_table_has_pound_wild_card = 0;
    branch->hash_table_has_plus_wild_card = 0;

    // Add topic to the hash table
    add_heir_to_hash(&subhier->children_hash_table, branch->topic, branch);

    // Determine whether topic is a wild card and set it appropriately for searching
    if(topic_is_wild_card(branch->topic, "#")) subhier->hash_table_has_pound_wild_card++;
    if(topic_is_wild_card(branch->topic, "+")) subhier->hash_table_has_plus_wild_card++;

    if(!subhier->children){
        subhier->children = branch;
    }else{
        // These must be in order
        tmp = subhier->children;
        while(tmp){
            last = tmp;
            tmp = tmp->next;
        }
        last->next = branch;
    }
    // DXL End

    return _sub_add_full_topic(db, context, qos, branch, tokens->next, fulltopic, fulltopiclen, depth + 1);
}

static int _sub_remove(struct mosquitto_db *db, struct mosquitto *context,
    struct _mosquitto_subhier *subhier, struct _sub_token *tokens)
{
    _fulltopic[0] = '\0';
    return _sub_remove_full_topic(db, context, subhier, tokens, _fulltopic, MAX_TOPIC_LEN, 0);
}

static int _sub_remove_full_topic(struct mosquitto_db *db, struct mosquitto *context,
    struct _mosquitto_subhier *subhier, struct _sub_token *tokens, char* fulltopic,
    unsigned int fulltopiclen, unsigned int depth)
{
    struct _mosquitto_subhier *branch, *last = NULL;
    struct _mosquitto_subleaf *leaf;

    _add_topic_to_path(subhier, fulltopic, fulltopiclen, depth); // DXL

    if(!tokens){
        leaf = subhier->subs;
        while(leaf){
            if(leaf->context==context){
                db->subscription_count--;
                if(leaf->prev){
                    leaf->prev->next = leaf->next;
                }else{
                    subhier->subs = leaf->next;
                }
                if(leaf->next){
                    leaf->next->prev = leaf->prev;
                }
                // DXL Begin
                context->subscription_count--;

                if(_is_topic_removed(context, subhier)){
                    dxl_on_topic_removed_from_broker(fulltopic);
                }
                // DXL End
                _mosquitto_free(leaf);
                return MOSQ_ERR_SUCCESS;
            }
            leaf = leaf->next;
        }
        return MOSQ_ERR_SUCCESS;
    }

    // TODO: Use hash table to to search for existing items.
    branch = subhier->children;
    while(branch){
        if(!strcmp(branch->topic, tokens->topic)){
            _sub_remove_full_topic(db, context, branch, tokens->next, fulltopic, fulltopiclen, depth + 1);
            if(!branch->children && !branch->subs && !branch->retained){
                if(last){
                    last->next = branch->next;
                }else{
                    subhier->children = branch->next;
                }

                // DXL Begin
                remove_heir_from_hash(&subhier->children_hash_table, branch->topic);

                // Determine wether topic is a wild card and set it appropriately for searching
                if(topic_is_wild_card(branch->topic, "#") && (subhier->hash_table_has_pound_wild_card > 0))
                    subhier->hash_table_has_pound_wild_card--;
                if(topic_is_wild_card(branch->topic, "+") && (subhier->hash_table_has_plus_wild_card > 0))
                    subhier->hash_table_has_plus_wild_card--;
                // DXL End

                _mosquitto_free(branch->topic);
                _mosquitto_free(branch);
            }
            return MOSQ_ERR_SUCCESS;
        }
        last = branch;
        branch = branch->next;
    }
    return MOSQ_ERR_SUCCESS;
}

static void _sub_search(struct mosquitto_db *db, struct _mosquitto_subhier *subhier,
    struct _sub_token *tokens, const char *source_id,
    const char *topic, int qos, int retain, struct mosquitto_msg_store *stored, bool /*set_retain*/)
{
    /* FIXME - need to take into account source_id if the client is a bridge */
    struct _mosquitto_subhier *branch;
    /*bool sr;*/
    struct subheir_hash *topic_child = 0;

    // DXL Start
    if(subhier->hash_table_has_pound_wild_card){ // #
        HASH_FIND_STR(subhier->children_hash_table, "#", topic_child);
        if(topic_child){
            branch = topic_child->subheir;
            if(!branch->children){
                /* The topic matches due to a # wildcard - process the
                 * subscriptions but *don't* return. Although this branch has ended
                 * there may still be other subscriptions to deal with.
                 */
                _subs_process(db, branch, source_id, topic, qos, retain, stored, false);
            }
        }
    }

    if(tokens && tokens->topic){
        if(subhier->hash_table_has_plus_wild_card){ // +
            topic_child = 0;
            HASH_FIND_STR(subhier->children_hash_table, "+", topic_child);
            if(topic_child){
                branch = topic_child->subheir;
                _sub_search(db, branch, tokens->next, source_id, topic, qos, retain, stored, false);
                if(!tokens->next){
                    _subs_process(db, branch, source_id, topic, qos, retain, stored, false);
                }
            }
        }

        topic_child = 0;
        // Topic match
        HASH_FIND_STR(subhier->children_hash_table, tokens->topic, topic_child);
        if(topic_child){
            branch = topic_child->subheir;
            /* The topic matches this subscription.
                * Doesn't include # wildcards */
            _sub_search(db, branch, tokens->next, source_id, topic, qos, retain, stored, false);
            if(!tokens->next){
                _subs_process(db, branch, source_id, topic, qos, retain, stored, false);
            }
        }
    }
    // DXL End
}

int mqtt3_sub_add(struct mosquitto_db *db, struct mosquitto *context,
    const char *sub, int qos, struct _mosquitto_subhier *root)
{
    int rc = 0;
    struct _mosquitto_subhier /* *subhier,*/ *child;
    struct _sub_token *tokens = NULL, *tail;

    assert(root);
    assert(sub);

    if(_sub_topic_tokenise(sub, &tokens)) return 1;

    struct subheir_hash *s = 0;
    HASH_FIND_STR(root->children_hash_table, tokens->topic, s);
    if(s){
        rc = _sub_add(db, context, qos, s->subheir, tokens);
    }else{
        child = (_mosquitto_subhier*)_mosquitto_malloc(sizeof(struct _mosquitto_subhier));
        if(!child){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
            return MOSQ_ERR_NOMEM;
        }
        child->topic = _mosquitto_strdup(tokens->topic);
        if(!child->topic){
            _mosquitto_log_printf(NULL, MOSQ_LOG_ERR, "Error: Out of memory.");
            return MOSQ_ERR_NOMEM;
        }
        child->subs = NULL;
        child->children = NULL;
        child->retained = NULL;
        child->children_hash_table = NULL;
        child->hash_table_has_pound_wild_card = 0;
        child->hash_table_has_plus_wild_card = 0;
        add_heir_to_hash(&db->subs.children_hash_table, child->topic, child);

        if(db->subs.children){
            child->next = db->subs.children;
        }else{
            child->next = NULL;
        }
        db->subs.children = child;

        rc = _sub_add(db, context, qos, child, tokens);
    }

    while(tokens){
        tail = tokens->next;
        _mosquitto_free(tokens->topic);
        _mosquitto_free(tokens);
        tokens = tail;
    }
    /* We aren't worried about -1 (already subscribed) return codes. */
    if(rc == -1) rc = MOSQ_ERR_SUCCESS;
    return rc;
}

int mqtt3_sub_remove(struct mosquitto_db *db, struct mosquitto *context,
    const char *sub, struct _mosquitto_subhier *root)
{
    int rc = 0;
    /*struct _mosquitto_subhier *subhier;*/
    struct _sub_token *tokens = NULL, *tail;

    assert(root);
    assert(sub);

    if(_sub_topic_tokenise(sub, &tokens)) return 1;

    struct subheir_hash *s = 0;
    HASH_FIND_STR(root->children_hash_table, tokens->topic, s);
    if(s){
        rc = _sub_remove(db, context, s->subheir, tokens);
    }

    while(tokens){
        tail = tokens->next;
        _mosquitto_free(tokens->topic);
        _mosquitto_free(tokens);
        tokens = tail;
    }

    return rc;
}

int mqtt3_db_messages_queue(struct mosquitto_db *db, const char *source_id, const char *topic, int qos,
    int retain, struct mosquitto_msg_store *stored)
{
    int rc = 0;
    struct _sub_token *tokens = NULL, *tail;

    assert(db);
    assert(topic);

    if(_sub_topic_tokenise(topic, &tokens)) return 1;

    struct subheir_hash *s = 0;
    HASH_FIND_STR(db->subs.children_hash_table, tokens->topic, s);
    if(s){
        if(retain){
            /* We have a message that needs to be retained, so ensure that the subscription
                * tree for its topic exists.
                */
            _sub_add(db, NULL, 0, s->subheir, tokens);
        }
        _sub_search(db, s->subheir, tokens, source_id, topic, qos, retain, stored, true);
    }
    while(tokens){
        tail = tokens->next;
        _mosquitto_free(tokens->topic);
        _mosquitto_free(tokens);
        tokens = tail;
    }

    dxl_on_finalize_message(stored->db_id); // DXL

    return rc;
}

static int _subs_clean_session(struct mosquitto_db *db, struct _mosquitto_subhier *root)
{
    _fulltopic[0] = '\0';
    return _subs_clean_session_full_topic(db, root, _fulltopic, MAX_TOPIC_LEN, 0);
}

static int _subs_clean_session_full_topic(struct mosquitto_db *db,
    struct _mosquitto_subhier *root, char* fulltopic, unsigned int fulltopiclen, unsigned int depth)
{
    int rc = 0;
    struct _mosquitto_subhier *child, *last = NULL;
    struct _mosquitto_subleaf *leaf, *next;

    if(!root) return MOSQ_ERR_SUCCESS;

    _add_topic_to_path(root, fulltopic, fulltopiclen, depth); // DXL

    leaf = root->subs;
    while(leaf){
        if(leaf->context->clean_subs /* DXL */){
            db->subscription_count--;
            if(leaf->prev){
                leaf->prev->next = leaf->next;
            }else{
                root->subs = leaf->next;
            }
            if(leaf->next){
                leaf->next->prev = leaf->prev;
            }
            next = leaf->next;
            // DXL Begin
            if(_is_topic_removed(leaf->context, root)){
                dxl_on_topic_removed_from_broker(fulltopic);
            }
            // DXL End
            _mosquitto_free(leaf);
            leaf = next;
        }else{
            leaf = leaf->next;
        }
    }

    child = root->children;
    while(child){
        // DXL Begin
        char* old = strdup(fulltopic);
        _subs_clean_session_full_topic(db, child, fulltopic, fulltopiclen, depth + 1);
        if(old){
            strcpy(fulltopic, old);
            free(old);
        }
        // DXL End
        if(!child->children && !child->subs && !child->retained){
            if(last){
                last->next = child->next;
            }else{
                root->children = child->next;
            }

            remove_heir_from_hash(&root->children_hash_table, child->topic);

            // Determine wether topic is a wild card and set it appropriately for searching
            if(topic_is_wild_card(child->topic, "#") && (root->hash_table_has_pound_wild_card > 0))
                root->hash_table_has_pound_wild_card--;
            if(topic_is_wild_card(child->topic, "+") && (root->hash_table_has_plus_wild_card > 0))
                root->hash_table_has_plus_wild_card--;

            _mosquitto_free(child->topic);
            _mosquitto_free(child);
            if(last){
                child = last->next;
            }else{
                child = root->children;
            }
        }else{
            last = child;
            child = child->next;
        }
    }
    return rc;
}

/* Remove all subscriptions for a client.
 */
int mqtt3_subs_clean_session(struct mosquitto_db *db, struct _mosquitto_subhier *root)
{
    struct _mosquitto_subhier *child;
    child = root->children;
    while(child){
        _subs_clean_session(db, child);
        child = child->next;
    }

    return MOSQ_ERR_SUCCESS;
}

static int _retain_process(struct mosquitto_db *db, struct mosquitto_msg_store *retained,
    struct mosquitto *context, const char *UNUSED(sub), int sub_qos)
{
    int qos;
    uint16_t mid;

    qos = retained->msg.qos;

    if(qos > sub_qos) qos = sub_qos;
    if(qos > 0){
        mid = _mosquitto_mid_generate(context);
    }else{
        mid = 0;
    }
    return mqtt3_db_message_insert(db, context, mid, mosq_md_out, qos, true, retained);
}

static int _retain_search(struct mosquitto_db *db, struct _mosquitto_subhier *subhier,
    struct _sub_token *tokens, struct mosquitto *context, const char *sub, int sub_qos, int level)
{
    struct _mosquitto_subhier *branch;
    int flag = 0;

    branch = subhier->children;
    while(branch){
        /* Subscriptions with wildcards in aren't really valid topics to publish to
         * so they can't have retained messages.
         */
        if(!strcmp(tokens->topic, "#") && !tokens->next){
            /* Set flag to indicate that we should check for retained messages
             * on "foo" when we are subscribing to e.g. "foo/#" and then exit
             * this function and return to an earlier _retain_search().
             */
            flag = -1;
            if(branch->retained){
                _retain_process(db, branch->retained, context, sub, sub_qos);
            }
            if(branch->children){
                _retain_search(db, branch, tokens, context, sub, sub_qos, level+1);
            }
        }else if(strcmp(branch->topic, "+") && (!strcmp(branch->topic, tokens->topic) || !strcmp(tokens->topic, "+"))){
            if(tokens->next){
                if(_retain_search(db, branch, tokens->next, context, sub, sub_qos, level+1) == -1
                        || (!branch->next && tokens->next && !strcmp(tokens->next->topic, "#") && level>0)){

                    if(branch->retained){
                        _retain_process(db, branch->retained, context, sub, sub_qos);
                    }
                }
            }else{
                if(branch->retained){
                    _retain_process(db, branch->retained, context, sub, sub_qos);
                }
            }
        }

        branch = branch->next;
    }
    return flag;
}

int mqtt3_retain_queue(struct mosquitto_db *db, struct mosquitto *context, const char *sub, int sub_qos)
{
    struct _mosquitto_subhier *subhier;
    struct _sub_token *tokens = NULL, *tail;

    assert(db);
    assert(context);
    assert(sub);

    if(_sub_topic_tokenise(sub, &tokens)) return 1;

    subhier = db->subs.children;
    while(subhier){
        if(!strcmp(subhier->topic, tokens->topic)){
            _retain_search(db, subhier, tokens, context, sub, sub_qos, 0);
            break;
        }
        subhier = subhier->next;
    }
    while(tokens){
        tail = tokens->next;
        _mosquitto_free(tokens->topic);
        _mosquitto_free(tokens);
        tokens = tail;
    }

    return MOSQ_ERR_SUCCESS;
}

// DXL Begin
static int _sub_count_process(struct mosquitto_db * /*db*/, struct _mosquitto_subhier *hier,
    const char * /*topic*/, int *count, const char* tenant_guid)
{
    int rc = 0;
    struct _mosquitto_subleaf *leaf;

    // When this is null, it means no one has subscribed to the message
    leaf = hier->subs;
    while(leaf){
        if(leaf->context->is_bridge){
            leaf = leaf->next;
            continue;
        }
        if(!tenant_guid ||
           (leaf->context->dxl_tenant_guid && !strcmp(tenant_guid, leaf->context->dxl_tenant_guid))){
           (*count)++;
        }
        leaf = leaf->next;
    }

    return rc;
}

static int _sub_count_search(struct mosquitto_db *db, struct _mosquitto_subhier *subhier,
    struct _sub_token *tokens, const char *topic, int *count, const char* tenant_guid)
{
    struct subheir_hash *s = 0;
    int flag = 0;

    if(tokens && tokens->topic){
        HASH_FIND_STR(subhier->children_hash_table, "+", s);
        if(s){
            if(_sub_count_search(db, s->subheir, tokens->next, topic, count, tenant_guid) == -1){
                flag = -1;
            }
            if(!tokens->next){
                _sub_count_process(db, s->subheir, topic, count, tenant_guid);
            }
        }        
        HASH_FIND_STR(subhier->children_hash_table, tokens->topic, s);
        if(s){
            if(_sub_count_search(db, s->subheir, tokens->next, topic, count, tenant_guid) == -1){
                flag = -1;
            }
            if(!tokens->next){
                _sub_count_process(db, s->subheir, topic, count, tenant_guid);
            }
        }
    }

    HASH_FIND_STR(subhier->children_hash_table, "#", s);
    if(s){
        /* The topic matches due to a # wildcard - process the
         * subscriptions but *don't* return. Although this branch has ended
         * there may still be other subscriptions to deal with.
         */
        _sub_count_process(db, s->subheir, topic, count, tenant_guid);
        flag = -1;
    }

    return flag;
}

int mqtt3_sub_count(struct mosquitto_db *db, const char *topic, int *count, const char* tenant_guid)
{
    int rc = 0;
    struct _sub_token *tokens = NULL, *tail;
    struct subheir_hash *s = 0;

    assert(db);
    assert(topic);

    if(_sub_topic_tokenise(topic, &tokens)) return 1;

    HASH_FIND_STR(db->subs.children_hash_table, tokens->topic, s);
    if(s){
        rc = _sub_count_search(db, s->subheir, tokens, topic, count, tenant_guid);
        if(rc == -1){
            _sub_count_process(db, s->subheir, topic, count, tenant_guid);
            rc = 0;
        }
    }
    while(tokens){
        tail = tokens->next;
        _mosquitto_free(tokens->topic);
        _mosquitto_free(tokens);
        tokens = tail;
    }

    return rc;
}
// DXL End
