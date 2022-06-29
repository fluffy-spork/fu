#pragma once

// everything is stored as text from input and conversions can be done later as
// needed.

// sqlite strict types: integer, real, text, blob, any
#define FIELD_TYPE_TABLE(var, E) \
    E(unknown, "unknown", var) \
    E(text, "text", var) \
    E(integer, "integer", var) \
    E(timestamp, "timestamp", var) \

ENUM_BLOB(field_type, FIELD_TYPE_TABLE)

#define EXTRACT_AS_ENUM_FIELD(name, ...) name##_id_field,
#define EXTRACT_AS_STRUCT_FIELD(name, ...) field_t * name;

#define EXTRACT_AS_INIT_FIELD(name, label, type, max_size) \
    fields.name = field(name##_id_field, const_blob(#name), const_blob(label), type, max_size); \
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
    u16 max_size;
} field_t;

typedef struct {
    field_t * field;
    blob_t * value;
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
field(u64 id, blob_t * name, blob_t * label, field_type_t type, u16 req_size)
{
    field_t * field = malloc(sizeof(field_t));
    if (field) {
        u16 max_size = max_size_field_type(type, req_size);

        *field = (field_t){
            .id = id,
            .type = type,
            .name = name,
            .label = label,
            .max_size = max_size
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

