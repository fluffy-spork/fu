#pragma once

// everything is stored as text from input and conversions can be done later as
// needed.

// sqlite strict types: integer, real, text, blob, any
#define FIELD_TYPE_TABLE(var, E) \
    E(unknown, "unknown", var) \
    E(text, "text", var) \
    E(integer, "integer", var) \
    E(hidden, "hidden", var) \
    E(file, "file", var) \
    E(timestamp, "timestamp", var) \
    E(password, "password", var) \
    E(select, "select", var) \

ENUM_BLOB(field_type, FIELD_TYPE_TABLE)

// NOTE(jason): still not sure if autocomplete should be part of the field.
// seems like it might be sitation/usage dependent
// is autocomplete desirable for stuff other than address and payment info?
#define AUTOCOMPLETE_ENUM(var, E) \
    E(off, "off", var) \
    E(on, "on", var) \
    E(email, "email", var) \
    E(name, "name", var) \

ENUM_BLOB(autocomplete, AUTOCOMPLETE_ENUM)

#define EXTRACT_AS_ENUM_FIELD(name, ...) name##_field,
#define EXTRACT_AS_STRUCT_FIELD(name, ...) field_t * name;

#define EXTRACT_AS_INIT_FIELD(name, label, type, min_size, max_size, autocomplete) \
    fields.name = field(name##_field, const_blob(#name), const_blob(label), type, min_size, max_size, autocomplete); \
    fields.list[name##_field] = fields.name; \

typedef enum {
    FIELD_TABLE(EXTRACT_AS_ENUM_FIELD)
    N_FIELDS
} field_id_t;

typedef struct {
    field_id_t id;
    field_type_t type;
    blob_t * name;
    blob_t * label;
    autocomplete_t  autocomplete;
    s32 min_size;
    s32 max_size;
} field_t;

s32
max_size_field_type(field_type_t type, s32 req_size)
{
    // verify req_size isn't above max size
    if (req_size) return req_size;

    switch (type) {
        // TODO(jason): reconsider changing select to id_select or something to
        // make it clear the value is an integer
        // these are 64 ids
        case select_field_type:
        case timestamp_field_type:
        case file_field_type:
            return 20;
        case integer_field_type:
            return 19 + 1 + 6; // 1 for '-', 6 for separators
        case text_field_type:
        case hidden_field_type:
        case password_field_type:
            return req_size;
        case unknown_field_type:
            return 0;
        default:
            debugf("missing max size for field_type: %d", type);
            assert(false);
    }
}

field_t *
field(u64 id, blob_t * name, blob_t * label, field_type_t type, s32 req_min_size, s32 req_max_size, autocomplete_t autocomplete)
{
    field_t * field = malloc(sizeof(field_t));
    if (field) {
        s32 max_size = max_size_field_type(type, req_max_size);
        s32 min_size = max_s32(req_min_size, 0);

        *field = (field_t){
            .id = id,
            .type = type,
            .name = name,
            .label = label,
            .min_size = min_size,
            .max_size = max_size,
            .autocomplete = autocomplete
        };
    }

    return field;
}

struct {
    FIELD_TABLE(EXTRACT_AS_STRUCT_FIELD)

    size_t n_list;
    field_t * list[N_FIELDS];
} fields;

void
init_fields()
{
    init_field_type();
    init_autocomplete();

    FIELD_TABLE(EXTRACT_AS_INIT_FIELD)

    fields.n_list = N_FIELDS;
}

field_t *
by_name_field(const blob_t * name)
{
    for (size_t i = 0; i < fields.n_list; i++) {
        if (equal_blob(fields.list[i]->name, name)) {
            return fields.list[i];
        }
    }

    return NULL;
}

#define log_var_field(field) \
    log_var_u64(field->id); \
    log_var_blob(field->name); \
    log_var_blob(field->label); \
    log_var_u64(field->type); \
    log_var_u64(field->max_size); \


typedef struct {
    field_t * field;
    blob_t * value;
    blob_t * error;
} param_t; // rename field_value_t?

// NOTE(jason): I think this usage of local_blob which uses alloca is ok.
// It shouldn't have issues with pointers in blocks since it isn't a pointer.
// we'll see.
#define local_param(f) (param_t){ .field = f, .value = local_blob(f->max_size), .error = local_blob(256) }

#define def_param(field) param_t field = local_param(fields.field);

#define debug_param(p) \
    debug_blob(p->field->name); \
    debug_s64(p->field->id); \
    debug_blob(p->value); \

void
debug_params(param_t * params, int n_params)
{
    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        debug_param(p);
    }
}

int
init_param(param_t * param, field_t * field)
{
    param->field = field;
    param->value = blob(field->max_size);
    param->error = blob(256);

    return 0;
}

int
init_by_name_param(param_t * param, const blob_t * field_name)
{
    field_t * field = by_name_field(field_name);
    dev_error(!field);

    return init_param(param, field);
}

param_t *
param(field_t * field)
{
    param_t * p = malloc(sizeof(*p));
    if (!p) return NULL;

    if (init_param(p, field)) {
        free(p);
        return NULL;
    }

    return p;
}

param_t *
by_id_param(param_t * params, int n_params, field_id_t field_id)
{
    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        if (p->field->id == field_id) {
            return p;
        }
    }

    return NULL;
}

blob_t *
value_by_id_param(param_t * params, int n_params, field_id_t field_id)
{
    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        if (p->field->id == field_id) {
            return p->value;
        }
    }

    return NULL;
}

