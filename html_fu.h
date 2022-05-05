#pragma once

// NOTE(jason): not doing single character method names so there's "anchor()"
// instead of "a()" for making a link.  Similar for paragraph

// NOTE(jason): these are mainly for internal use
struct {
    // dont' like the duplicate and there will possibly need to be some default
    // resources.  Maybe in blob or a resources_fu.h
    blob_t * space;
    blob_t * double_quote_ref;
    blob_t * less_than_ref;

    blob_t * begin_head;
    blob_t * end_head;

    blob_t * begin_title;
    blob_t * end_title;

    blob_t * begin_page;
    blob_t * end_page;

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

    blob_t * label;
} res_html;

// TODO(jason): is there a reason this isn't just init_html?
void
init_html()
{
    res_html.space = const_blob(" ");
    res_html.double_quote_ref = const_blob("&quot;");
    res_html.less_than_ref = const_blob("&lt;");

    res_html.begin_head = const_blob("<html><head>");
    res_html.end_head = const_blob("<link rel=\"stylesheet\" href=\"/res/main.css\"></head>");

    res_html.begin_title = const_blob("<title>");
    res_html.end_title = const_blob("</title>");

    res_html.begin_page = const_blob("<body>");
    res_html.end_page = const_blob("</body></html>");

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

    res_html.label = const_blob("label");
}

ssize_t
escape_html(blob_t * t, const blob_t * content)
{
    return escape_blob(t, content, '<', res_html.less_than_ref);
}

ssize_t
double_quote_escape(blob_t * t, const blob_t * content)
{
    return escape_blob(t, content, '\"', res_html.double_quote_ref);
}

void
head(blob_t *t, const blob_t * title)
{
    add_blob(t, res_html.begin_head);
    add_blob(t, res_html.begin_title);
    add_blob(t, title);
    add_blob(t, res_html.end_title);
    add_blob(t, res_html.end_head);
}

void
page_begin(blob_t *t, const blob_t * title)
{
    head(t, title);

    add_blob(t, res_html.begin_page);
}

void
page_end(blob_t *t)
{
    add_blob(t, res_html.end_page);
}

void
start_element(blob_t * t, const blob_t * name)
{
    add_blob(t, res_html.start_tag);
    add_blob(t, name);
    add_blob(t, res_html.close_tag);
}

void
end_element(blob_t * t, const blob_t * name)
{
    add_blob(t, res_html.end_tag);
    add_blob(t, name);
    add_blob(t, res_html.close_tag);
}

void
element(blob_t * t, const blob_t * name, const blob_t * content)
{
    assert(t != NULL);
    assert(name != NULL);
    assert(content != NULL);

    start_element(t, name);
    escape_html(t, content);
    end_element(t, name);
}

void
open_tag(blob_t *t, const blob_t *name)
{
    add_blob(t, res_html.start_tag);
    add_blob(t, name);
}

void
close_tag(blob_t *t)
{
    add_blob(t, res_html.close_tag);
}

void
end_tag(blob_t * t, const blob_t *name)
{
    add_blob(t, res_html.end_tag);
    add_blob(t, name);
    add_blob(t, res_html.close_tag);
}

// XXX: should be a way to validate an attribute name is valid, but doesn't
// need to happen all the time.  maybe just add helper functions like src_attr()
void
attr(blob_t *t, const blob_t * name, const blob_t * value)
{
    add_blob(t, res_html.space);
    add_blob(t, name);
    add_blob(t, res_html.open_attr_value);

    //&quot;
    double_quote_escape(t, value);

    add_blob(t, res_html.close_attr_value);
}

void
empty_attr(blob_t *t, const blob_t * name)
{
    add_blob(t, res_html.space);
    add_blob(t, name);
}

void
src_attr(blob_t *t, const blob_t * src)
{
    attr(t, res_html.src, src);
}

void
h1(blob_t *t, const blob_t * content)
{
    element(t, res_html.h1, content);
}

void
img(blob_t * t, const blob_t * src, const blob_t * alt)
{
    assert(src != NULL);

    open_tag(t, res_html.img);
    attr(t, res_html.src, src);
    if (alt) attr(t, res_html.alt, alt);
    close_tag(t);
}

void
anchor(blob_t * t, const blob_t * url, const blob_t * content)
{
    assert(url != NULL);

    open_tag(t, res_html.anchor);
    attr(t, res_html.href, url);
    close_tag(t);

    if (content != NULL) {
        escape_html(t, content);
    }

    end_tag(t, res_html.anchor);
}

void
div_html(blob_t * t, const blob_t * content)
{
    element(t, res_html.div, content);
}

void
start_div_html(blob_t * t)
{
    start_element(t, res_html.div);
}

void
end_div_html(blob_t * t)
{
    end_element(t, res_html.div);
}

void
para(blob_t * t, const blob_t * content)
{
    element(t, res_html.para, content);
}

void
start_para(blob_t * t)
{
    start_element(t, res_html.para);
}

void
end_para(blob_t * t)
{
    end_element(t, res_html.para);
}

void
_start_form(blob_t * t, const blob_t * action, const blob_t * method)
{
    assert(action != NULL);

    open_tag(t, res_html.form);
    attr(t, res_html.method, method);
    attr(t, res_html.action, action);

    close_tag(t);
}

void
start_post_form(blob_t * t, const blob_t * action)
{
    _start_form(t, action, res_html.post_method);
}

void
start_get_form(blob_t * t, const blob_t * action)
{
    _start_form(t, action, res_html.get_method);
}

void
end_form(blob_t * t)
{
    end_element(t, res_html.form);
}

void
label(blob_t * t, blob_t * content)
{
    element(t, res_html.label, content);
}

void
text_input(blob_t * t, const blob_t * name)
{
    open_tag(t, res_html.input);
    attr(t, res_html.type, res_html.text_type);
    attr(t, res_html.name, name);
    close_tag(t);
}

void
submit_input(blob_t * t, const blob_t * value)
{
    open_tag(t, res_html.input);
    attr(t, res_html.type, res_html.submit_type);
    attr(t, res_html.value, value);
    close_tag(t);
}

