#include "moarvm.h"

/* This representation's function pointer table. */
static MVMREPROps *this_repr;

/* Creates a new type object of this representation, and associates it with
 * the given HOW. */
static MVMObject * type_object_for(MVMThreadContext *tc, MVMObject *HOW) {
    MVMSTable *st;
    MVMObject *obj;
    
    st = MVM_gc_allocate_stable(tc, this_repr, HOW);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&st);
    
    obj = MVM_gc_allocate_type_object(tc, st);
    st->WHAT = obj;
    st->size = sizeof(MVMString);
    
    MVM_gc_root_temp_pop(tc);
    
    return st->WHAT;
}

/* Creates a new instance based on the type object. */
static MVMObject * allocate(MVMThreadContext *tc, MVMSTable *st) {
    return MVM_gc_allocate_object(tc, st);
}

/* Initializes a new instance. */
static void initialize(MVMThreadContext *tc, MVMSTable *st, MVMObject *root, void *data) {
}

/* Copies the body of one object to another. */
static void copy_to(MVMThreadContext *tc, MVMSTable *st, void *src, MVMObject *dest_root, void *dest) {
    MVMStringBody *src_body  = (MVMStringBody *)src;
    MVMStringBody *dest_body = (MVMStringBody *)dest;
    dest_body->graphs = src_body->graphs;
    dest_body->codes  = src_body->codes;
    switch(src_body->codes & MVM_STRING_TYPE_MASK) {
        case MVM_STRING_TYPE_INT32:
            dest_body->data.int32s = malloc(sizeof(MVMint32) * dest_body->graphs);
            memcpy(dest_body->data.int32s, src_body->data.int32s, sizeof(MVMint32) * src_body->graphs);
            break;
        case MVM_STRING_TYPE_UINT8:
            dest_body->data.uint8s = malloc(sizeof(MVMuint8) * dest_body->graphs);
            memcpy(dest_body->data.uint8s, src_body->data.uint8s, sizeof(MVMuint8) * src_body->graphs);
            break;
        case MVM_STRING_TYPE_ROPE:
            dest_body->strands = malloc(sizeof(MVMStrand) * dest_body->graphs);
            memcpy(dest_body->strands, src_body->strands, sizeof(MVMStrand) * src_body->graphs);
    }
}

/* Adds held objects to the GC worklist. */
static void gc_mark(MVMThreadContext *tc, MVMSTable *st, void *data, MVMGCWorklist *worklist) {
    MVMStringBody *body  = (MVMStringBody *)data;
    if ((body->codes & MVM_STRING_TYPE_MASK) == MVM_STRING_TYPE_ROPE) {
        MVMStrand *strands = body->strands;
        MVMuint32 index = 0;
        MVMuint32 size = MVM_string_rope_strands_size(tc, body);
        while(index < body->graphs)
            MVM_gc_worklist_add(tc, worklist, &strands[index++].string);
    }
}

/* Called by the VM in order to free memory associated with this object. */
static void gc_free(MVMThreadContext *tc, MVMObject *obj) {
    MVMString *str = (MVMString *)obj;
    if (str->body.data.int32s)
        free(str->body.data.int32s);
    if (str->body.strands)
        free(str->body.strands);
    str->body.data.int32s = NULL;
    str->body.strands = NULL;
    str->body.graphs = str->body.codes = 0;
}

/* Gets the storage specification for this representation. */
static MVMStorageSpec get_storage_spec(MVMThreadContext *tc, MVMSTable *st) {
    MVMStorageSpec spec;
    spec.inlineable      = MVM_STORAGE_SPEC_REFERENCE;
    spec.boxed_primitive = MVM_STORAGE_SPEC_BP_NONE;
    spec.can_box         = 0;
    return spec;
}

/* Compose the representation. */
static void compose(MVMThreadContext *tc, MVMSTable *st, MVMObject *info) {
    /* Nothing to do for this REPR. */
}

/* Initializes the representation. */
MVMREPROps * MVMString_initialize(MVMThreadContext *tc) {
    /* Allocate and populate the representation function table. Note
     * that to support the bootstrap, this one REPR guards against a
     * duplicate initialization (which we actually will do). */
    if (!this_repr) {
        this_repr = malloc(sizeof(MVMREPROps));
        memset(this_repr, 0, sizeof(MVMREPROps));
        this_repr->type_object_for = type_object_for;
        this_repr->allocate = allocate;
        this_repr->initialize = initialize;
        this_repr->copy_to = copy_to;
        this_repr->gc_mark = gc_mark;
        this_repr->gc_free = gc_free;
        this_repr->get_storage_spec = get_storage_spec;
        this_repr->compose = compose;
    }
    return this_repr;
}
