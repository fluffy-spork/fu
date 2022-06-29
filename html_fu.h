#pragma once

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
#define text_input(...) input_html(html, res_html.text_type, __VA_ARGS__);
#define submit_input(...) input_html(html, res_html.submit_type, __VA_ARGS__);
#define label(...) label_html(html, __VA_ARGS__);
#define para(...) para_html(html, __VA_ARGS__);
#define start_para() start_para_html(html);
#define end_para() end_para_html(html);
// NOTE(jason): have to do _tag because "div" is a c std
#define div_tag(...) div_html(html, __VA_ARGS__);
#define start_div(...) start_div_html(html, __VA_ARGS__);
#define end_div() end_div_html(html);
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

// NOTE(jason): these are mainly for internal use
struct {
    // dont' like the duplicate and there will possibly need to be some default
    // resources.  Maybe in blob or a resources_fu.h
    blob_t * space;
    blob_t * double_quote_ref;
    blob_t * less_than_ref;

    blob_t * doctype;

    blob_t * begin_head;
    blob_t * end_head;

    blob_t * begin_title;
    blob_t * end_title;

    blob_t * begin_page;
    blob_t * end_page;

    blob_t * id;

    blob_t * start_tag;
    blob_t * end_tag;
    blob_t * close_tag;

    blob_t * open_attr_value;
    blob_t * close_attr_value;

    blob_t * h1;

    blob_t * img;
    blob_t * src;
    blob_t * alt;

    blob_t * anchor;
    blob_t * href;

    blob_t * div;

    blob_t * para;

    blob_t * form;
    blob_t * action;
    blob_t * method;
    blob_t * get_method;
    blob_t * post_method;

    blob_t * input;
    blob_t * type;
    blob_t * text_type;
    blob_t * submit_type;
    blob_t * name;
    blob_t * value;
    blob_t * autocomplete;
    blob_t * off;
    blob_t * on;

    blob_t * label;

    blob_t * table;
    blob_t * tr;
    blob_t * td;
    blob_t * th;
} res_html;

// TODO(jason): is there a reason this isn't just init_html?
void
init_html()
{
    res_html.space = const_blob(" ");
    res_html.double_quote_ref = const_blob("&quot;");
    res_html.less_than_ref = const_blob("&lt;");

    res_html.doctype = const_blob("<!doctype html>");

    res_html.begin_head = const_blob("<html><head>");
    res_html.end_head = const_blob("<link rel=\"stylesheet\" href=\"/res/main.css\"></head>");

    res_html.begin_title = const_blob("<title>");
    res_html.end_title = const_blob("</title>");

    res_html.begin_page = const_blob("<body>");
    res_html.end_page = const_blob("</body></html>");

    res_html.id = const_blob("id");

    // XXX: maybe these names should be different
    res_html.start_tag = const_blob("<");
    res_html.end_tag = const_blob("</");
    res_html.close_tag = const_blob(">");

    res_html.open_attr_value = const_blob("=\"");
    res_html.close_attr_value = const_blob("\"");

    res_html.h1 = const_blob("h1");

    res_html.div = const_blob("div");

    res_html.img = const_blob("img");
    res_html.src = const_blob("src");
    res_html.alt = const_blob("alt");

    res_html.anchor = const_blob("a");
    res_html.href = const_blob("href");

    res_html.para = const_blob("p");

    res_html.form = const_blob("form");
    res_html.action = const_blob("action");
    res_html.method = const_blob("method");
    res_html.get_method = const_blob("get");
    res_html.post_method = const_blob("post");

    res_html.input = const_blob("input");
    res_html.name = const_blob("name");
    res_html.value = const_blob("value");
    res_html.type = const_blob("type");
    res_html.text_type = const_blob("text");
    res_html.submit_type = const_blob("submit");
    res_html.autocomplete = const_blob("autocomplete");
    res_html.off = const_blob("off");
    res_html.on = const_blob("on");

    res_html.label = const_blob("label");

    res_html.table = const_blob("table");
    res_html.tr = const_blob("tr");
    res_html.th = const_blob("th");
    res_html.td = const_blob("td");
}

ssize_t
escape_html(blob_t * html, const blob_t * content)
{
    assert_html();

    return escape_blob(html, content, '<', res_html.less_than_ref);
}

ssize_t
double_quote_escape_html(blob_t * html, const blob_t * content)
{
    assert_html();

    return escape_blob(html, content, '\"', res_html.double_quote_ref);
}

void
head_html(blob_t * html, const blob_t * title)
{
    assert_html();

    add_blob(html, res_html.begin_head);
    add_blob(html, res_html.begin_title);
    add_blob(html, title);
    add_blob(html, res_html.end_title);
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

void
start_element_html(blob_t * html, const blob_t * name)
{
    assert_html();

    add_blob(html, res_html.start_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

void
end_element_html(blob_t * html, const blob_t * name)
{
    assert_html();

    add_blob(html, res_html.end_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

void
element_html(blob_t * html, const blob_t * name, const blob_t * content)
{
    assert_html();

    assert(name != NULL);
    assert(content != NULL);

    start_element_html(html, name);
    escape_html(html, content);
    end_element_html(html, name);
}

void
open_tag_html(blob_t * html, const blob_t *name)
{
    assert_html();

    add_blob(html, res_html.start_tag);
    add_blob(html, name);
}

void
close_tag_html(blob_t * html)
{
    assert_html();

    add_blob(html, res_html.close_tag);
}

void
end_tag_html(blob_t * html, const blob_t *name)
{
    assert_html();

    add_blob(html, res_html.end_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

// XXX: should be a way to validate an attribute name is valid, but doesn't
// need to happen all the time.  maybe just add helper functions like src_attr()
void
attr_html(blob_t * html, const blob_t * name, const blob_t * value)
{
    assert_html();

    add_blob(html, res_html.space);
    add_blob(html, name);
    add_blob(html, res_html.open_attr_value);

    //&quot;
    double_quote_escape_html(html, value);

    add_blob(html, res_html.close_attr_value);
}

void
empty_attr_html(blob_t * html, const blob_t * name)
{
    assert_html();

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
    assert_html();

    element_html(html, res_html.div, content);
}

void
start_div_html(blob_t * html, blob_t * id)
{
    assert_html();

    open_tag_html(html, res_html.div);
    attr_html(html, res_html.id, id);
    close_tag_html(html);
}

void
end_div_html(blob_t * html)
{
    assert_html();

    end_element_html(html, res_html.div);
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

void
_start_form_html(blob_t * html, const blob_t * action, const blob_t * method)
{
    assert_html();
    assert(action != NULL);

    open_tag_html(html, res_html.form);
    attr_html(html, res_html.method, method);
    attr_html(html, res_html.action, action);

    close_tag_html(html);
}

void
start_post_form_html(blob_t * html, const blob_t * action)
{
    assert_html();

    _start_form_html(html, action, res_html.post_method);
}

void
start_get_form_html(blob_t * html, const blob_t * action)
{
    _start_form_html(html, action, res_html.get_method);
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

