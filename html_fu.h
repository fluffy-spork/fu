#pragma once

#include <math.h>

// NOTE(jason): not doing single character method names so there's "para()"
// instead of "p()" for making a paragraph.  Using "link" instead of "anchor"
// since it's more meaningful

// NOTE(jason): checks for a variable named "html" in the current context
#define assert_html() assert(html != NULL);

// TODO(jason): could these be defined as a macro at the definition?
// html_gen(img) that defines the macro then the function?  I would prefer to
// just be able to access the macro name within the macro, but doesn't seem
// possible
#define page_begin(...) page_begin_html(html, __VA_ARGS__);
#define page_end() page_end_html(html);
#define h1(...) h1_html(html, __VA_ARGS__);
#define img(...) img_html(html, __VA_ARGS__);
#define start_post_form(...) start_post_form_html(html, __VA_ARGS__);
#define start_get_form(...) start_get_form_html(html, __VA_ARGS__);
#define end_form() end_form_html(html);
#define param_input(...) param_input_html(html, __VA_ARGS__);
#define text_input(...) input_html(html, res_html.text_type, __VA_ARGS__);
#define hidden_input(...) input_html(html, res_html.hidden_type, __VA_ARGS__);
#define numeric_input(...) numeric_input_html(html, __VA_ARGS__);
#define submit_input(...) input_html(html, res_html.submit_type, __VA_ARGS__);
#define label(...) label_html(html, __VA_ARGS__);
#define para(...) para_html(html, __VA_ARGS__);
#define start_para() start_para_html(html);
#define end_para() end_para_html(html);
#define div_id(...) div_id_html(html, __VA_ARGS__);
#define div_class(...) div_class_html(html, __VA_ARGS__)
#define div_id_class(...) div_id_class_html(html, __VA_ARGS__)
#define start_div_id(...) start_div_id_html(html, __VA_ARGS__);
#define start_div_id_class(...) start_div_id_class_html(html, __VA_ARGS__);
#define end_div() end_div_html(html);
#define start_div_class(...) start_div_class_html(html, __VA_ARGS__)
#define link(...) link_html(html, __VA_ARGS__);
#define table_start() table_start_html(html);
#define table_end() table_end_html(html);
#define tr_start() tr_start_html(html);
#define tr_end() tr_end_html(html);
#define td(content) td_html(html, content);
#define td_start() td_start_html(html);
#define td_end() td_end_html(html);
#define th(content) th_html(html, content);
#define th_start() th_start_html(html);
#define th_end() th_end_html(html);
#define start_list() start_list_html(html);
#define end_list() end_list_html(html);
#define list_item(...) list_item_html(html, __VA_ARGS__);

#define RES_HTML(var, E) \
    E(space, " ", var) \
    E(double_quote_ref, "&quot;", var) \
    E(less_than_ref, "&lt;", var) \
    E(error, "error", var) \
    E(errors, "errors", var) \
    E(doctype, "<!doctype html><meta charset=\"utf-8\" <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">", var) \
    E(begin_head, "<html><head><meta name=\"theme-color\" content=\"#000\">", var) \
    E(end_head, "</head>", var) \
    E(res_path, "/res/", var) \
    E(begin_title, "<title>", var) \
    E(end_title, "</title>", var) \
    E(begin_page, "<body>", var) \
    E(end_page, "</body></html>", var) \
    E(header, "header", var) \
    E(main, "main", var) \
    E(user, "user", var) \
    E(id, "id", var) \
    E(clazz, "class", var) \
    E(start_tag, "<", var) \
    E(end_tag, "</", var) \
    E(close_tag, ">", var) \
    E(open_attr_value, "=\"", var) \
    E(close_attr_value, "\"", var) \
    E(h1, "h1", var) \
    E(div, "div", var) \
    E(img, "img", var) \
    E(src, "src", var) \
    E(alt, "alt", var) \
    E(anchor, "a", var) \
    E(href, "href", var) \
    E(para, "p", var) \
    E(form, "form", var) \
    E(action, "action", var) \
    E(method, "method", var) \
    E(get_method, "get", var) \
    E(post_method, "post", var) \
    E(input, "input", var) \
    E(name, "name", var) \
    E(value, "value", var) \
    E(type, "type", var) \
    E(text_type, "text", var) \
    E(submit_type, "submit", var) \
    E(hidden_type, "hidden", var) \
    E(minlength, "minlength", var) \
    E(maxlength, "maxlength", var) \
    E(cols, "cols", var) \
    E(rows, "rows", var) \
    E(autocomplete, "autocomplete", var) \
    E(autofocus, "autofocus", var) \
    E(placeholder, "placeholder", var) \
    E(action_bar, "action-bar", var) \
    E(textarea, "textarea", var) \
    E(label, "label", var) \
    E(table, "table", var) \
    E(tr, "tr", var) \
    E(th, "th", var) \
    E(td, "td", var) \
    E(ul, "ul", var) \
    E(li, "li", var) \

ENUM_BLOB(res_html, RES_HTML)

blob_t * _res_suffix_html;
blob_t * _page_res_html;

blob_t *
res_url(blob_t * b, blob_t * name)
{
    vadd_blob(b, res_html.res_path, name, _res_suffix_html);
    return b;
}

// /res/main.css?v=23098abc089ef09

void
init_html()
{
    init_res_html();

    blob_t * b = blob(1024);
    add_blob(b, B("?v="));
    add_random_blob(b, 8);
    _res_suffix_html = b;
    log_var_blob(_res_suffix_html);

    b = blob(1024);
    add_blob(b, B("<link rel=\"stylesheet\" href=\""));
    res_url(b, B("main.css"));
    add_blob(b, B("\">"));
    _page_res_html = b;

    // TODO(jason): will probably have to add javascript
}

// NOTE(jason): this is just a wrapper so it's easy to identify when it's being
// used compared to add_blob.  maybe should be a macro?
ssize_t
raw_html(blob_t * html, const blob_t * content)
{
    return add_blob(html, content);
}

ssize_t
escape_html(blob_t * html, const blob_t * content)
{
    return escape_blob(html, content, '<', res_html.less_than_ref);
}

ssize_t
double_quote_escape_html(blob_t * html, const blob_t * content)
{
    return escape_blob(html, content, '\"', res_html.double_quote_ref);
}

void
head_html(blob_t * html, const blob_t * title)
{
    add_blob(html, res_html.begin_head);
    add_blob(html, res_html.begin_title);
    add_blob(html, title);
    add_blob(html, res_html.end_title);
    add_blob(html, _page_res_html);
    add_blob(html, res_html.end_head);
}

void
page_begin_html(blob_t * html, const blob_t * title)
{
    assert_html();

    add_blob(html, res_html.doctype);

    head_html(html, title);

    add_blob(html, res_html.begin_page);
}

void
page_end_html(blob_t * html)
{
    assert_html();

    add_blob(html, res_html.end_page);
}

// XXX: should be a way to validate an attribute name is valid, but doesn't
// need to happen all the time.  maybe just add helper functions like src_attr()
void
attr_html(blob_t * html, const blob_t * name, const blob_t * value)
{
    if (!value) return;

    add_blob(html, res_html.space);
    add_blob(html, name);
    add_blob(html, res_html.open_attr_value);

    //&quot;
    double_quote_escape_html(html, value);

    add_blob(html, res_html.close_attr_value);
}

void
start_element_html(blob_t * html, const blob_t * name)
{
    assert_html();

    add_blob(html, res_html.start_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

void
start_element_id_class_html(blob_t * html, const blob_t * name, const blob_t * id, const blob_t * clazz)
{
    add_blob(html, res_html.start_tag);
    add_blob(html, name);
    if (id) attr_html(html, res_html.id, id);
    if (clazz) attr_html(html, res_html.clazz, clazz);
    add_blob(html, res_html.close_tag);
}

void
end_element_html(blob_t * html, const blob_t * name)
{
    add_blob(html, res_html.end_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

void
element_html(blob_t * html, const blob_t * name, const blob_t * content)
{
    start_element_html(html, name);
    escape_html(html, content);
    end_element_html(html, name);
}

void
element_id_class_html(blob_t * html, const blob_t * name, const blob_t * id, const blob_t * clazz, const blob_t * content)
{
    start_element_id_class_html(html, name, id, clazz);
    escape_html(html, content);
    end_element_html(html, name);
}

void
open_tag_html(blob_t * html, const blob_t *name)
{
    add_blob(html, res_html.start_tag);
    add_blob(html, name);
}

void
close_tag_html(blob_t * html)
{
    add_blob(html, res_html.close_tag);
}

void
end_tag_html(blob_t * html, const blob_t *name)
{
    add_blob(html, res_html.end_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

void
s64_attr_html(blob_t * html, blob_t * name, s64 value)
{
    add_blob(html, res_html.space);
    add_blob(html, name);

    add_blob(html, res_html.open_attr_value);
    add_s64_blob(html, value);
    add_blob(html, res_html.close_attr_value);
}

void
empty_attr_html(blob_t * html, const blob_t * name)
{
    add_blob(html, res_html.space);
    add_blob(html, name);
}

void
src_attr_html(blob_t * html, const blob_t * src)
{
    assert_html();

    attr_html(html, res_html.src, src);
}

void
h1_html(blob_t * html, const blob_t * content)
{
    assert_html();

    element_html(html, res_html.h1, content);
}

void
img_html(blob_t * html, const blob_t * src, const blob_t * alt)
{
    assert_html();
    assert(src != NULL);

    open_tag_html(html, res_html.img);
    attr_html(html, res_html.src, src);
    if (alt) attr_html(html, res_html.alt, alt);
    close_tag_html(html);
}

void
link_html(blob_t * html, const blob_t * url, const blob_t * content)
{
    assert_html();
    assert(url != NULL);

    open_tag_html(html, res_html.anchor);
    attr_html(html, res_html.href, url);
    close_tag_html(html);

    if (content != NULL) {
        escape_html(html, content);
    }

    end_tag_html(html, res_html.anchor);
}

void
div_html(blob_t * html, const blob_t * content)
{
    element_html(html, res_html.div, content);
}

void
start_div_id_html(blob_t * html, const blob_t * id)
{
    start_element_id_class_html(html, res_html.div, id, NULL);
}

void
start_div_id_class_html(blob_t * html, const blob_t * id, const blob_t * clazz)
{
    start_element_id_class_html(html, res_html.div, id, clazz);
}

void
start_div_class_html(blob_t * html, const blob_t * clazz)
{
    start_element_id_class_html(html, res_html.div, NULL, clazz);
}

void
div_id_class_html(blob_t * html, const blob_t * id, const blob_t * clazz, const blob_t * content)
{
    element_id_class_html(html, res_html.div, id, clazz, content);
}

void
div_id_html(blob_t * html, const blob_t * id, const blob_t * content)
{
    element_id_class_html(html, res_html.div, id, NULL, content);
}

void
div_class_html(blob_t * html, const blob_t * clazz, const blob_t * content)
{
    element_id_class_html(html, res_html.div, NULL, clazz, content);
}

void
end_div_html(blob_t * html)
{
    end_element_html(html, res_html.div);
}

// include a raw, preformatted bit of html
void
include_div_html(blob_t * html, blob_t * id, blob_t * content)
{
    start_div_id_html(html, id);
    raw_html(html, content);
    end_div_html(html);
}

void
para_html(blob_t * html, const blob_t * content)
{
    element_html(html, res_html.para, content);
}

void
start_para_html(blob_t * html)
{
    start_element_html(html, res_html.para);
}

void
end_para_html(blob_t * html)
{
    end_element_html(html, res_html.para);
}

// NOTE(jason): autocomplete really only makes sense for off/on
void
_start_form_html(blob_t * html, const blob_t * action, const blob_t * method, autocomplete_t autocomplete)
{
    assert_html();
    assert(action != NULL);

    open_tag_html(html, res_html.form);
    attr_html(html, res_html.method, method);
    attr_html(html, res_html.action, action);
    attr_html(html, res_html.autocomplete, blob_autocomplete(autocomplete));

    close_tag_html(html);
}

void
start_post_form_html(blob_t * html, const blob_t * action, autocomplete_t autocomplete)
{
    _start_form_html(html, action, res_html.post_method, autocomplete);
}

void
start_get_form_html(blob_t * html, const blob_t * action, autocomplete_t autocomplete)
{
    _start_form_html(html, action, res_html.get_method, autocomplete);
}

void
end_form_html(blob_t * html)
{
    end_element_html(html, res_html.form);
}

void
label_html(blob_t * html, blob_t * content)
{
    element_html(html, res_html.label, content);
}

void
input_html(blob_t * html, const blob_t * type, const blob_t * name, const blob_t * value)
{
    open_tag_html(html, res_html.input);
    attr_html(html, res_html.type, type);
    attr_html(html, res_html.name, name);
    //attr_html(html, res_html.autocomplete, res_html.off);
    if (valid_blob(value)) attr_html(html, res_html.value, value);
    close_tag_html(html);
}

void
single_input_post_form_html(blob_t * html, blob_t * action, field_t * field, autocomplete_t autocomplete)
{
    start_post_form(action, autocomplete);

    open_tag_html(html, res_html.input);
    attr_html(html, res_html.type, res_html.text_type);
    attr_html(html, res_html.name, field->name);
    attr_html(html, res_html.placeholder, field->label);
    close_tag_html(html);

    end_form();
}

void
hidden_submit_post_form_html(blob_t * html, blob_t * action, field_t * field, blob_t * value, blob_t * label)
{
    start_post_form(action, off_autocomplete);
    hidden_input(field->name, value);
    submit_input(B("action"), label);
    end_form();
}

void
textarea_html(blob_t * html, param_t * param, bool autofocus)
{
    field_t * f = param->field;
    blob_t * v = param->value;

    open_tag_html(html, res_html.textarea);
    attr_html(html, res_html.name, f->name);
    attr_html(html, res_html.autocomplete, blob_autocomplete(f->autocomplete));
    if (autofocus) empty_attr_html(html, res_html.autofocus);
    int cols = 62;
    s64_attr_html(html, res_html.cols, cols);
    s64_attr_html(html, res_html.rows, ceil((double)f->max_size/(cols - 1)));
    if (f->min_size) s64_attr_html(html, res_html.minlength, f->min_size);
    if (f->max_size) s64_attr_html(html, res_html.maxlength, f->max_size);
    close_tag_html(html);

    if (valid_blob(v)) escape_html(html, v);

    end_tag_html(html, res_html.textarea);
}

// NOTE(jason): could be improved.  maybe other input types?
void
param_input_html(blob_t * html, param_t * param, bool autofocus)
{
    blob_t * value = param->value;
    field_t * field = param->field;

#define TEXT_AREA_MIN_SIZE 64

    if (field->max_size > TEXT_AREA_MIN_SIZE) {
        textarea_html(html, param, autofocus);
    }
    else {
        open_tag_html(html, res_html.input);
        attr_html(html, res_html.type, res_html.text_type);
        attr_html(html, res_html.name, field->name);
        attr_html(html, res_html.autocomplete, blob_autocomplete(field->autocomplete));
        if (autofocus) empty_attr_html(html, res_html.autofocus);
        s64_attr_html(html, res_html.maxlength, param->field->max_size);
        if (valid_blob(value)) attr_html(html, res_html.value, value);
        close_tag_html(html);
    }
}

/*
void
numeric_input_html(blob_t * html, const blob_t * name, const blob_t * value)
{
    open_tag_html(html, res_html.input);
    attr_html(html, res_html.type, res_html.text_type);
    attr_html(html, res_html.name, name);
    attr_html(html, res_html.inputmode, res_html.numeric);
    //attr_html(html, res_html.autocomplete, res_html.off);
    if (valid_blob(value)) attr_html(html, res_html.value, value);
    close_tag_html(html);
}
*/

void
table_start_html(blob_t * html)
{
    start_element_html(html, res_html.table);
}

void
table_end_html(blob_t * html)
{
    end_element_html(html, res_html.table);
}

void
tr_start_html(blob_t * html)
{
    start_element_html(html, res_html.tr);
}

void
tr_end_html(blob_t * html)
{
    end_element_html(html, res_html.tr);
}

void
td_html(blob_t * html, blob_t * content)
{
    element_html(html, res_html.td, content);
}

void
td_start_html(blob_t * html)
{
    start_element_html(html, res_html.td);
}

void
td_end_html(blob_t * html)
{
    end_element_html(html, res_html.td);
}

void
th_html(blob_t * html, blob_t * content)
{
    element_html(html, res_html.th, content);
}

void
th_start_html(blob_t * html)
{
    start_element_html(html, res_html.th);
}

void
th_end_html(blob_t * html)
{
    end_element_html(html, res_html.th);
}

void
start_list_html(blob_t * html)
{
    start_element_html(html, res_html.ul);
}

void
end_list_html(blob_t * html)
{
    end_element_html(html, res_html.ul);
}

void
list_item_html(blob_t * html, blob_t * content)
{
    element_html(html, res_html.li, content);
}

