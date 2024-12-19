#pragma once

#include <math.h>

// NOTE(jason): not doing single character method names so there's "para()"
// instead of "p()" for making a paragraph.  Using "link" instead of "anchor"
// since it's more meaningful

// TODO(jason): could these be defined as a macro at the definition?
// html_gen(img) that defines the macro then the function?  I would prefer to
// just be able to access the macro name within the macro, but doesn't seem
// possible
#define page_begin(...) page_begin_html(html, __VA_ARGS__)
#define page_end() page_end_html(html)
#define h1(...) h1_html(html, __VA_ARGS__)
#define h1_link(...) h1_link_html(html, __VA_ARGS__)
#define img(...) img_html(html, __VA_ARGS__)
#define video(...) video_html(html, __VA_ARGS__, false)
#define autoplay_video(...) video_html(html, __VA_ARGS__)
#define start_post_form(...) start_post_form_html(html, __VA_ARGS__)
#define start_get_form(...) start_get_form_html(html, __VA_ARGS__)
#define end_form() end_form_html(html)
#define param_input(...) param_input_html(html, __VA_ARGS__)
#define text_input(...) input_html(html, res_html.text_type, __VA_ARGS__)
#define hidden_input(...) input_html(html, res_html.hidden_type, __VA_ARGS__)
#define numeric_input(...) numeric_input_html(html, __VA_ARGS__)
#define submit_input(...) input_html(html, res_html.submit_type, __VA_ARGS__)
#define button(...) button_html(html, __VA_ARGS__)
#define start_button_bar() start_button_bar_html(html)
#define end_button_bar() end_button_bar_html(html)
#define start_action_bar() start_action_bar_html(html)
#define end_action_bar() end_action_bar_html(html)
#define label(...) label_html(html, __VA_ARGS__)
#define para(...) para_html(html, __VA_ARGS__)
#define start_para() start_para_html(html)
#define end_para() end_para_html(html)
#define div_id(...) div_id_html(html, __VA_ARGS__)
#define div_class(...) div_class_html(html, __VA_ARGS__)
#define div_id_class(...) div_id_class_html(html, __VA_ARGS__)
#define placeholder_div(class_name) div_class_html(html, class_name, NULL)
#define start_div_id(...) start_div_id_html(html, __VA_ARGS__)
#define start_div_id_class(...) start_div_id_class_html(html, __VA_ARGS__)
#define end_div() end_div_html(html)
#define start_div_class(...) start_div_class_html(html, __VA_ARGS__)
#define start_link(...) start_link_html(html, __VA_ARGS__)
#define end_link(...) end_link_html(html)
#define link(url, content) link_html(html, url, content, NULL)
#define link_target(url, content, target) link_html(html, url, content, target)
#define table_start() table_start_html(html, NULL)
#define table_class_start(class_name) table_start_html(html, class_name)
#define table_end() table_end_html(html)
#define tr_start() tr_start_html(html)
#define tr_end() tr_end_html(html)
#define td(content) td_html(html, content)
#define td_start() td_start_html(html)
#define td_end() td_end_html(html)
#define th(content) th_html(html, content)
#define th_start() th_start_html(html)
#define th_end() th_end_html(html)
#define thtd(th, td) thtd_html(html, th, td)
#define trthtd(th, td) trthtd_html(html, th, td)
// NOTE(jason): not sure about the B() usage.  probably only used for debug
// maybe debug_html or something?
#define trthtd_var(var) trthtd_html(html, B(#var), var)
#define trthtd_var_s64(var) trthtd_html(html, B(#var), stk_s64_blob(var))
#define start_list() start_list_html(html)
#define end_list() end_list_html(html)
#define list_item(...) list_item_html(html, __VA_ARGS__)
#define script(...) script_html(html, __VA_ARGS__)
#define res_script(...) res_script_html(html, __VA_ARGS__)

#define AUTOCOMPLETE_ENUM(var, E) \
    E(off, "off", var) \
    E(on, "on", var) \
    E(email, "email", var) \
    E(name, "name", var) \

ENUM_BLOB(autocomplete, AUTOCOMPLETE_ENUM)

#define RES_HTML(var, E) \
    E(space, " ", var) \
    E(zero, "0", var) \
    E(double_quote_ref, "&quot;", var) \
    E(less_than_ref, "&lt;", var) \
    E(error, "error", var) \
    E(errors, "errors", var) \
    E(doctype, "<!doctype html>", var) \
    E(begin_head, "<html><head><meta name=\"theme-color\" content=\"#000\"><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">", var) \
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
    E(class_name, "class", var) \
    E(start_tag, "<", var) \
    E(end_tag, "</", var) \
    E(close_tag, ">", var) \
    E(open_attr_value, "=\"", var) \
    E(close_attr_value, "\"", var) \
    E(h1, "h1", var) \
    E(div, "div", var) \
    E(img, "img", var) \
    E(src, "src", var) \
    E(data_src, "data-src", var) \
    E(alt, "alt", var) \
    E(video, "video", var) \
    E(controls, "controls", var) \
    E(controlslist, "controlslist", var) \
    E(controlslist_all, "nodownload, nofullscreen, noremoteplayback", var) \
    E(autoplay, "autoplay", var) \
    E(preload, "preload", var) \
    E(preload_auto, "auto", var) \
    E(preload_none, "none", var) \
    E(preload_metadata, "metadata", var) \
    E(poster, "poster", var) \
    E(playsinline, "playsinline", var) \
    E(anchor, "a", var) \
    E(href, "href", var) \
    E(para, "p", var) \
    E(form, "form", var) \
    E(action, "action", var) \
    E(method, "method", var) \
    E(enctype, "enctype", var) \
    E(multipart_form_data, "multipart/form-data", var) \
    E(get_method, "get", var) \
    E(post_method, "post", var) \
    E(field, "field", var) \
    E(field_value, "field-value", var) \
    E(input, "input", var) \
    E(name, "name", var) \
    E(value, "value", var) \
    E(type, "type", var) \
    E(text_type, "text", var) \
    E(submit_type, "submit", var) \
    E(hidden_type, "hidden", var) \
    E(button, "button", var) \
    E(file_type, "file", var) \
    E(file_class, "file", var) \
    E(file_input_class, "file visually-hidden", var) \
    E(uploads_class, "uploads hidden", var) \
    E(multiple, "multiple", var) \
    E(data_max_size_upload, "data-max-size-upload", var) \
    E(data_max_size_upload_msg, "data-max-size-upload-msg", var) \
    E(accept, "accept", var) \
    E(accept_video, "video/mp4,.mp4,.mov", var) \
    E(accept_image, "image/jpeg,image/png,image/gif,.jpeg,.jpg,.png,.gif", var) \
    E(accept_image_video, "video/mp4,image/jpeg,image/png,image/gif,.mp4,.mov,.jpg,.jpeg,.png,.gif", var) \
    E(no_files_selected, "no files selected", var) \
    E(minlength, "minlength", var) \
    E(maxlength, "maxlength", var) \
    E(tabindex, "tabindex", var) \
    E(ignore_tabindex, "-1", var) \
    E(select, "select", var) \
    E(option, "option", var) \
    E(selected, "selected", var) \
    E(disabled, "disabled", var) \
    E(cols, "cols", var) \
    E(rows, "rows", var) \
    E(autocomplete, "autocomplete", var) \
    E(autofocus, "autofocus", var) \
    E(placeholder, "placeholder", var) \
    E(capture, "capture", var) \
    E(environment, "environment", var) \
    E(action_bar, "action-bar", var) \
    E(button_bar, "button-bar", var) \
    E(textarea, "textarea", var) \
    E(label, "label", var) \
    E(for_id, "for", var) \
    E(table, "table", var) \
    E(tr, "tr", var) \
    E(th, "th", var) \
    E(td, "td", var) \
    E(ul, "ul", var) \
    E(li, "li", var) \
    E(target, "target", var) \
    E(target_blank, "_blank", var) \
    E(center_class, "center", var) \
    E(content_class, "content", var) \
    E(script, "script", var) \
    E(defer, "defer", var) \

ENUM_BLOB(res_html, RES_HTML)

typedef int param_options_html_f(blob_t * html, param_t * param);

typedef struct {
    s64 max_size_upload;
    param_options_html_f * param_options;
} config_html_t;

config_html_t config_html;

blob_t * _max_size_upload_msg;
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
init_html(config_html_t * config)
{
    init_autocomplete();
    init_res_html();

    config_html = *config;

    blob_t * b = blob(64);
    add_blob(b, B("upload too large: "));
    add_s64_blob(b, megabytes(config_html.max_size_upload));
    add_blob(b, B("MB"));
    _max_size_upload_msg = b;
    //debug_blob(_max_size_upload_msg);

    b = blob(1024);
    add_blob(b, B("?v="));
    add_random_blob(b, 8);
    _res_suffix_html = b;
    //debug_blob(_res_suffix_html);

    b = blob(1024);
    AB(b, "<link rel=\"stylesheet\" href=\"");
    res_url(b, B("main.css"));
    AB(b, "\">");
    AB(b, "<script defer src=\"");
    res_url(b, B("main.js"));
    AB(b, "\"></script>");
    AB(b, "<link rel=\"manifest\" href=\"");
    AB(b, "/manifest.json");
    add_blob(b, _res_suffix_html);
    AB(b, "\" crossorigin=\"use-credentials\">");
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
    add_blob(html, res_html.doctype);

    head_html(html, title);

    add_blob(html, res_html.begin_page);
}

void
page_end_html(blob_t * html)
{
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
    add_blob(html, res_html.start_tag);
    add_blob(html, name);
    add_blob(html, res_html.close_tag);
}

void
start_element_id_class_html(blob_t * html, const blob_t * name, const blob_t * id, const blob_t * class_name)
{
    add_blob(html, res_html.start_tag);
    add_blob(html, name);
    if (id) attr_html(html, res_html.id, id);
    if (class_name) attr_html(html, res_html.class_name, class_name);
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
    if (content) escape_html(html, content);
    end_element_html(html, name);
}

void
element_id_class_html(blob_t * html, const blob_t * name, const blob_t * id, const blob_t * class_name, const blob_t * content)
{
    start_element_id_class_html(html, name, id, class_name);
    if (content) escape_html(html, content);
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
    attr_html(html, res_html.src, src);
}

void
defer_attr_html(blob_t * html)
{
    empty_attr_html(html, res_html.defer);
}

void
script_html(blob_t * html, blob_t * path)
{
    open_tag_html(html, res_html.script);
    if (valid_blob(path)) {
        src_attr_html(html, path);
        defer_attr_html(html);
    }
    close_tag_html(html);

    end_element_html(html, res_html.script);
}

void
res_script_html(blob_t * html, blob_t * file)
{
    blob_t * url = stk_blob(1024);
    res_url(url, file);

    script_html(html, url);
}

void
start_link_html(blob_t * html, const blob_t * url, const blob_t * target)
{
    assert(url != NULL);

    open_tag_html(html, res_html.anchor);
    attr_html(html, res_html.href, url);
    if (target) attr_html(html, res_html.target, target);
    close_tag_html(html);
}

void
end_link_html(blob_t * html)
{
    end_tag_html(html, res_html.anchor);
}

void
link_html(blob_t * html, const blob_t * url, const blob_t * content, const blob_t * target)
{
    // TODO(jason): maybe link should take another blob_t * query parameter instead
    // of assuming they're already in the url

    start_link_html(html, url, target);

    if (content) escape_html(html, content);

    end_link_html(html);
}

void
h1_html(blob_t * html, const blob_t * content)
{
    element_html(html, res_html.h1, content);
}

void
h1_link_html(blob_t * html, const blob_t * content, const blob_t * url)
{
    start_element_html(html, res_html.h1);
    link_html(html, url, content, NULL);
    end_element_html(html, res_html.h1);
}

void
img_html(blob_t * html, const blob_t * src, const blob_t * alt)
{
    open_tag_html(html, res_html.img);
    attr_html(html, res_html.src, src);
    if (alt) attr_html(html, res_html.alt, alt);
    close_tag_html(html);
}

void
video_html(blob_t * html, const blob_t * src, const blob_t * preload, const blob_t * poster, bool autoplay)
{
    open_tag_html(html, res_html.video);
    attr_html(html, res_html.src, src);
    // NOTE(jason): preload can make pages slow if there's a lot of videos.
    // really need preload_none and posters
    //attr_html(html, res_html.preload, res_html.preload_none);
    if (valid_blob(preload)) attr_html(html, res_html.preload, preload);
    if (valid_blob(poster)) attr_html(html, res_html.poster, poster);
    if (autoplay) empty_attr_html(html, res_html.autoplay);
    empty_attr_html(html, res_html.controls);
    attr_html(html, res_html.controlslist, res_html.controlslist_all);
    // NOTE(jason): bullshit to work around safari iphone always playing stuff fullscreen
    empty_attr_html(html, res_html.playsinline);
    //if (alt) attr_html(html, res_html.alt, alt);
    close_tag_html(html);

    end_tag_html(html, res_html.video);
}

// for background video tags that aren't visible initially and shouldn't load
// anything, including a poster
//
// src is in data-src and needs to be copied to src when loading
//
// like in a slideshow
void
data_video_html(blob_t * html, const blob_t * src)
{
    open_tag_html(html, res_html.video);
    attr_html(html, res_html.data_src, src);
    attr_html(html, res_html.preload, res_html.preload_none);
    empty_attr_html(html, res_html.controls);
    attr_html(html, res_html.controlslist, res_html.controlslist_all);
    // NOTE(jason): bullshit to work around safari iphone always playing stuff fullscreen
    empty_attr_html(html, res_html.playsinline);
    close_tag_html(html);

    end_tag_html(html, res_html.video);
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
start_div_id_class_html(blob_t * html, const blob_t * id, const blob_t * class_name)
{
    start_element_id_class_html(html, res_html.div, id, class_name);
}

void
start_div_class_html(blob_t * html, const blob_t * class_name)
{
    start_element_id_class_html(html, res_html.div, NULL, class_name);
}

void
div_id_class_html(blob_t * html, const blob_t * id, const blob_t * class_name, const blob_t * content)
{
    element_id_class_html(html, res_html.div, id, class_name, content);
}

void
div_id_html(blob_t * html, const blob_t * id, const blob_t * content)
{
    element_id_class_html(html, res_html.div, id, NULL, content);
}

void
div_class_html(blob_t * html, const blob_t * class_name, const blob_t * content)
{
    element_id_class_html(html, res_html.div, NULL, class_name, content);
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
_start_form_html(blob_t * html, const blob_t * action, const blob_t * method, const blob_t * enctype, autocomplete_t autocomplete)
{
    assert_not_null(action);

    open_tag_html(html, res_html.form);
    attr_html(html, res_html.method, method);
    attr_html(html, res_html.action, action);
    if (enctype) attr_html(html, res_html.enctype, enctype);
    attr_html(html, res_html.autocomplete, blob_autocomplete(autocomplete));

    close_tag_html(html);
}

void
start_post_form_html(blob_t * html, const blob_t * action, autocomplete_t autocomplete)
{
    _start_form_html(html, action, res_html.post_method, NULL, autocomplete);
}

void
start_get_form_html(blob_t * html, const blob_t * action, autocomplete_t autocomplete)
{
    _start_form_html(html, action, res_html.get_method, NULL, autocomplete);
}

void
end_form_html(blob_t * html)
{
    end_element_html(html, res_html.form);
}

void
button_html(blob_t * html, const blob_t * class_name, const blob_t * content, bool disabled)
{
    open_tag_html(html, res_html.button);
    if (class_name) attr_html(html, res_html.class_name, class_name);
    attr_html(html, res_html.type, res_html.button);
    if (disabled) empty_attr_html(html, res_html.disabled);
    close_tag_html(html);
    escape_html(html, content);
    end_element_html(html, res_html.button);
}

// TODO(jason): in the fluffy CSS this is flexbox and has some extra styles in
// certain places to get wrap, etc.  still might be better to just make it a table
void
start_button_bar_html(blob_t * html)
{
    start_div_class_html(html, res_html.button_bar);
}

void
end_button_bar_html(blob_t * html)
{
    end_div_html(html);
}

void
start_action_bar_html(blob_t * html)
{
    start_div_class_html(html, res_html.action_bar);
    start_button_bar_html(html);
}

void
end_action_bar_html(blob_t * html)
{
    end_button_bar_html(html);
    end_div_html(html);
}

void
label_html(blob_t * html, blob_t * content, blob_t * for_id)
{
    assert(content != NULL);
    // NOTE(jason): for_id is optional because file input won't show the file
    // dialog when the label is clicked.  I actually thing for_id was added
    // explicitly for the file input, but shouldn't remove as it's desirable
    // for accessiblilty anyway.
    //assert(for_id != NULL);

    open_tag_html(html, res_html.label);
    attr_html(html, res_html.for_id, for_id);
    close_tag_html(html);

    escape_html(html, content);

    end_tag_html(html, res_html.label);
}

void
input_html(blob_t * html, const blob_t * type, const blob_t * name, const blob_t * value)
{
    open_tag_html(html, res_html.input);
    attr_html(html, res_html.type, type);
    // NOTE(jason): can't set the id by default as you don't know it will be
    // unique.  IDK if that still matters.  Hopefully only 1 form per page in
    // most cases.
    //attr_html(html, res_html.id, name);
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
    empty_attr_html(html, res_html.autofocus);
    close_tag_html(html);

    end_form();
}

void
hidden_post_form_html(blob_t * html, blob_t * action, field_t * field, blob_t * value, blob_t * label)
{
    start_post_form(action, off_autocomplete);
    hidden_input(field->name, value);
    submit_input(res_html.button, label);
    end_form();
}

/*
 * NOTE(jason): so far, everytime i think i want this function i end up using post.  we'll see.
 *
void
hidden_submit_get_form_html(blob_t * html, blob_t * action, field_t * field, blob_t * value, blob_t * label)
{
    start_get_form(action, off_autocomplete);
    hidden_input(field->name, value);
    submit_input(res_html.button, label);
    end_form();
}
*/

void
textarea_html(blob_t * html, param_t * param, bool autofocus)
{
    field_t * f = param->field;
    blob_t * v = param->value;

    open_tag_html(html, res_html.textarea);
    attr_html(html, res_html.name, f->name);
    //attr_html(html, res_html.autocomplete, blob_autocomplete(f->autocomplete));
    if (autofocus) empty_attr_html(html, res_html.autofocus);
    // TODO(jason): setting rows and cols conflicts with setting the size in
    // css.  particularly the width.  it may be ok to set rows.  my general
    // preference would be for the width and height to resize to always show
    // all characters and not require scrolling.  may not be possible with js
    //int cols = 50;
    //s64_attr_html(html, res_html.cols, cols);
    //s64_attr_html(html, res_html.rows, ceil((double)f->max_size/(cols - 1)));
    //s64_attr_html(html, res_html.rows, 5);
    if (f->min_size) s64_attr_html(html, res_html.minlength, f->min_size);
    if (f->max_size) s64_attr_html(html, res_html.maxlength, f->max_size);
    close_tag_html(html);

    if (valid_blob(v)) escape_html(html, v);

    end_tag_html(html, res_html.textarea);
}

void
option_html(blob_t * html, param_t * param, blob_t * value, blob_t * label)
{
    open_tag_html(html, res_html.option);
    attr_html(html, res_html.value, value);
    if (param && equal_blob(value, param->value)) empty_attr_html(html, res_html.selected);
    close_tag_html(html);
    escape_html(html, label);
    end_tag_html(html, res_html.option);
}

void
select_html(blob_t * html, param_t * param)
{
    open_tag_html(html, res_html.select);
    attr_html(html, res_html.name, param->field->name);
    close_tag_html(html);

    if (config_html.param_options) {
        config_html.param_options(html, param);
    }

    end_tag_html(html, res_html.select);
}

void
select_values_html(blob_t * html, param_t * param, blob_t ** values, blob_t ** labels, int n_values)
{
    open_tag_html(html, res_html.select);
    attr_html(html, res_html.name, param->field->name);
    close_tag_html(html);

    //blob_t * v = param->value;

    for (int i = 0; i < n_values; i++) {
        option_html(html, param, values[i], labels[i]);

        /*
        open_tag_html(html, res_html.option);
        attr_html(html, res_html.value, v);
        if (equal_blob(values[i], v)) empty_attr_html(html, res_html.selected);
        close_tag_html(html);
        escape_html(html, labels[i]);
        end_tag_html(html, res_html.option);
        */
    }

    end_tag_html(html, res_html.select);
}

void
param_html(blob_t * html, param_t * param)
{
    start_div_class(res_html.field);
    div_class(res_html.field_value, param->value);
    label(param->field->label, param->field->name);
    end_div();
}

// NOTE(jason): could be improved.  maybe other input types?
void
param_input_html(blob_t * html, param_t * param, bool autofocus)
{
    blob_t * value = param->value;
    field_t * field = param->field;

#define TEXT_AREA_MIN_SIZE 64

    if (field->type == text_type_field && field->max_size > TEXT_AREA_MIN_SIZE) {
        start_div_class(res_html.field);
        textarea_html(html, param, autofocus);
        label(field->label, field->name);
        end_div();
    }
    else if (field->type == select_integer_type_field) {
        start_div_class(res_html.field);

        select_html(html, param);

        label(field->label, field->name);
        end_div();
    }
    else if (field->type == file_type_field || field->type == file_multiple_type_field) {
        start_div_class(res_html.field);
        open_tag_html(html, res_html.input);
        attr_html(html, res_html.type, res_html.file_type);
        s64_attr_html(html, res_html.data_max_size_upload, config_html.max_size_upload);
        attr_html(html, res_html.data_max_size_upload_msg, _max_size_upload_msg);
        attr_html(html, res_html.tabindex, res_html.ignore_tabindex);
        // TODO(jason): default disabled and javascript will enable file input
        // until multipart/form-data is implemented.  most people will have JS
        // anyway
        empty_attr_html(html, res_html.disabled);
        if (field->type == file_multiple_type_field) empty_attr_html(html, res_html.multiple);
        // NOTE(jason): file input field itself doesn't have an id or name as
        // it shouldn't send a value directly to the server.
        //attr_html(html, res_html.id, field->name);
        //attr_html(html, res_html.name, field->name);
        attr_html(html, res_html.class_name, res_html.file_input_class);
        // needs to be a param
        attr_html(html, res_html.accept, res_html.accept_image_video);
        // NOTE(jason): capture disables file selection on mobile
        //attr_html(html, res_html.capture, res_html.environment);
        //s64_attr_html(html, res_html.maxlength, param->field->max_size);
        close_tag_html(html);
        // NOTE(jason): the for id causes problems since it doesn't currently
        // work for mobile chrome.  At least on linux chrome, with for id
        // enabled and the javascript click event added the file dialog shows
        // twice:(
        // possibly a way to disable the default behavior with JS and leave the
        // for id
        //label(field->label, NULL);
        //div_class(res_html.file_preview_class, res_html.no_files_selected);

        hidden_input(field->name, res_html.zero);
        div_class(res_html.error, NULL);
        button(res_html.file_class, field->label, true);
        div_class(res_html.uploads_class, NULL);
        end_div();
    }
    else if (field->type == hidden_type_field) {
        hidden_input(field->name, value);
    }
    else {
        start_div_class(res_html.field);
        open_tag_html(html, res_html.input);
        attr_html(html, res_html.type, res_html.text_type);
        attr_html(html, res_html.id, field->name);
        attr_html(html, res_html.name, field->name);
        //attr_html(html, res_html.autocomplete, blob_autocomplete(field->autocomplete));
        if (autofocus) empty_attr_html(html, res_html.autofocus);
        s64_attr_html(html, res_html.maxlength, param->field->max_size);
        if (valid_blob(value)) attr_html(html, res_html.value, value);
        close_tag_html(html);
        label(field->label, field->name);
        end_div();
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
table_start_html(blob_t * html, const blob_t * class_name)
{
   start_element_id_class_html(html, res_html.table, NULL, class_name);
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
td_html(blob_t * html, const blob_t * content)
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
th_html(blob_t * html, const blob_t * content)
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
thtd_html(blob_t * html, const blob_t * th, const blob_t * td)
{
    th(th);
    td(td);
}

// <tr><th>$th</th><td>$td</td></tr>
void
trthtd_html(blob_t * html, const blob_t * th, const blob_t * td)
{
    tr_start();
    th(th);
    td(td);
    tr_end();
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

