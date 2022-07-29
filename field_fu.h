#pragma once

// everything is stored as text from input and conversions can be done later as
// needed.

// sqlite strict types: integer, real, text, blob, any
#define FIELD_TYPE_TABLE(var, E) \
    E(unknown, "unknown", var) \
    E(text, "text", var) \
    E(integer, "integer", var) \
    E(timestamp, "timestamp", var) \
    E(password, "password", var) \
    E(hidden, "hidden", var) \

ENUM_BLOB(field_type, FIELD_TYPE_TABLE)

// NOTE(jason): still not sure if autocomplete should be part of the field.
// seems like it might be sitation/usage dependent
// is autocomplete desirable for stuff other than address and payment info?
#define AUTOCOMPLETE_TABLE(var, E) \
    E(off, "off", var) \
    E(on, "on", var) \
    E(email, "email", var) \
    E(name, "name", var) \

ENUM_BLOB(autocomplete, AUTOCOMPLETE_TABLE)

#define EXTRACT_AS_ENUM_FIELD(name, ...) name##_id_field,
#define EXTRACT_AS_STRUCT_FIELD(name, ...) field_t * name;

#define EXTRACT_AS_INIT_FIELD(name, label, type, min_size, max_size, autocomplete) \
    fields.name = field(name##_id_field, const_blob(#name), const_blob(label), type, min_size, max_size, autocomplete); \
    fields.list[name##_id_field] = fields.name; \

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
    s16 min_size;
    s16 max_size;
} field_t;

typedef struct {
    field_t * field;
    blob_t * value;
    blob_t * error;
} param_t;

u16
max_size_field_type(field_type_t type, u16 req_size)
{
    // verify req_size isn't above max size
    if (req_size) return req_size;

    switch (type) {
        case integer_field_type:
            return 19 + 1 + 6; // 1 for '-', 6 for separators
        default:
            return req_size;
    }
}

field_t *
field(u64 id, blob_t * name, blob_t * label, field_type_t type, s16 req_min_size, s16 req_max_size, autocomplete_t autocomplete)
{
    field_t * field = malloc(sizeof(field_t));
    if (field) {
        s16 max_size = max_size_field_type(type, req_max_size);
        s16 min_size = max_s16(req_min_size, 0);

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

int
sql_type_field(blob_t * sql, field_t * field)
{
    if (text_field_type == field->type) {
        add_blob(sql, B("text"));
    }
    else if (integer_field_type == field->type) {
        add_blob(sql, B("integer"));
    }
    else if (timestamp_field_type == field->type) {
        add_blob(sql, B("integer"));
    }
    else {
        assert(!"uknown field type");
        return -1;
    }

    return 0;
}

#define log_var_field(field) \
    log_var_u64(field->id); \
    log_var_blob(field->name); \
    log_var_blob(field->label); \
    log_var_u64(field->type); \
    log_var_u64(field->max_size); \

