#pragma once

// everything is stored as text from input and conversions can be done later as
// needed.

// sqlite strict types: integer, real, text, blob, any
#define TYPE_FIELD_TABLE(var, E) \
    E(unknown, "unknown", var) \
    E(text, "text", var) \
    E(integer, "integer", var) \
    E(hidden, "hidden", var) \
    E(file, "file", var) \
    E(timestamp, "timestamp", var) \
    E(password, "password", var) \
    E(select_integer, "select_integer", var) \
    E(file_multiple, "file_multiple", var) \

ENUM_BLOB(type_field, TYPE_FIELD_TABLE)

#define EXTRACT_AS_ENUM_FIELD(name, ...) name##_field,
#define EXTRACT_AS_STRUCT_FIELD(name, ...) field_t * name;

// TODO(jason): make the field name and the name used for a parameter separate
// can have to parameters that are conceptually different that need to use the
// same name for html forms, etc
#define EXTRACT_AS_INIT_FIELD(name, label, type, min_size, max_size) \
    fields.name = field(name##_field, const_blob(#name), const_blob(label), type, min_size, max_size); \
    fields.list[name##_field] = fields.name; \

typedef enum {
    FIELD_TABLE(EXTRACT_AS_ENUM_FIELD)
    N_FIELDS
} field_id_t;

typedef struct {
    field_id_t id;
    type_field_t type;
    blob_t * name;
    blob_t * label;
    s32 min_size;
    s32 max_size;
} field_t;

s32
max_size_type_field(type_field_t type, s32 req_size)
{
    // verify req_size isn't above max size
    if (req_size) return req_size;

    switch (type) {
        case select_integer_type_field:
            return 20;
        case timestamp_type_field:
            return 20;
        case integer_type_field:
            return 19 + 1 + 6; // 1 for '-', 6 for separators
        case file_type_field:
        case file_multiple_type_field:
            return max_s32(req_size, 255);
        case text_type_field:
        case hidden_type_field:
        case password_type_field:
            return req_size;
        case unknown_type_field:
            return 0;
        default:
            debugf("missing max size for type_field: %d", type);
            assert(false);
    }
}

field_t *
field(field_id_t id, blob_t * name, blob_t * label, type_field_t type, s32 req_min_size, s32 req_max_size)
{
    field_t * field = malloc(sizeof(field_t));
    if (field) {
        s32 max_size = max_size_type_field(type, req_max_size);
        s32 min_size = max_s32(req_min_size, 0);

        *field = (field_t){
            .id = id,
            .type = type,
            .name = name,
            .label = label,
            .min_size = min_size,
            .max_size = max_size,
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
init_fields(void)
{
    init_type_field();

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

field_t *
by_id_field(field_id_t id)
{
    dev_error(id < 0);
    dev_error(id >= fields.n_list);

    return fields.list[id];
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

// NOTE(jason): I think this usage of stk_blob which uses alloca is ok.
// It shouldn't have issues with pointers in blocks since it isn't a pointer.
// we'll see.
#define local_param(f) (param_t){ .field = f, .value = stk_blob(f->max_size), .error = stk_blob(256) }
// prefer stk_param and remove local_param
#define stk_param(f) (param_t){ .field = f, .value = stk_blob(f->max_size), .error = stk_blob(256) }

#define def_param(field) param_t field = stk_param(fields.field);

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
    dev_error(field == NULL);

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

// TODO(jason): untested/unused
int
copy_params(param_t * dest, int n_dest, param_t * src, int n_src)
{
    for (int i = 0; i < n_src; i++) {
        param_t * s = &src[i];

        param_t * d = by_id_param(dest, n_dest, s->field->id);
        if (d) {
            set_blob(s->value, d->value);
        }
    }

    return 0;
}

